const API = '/api/adaptive';
const QUESTION_WINDOW_SECONDS = 40;
const QUESTION_WINDOW_MS = QUESTION_WINDOW_SECONDS * 1000;

async function getQuestion() {
  const res = await fetch(API, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    credentials: 'include',
    body: JSON.stringify({})
  });
  const data = await res.json();
  return {
    ok: !!data?.success,
    problem: data?.data?.next_question?.problem || 'Question unavailable',
    raw: data
  };
}

function createRecognizer() {
  const SR = window.SpeechRecognition || window.webkitSpeechRecognition;
  if (!SR) { 
    alert('SpeechRecognition not supported in this browser. Please use Chrome, Edge, or Safari.'); 
    return null; 
  }
  const rec = new SR();
  rec.lang = 'en-US';
  rec.continuous = true;
  rec.interimResults = true;
  rec.maxAlternatives = 1;
  return rec;
}

let rec = null;
let finalTranscript = '';
let timer = null;
let stopped = false;

function enableButtons(listening) {
  document.getElementById('start').disabled = listening;
  document.getElementById('stop').disabled = !listening;
}

function stopAll() {
  if (stopped) return;
  stopped = true;
  try { 
    if (rec) {
      rec.stop();
      rec = null;
    }
  } catch (e) {
    console.error('Error stopping recognizer:', e);
  }
  if (timer) {
    clearTimeout(timer);
    timer = null;
  }
  enableButtons(false);
}

async function askAndListenWindow() {
  const out = document.getElementById('out');
  if (!out) return;

  // Stop any existing recognizer first
  if (rec) {
    try {
      rec.stop();
    } catch (e) {
      console.warn('Error stopping previous recognizer:', e);
    }
    rec = null;
  }

  // Request microphone permission first
  try {
    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
    // Stop the stream immediately - we just needed permission
    stream.getTracks().forEach(track => track.stop());
  } catch (error) {
    out.textContent = 'Error: Microphone permission denied or not available. Please allow microphone access and try again.';
    console.error('Microphone permission error:', error);
    return;
  }

  out.textContent = 'Fetching question...';
  const q = await getQuestion();
  if (!q.ok) {
    out.textContent = `Error fetching question: ${q.problem}`;
    return;
  }
  const isClosing = !!q.raw?.data?.next_question?.isClosing;
  if (isClosing) {
    out.textContent = `Interview complete:\n${q.problem}`;
    return;
  }
  out.textContent = `Question:\n${q.problem}\n\nListening for ${QUESTION_WINDOW_SECONDS} seconds...`;

  rec = createRecognizer();
  if (!rec) {
    out.textContent += '\nError: Speech recognition not available.';
    return;
  }
  finalTranscript = '';
  stopped = false;
  enableButtons(true);

  timer = setTimeout(() => {
    out.textContent += '\nTime up. Stopping...';
    stopAll();
  }, QUESTION_WINDOW_MS);

  rec.onresult = (e) => {
    if (stopped) return;
    let interim = '';
    for (let i = e.resultIndex; i < e.results.length; i++) {
      if (e.results[i] && e.results[i][0]) {
        const t = e.results[i][0].transcript;
        if (e.results[i].isFinal) finalTranscript += t + ' ';
        else interim += t;
      }
    }
    out.textContent = `Question:\n${q.problem}\n\nListening...\n\nInterim: ${interim}\nFinal: ${finalTranscript}`;
  };

  rec.onerror = (e) => {
    console.error('Speech recognition error:', e.error);
    if (e.error === 'not-allowed') {
      out.textContent += `\nError: Microphone permission denied. Please allow microphone access in your browser settings.`;
    } else if (e.error === 'no-speech') {
      console.warn('No speech detected');
    } else if (e.error === 'aborted') {
      console.warn('Speech recognition aborted');
    } else {
      out.textContent += `\nError: ${e.error}`;
    }
    stopAll();
  };

  rec.onend = async () => {
    if (timer) {
      clearTimeout(timer);
      timer = null;
    }
    if (stopped) return;
    
    out.textContent += `\nSending answer...`;
    const answerToSend = finalTranscript.trim();
    const payload = answerToSend ? { answer: answerToSend } : {};
    try {
      const res = await fetch(API, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        credentials: 'include',
        body: JSON.stringify(payload)
      });
      const data = await res.json();
      out.textContent = `Server response:\n${JSON.stringify(data, null, 2)}`;
      const closingQuestion = data?.data?.next_question;
      if (closingQuestion?.isClosing) {
        out.textContent += `\n\nInterview complete:\n${closingQuestion.problem || ''}`;
      }
    } catch (error) {
      out.textContent += `\nError sending answer: ${error.message}`;
      console.error('Error sending answer:', error);
    }
  };

  try {
    rec.start();
  } catch (error) {
    console.error('Error starting speech recognition:', error);
    out.textContent += '\nError: Failed to start microphone. Please check your browser settings and try again.';
    stopAll();
  }
}

window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('start').addEventListener('click', askAndListenWindow);
  document.getElementById('stop').addEventListener('click', stopAll);
});



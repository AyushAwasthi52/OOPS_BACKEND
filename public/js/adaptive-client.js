const API = '/api/adaptive';
const QUESTION_WINDOW_SECONDS = 40;

let questionCard, qDiff, qProblem, qIn, qOut, qConst, qEx, resp, interimEl, finalEl, typedEl, timerEl;

let rec = null;
let stopTimer = null;
let running = false;
let finalTranscript = '';
let audioStream = null; // Keep track of audio stream
let isSpeaking = false; // Track if TTS is currently speaking
let autoStartMicAfterTTS = false; // Flag to auto-start mic after TTS
let manuallyStopped = false; // Track if mic was manually stopped
let isSubmitting = false; // Prevent double submission

function formatSec(s) {
  const m = Math.floor(s/60).toString().padStart(2,'0');
  const r = (s%60).toString().padStart(2,'0');
  return `${m}:${r}`;
}

function startCountdown(seconds, onDone) {
  let s = seconds;
  timerEl.textContent = formatSec(s);
  stopTimer = () => {};
  const id = setInterval(() => {
    s -= 1;
    timerEl.textContent = formatSec(Math.max(0, s));
    if (s <= 0) {
      clearInterval(id);
      onDone && onDone();
    }
  }, 1000);
  stopTimer = () => clearInterval(id);
}

function createRecognizer() {
  const SR = window.SpeechRecognition || window.webkitSpeechRecognition;
  if (!SR) { 
    alert('SpeechRecognition not supported in this browser. Please use Chrome, Edge, or Safari.'); 
    return null; 
  }
  const r = new SR();
  r.lang = 'en-US';
  r.continuous = true;
  r.interimResults = true;
  r.maxAlternatives = 1;
  return r;
}

// Format question data into a readable text string for TTS (only problem statement)
function formatQuestionForSpeech(questionData) {
  if (!questionData) return '';
  
  // Only read the problem statement
  if (questionData.problem) {
    return questionData.problem;
  }
  
  return '';
}

// Format feedback for speech
function formatFeedbackForSpeech(evaluationText) {
  if (!evaluationText) return '';
  
  // Clean up the feedback text for speech
  let feedback = evaluationText.trim();
  
  // Add a prefix to make it clear it's feedback
  if (feedback) {
    feedback = 'Feedback: ' + feedback;
  }
  
  return feedback;
}

// Speak text using Web Speech API and optionally start mic after
async function speakText(text, autoStartMic = false) {
  if (!text || !text.trim()) {
    console.warn('No text to speak');
    if (autoStartMic) {
      await startMicWindow();
    }
    return;
  }

  if (!('speechSynthesis' in window)) {
    console.warn('Speech synthesis not supported in this browser');
    if (autoStartMic) {
      await startMicWindow();
    }
    return;
  }

  // Stop any ongoing speech
  window.speechSynthesis.cancel();
  isSpeaking = true;
  autoStartMicAfterTTS = autoStartMic;

  return new Promise((resolve) => {
    const utterance = new SpeechSynthesisUtterance(text);
    utterance.lang = 'en-US';
    utterance.rate = 0.9; // Slightly slower for clarity
    utterance.pitch = 1;
    utterance.volume = 1;

    utterance.onend = () => {
      console.log('Text-to-speech finished');
      isSpeaking = false;
      if (autoStartMicAfterTTS) {
        console.log('Auto-starting microphone after TTS...');
        autoStartMicAfterTTS = false;
        // Small delay before starting mic to ensure TTS is fully done
        setTimeout(async () => {
          await startMicWindow();
        }, 500);
      }
      resolve();
    };

    utterance.onerror = (event) => {
      console.error('Speech synthesis error:', event.error);
      isSpeaking = false;
      autoStartMicAfterTTS = false;
      if (autoStartMic) {
        // If TTS fails, still try to start mic
        setTimeout(async () => {
          await startMicWindow();
        }, 500);
      }
      resolve();
    };

    console.log('Speaking question:', text);
    if (resp) resp.textContent = 'Reading question aloud...';
    window.speechSynthesis.speak(utterance);
  });
}

// Stop any ongoing speech
function stopSpeaking() {
  if ('speechSynthesis' in window) {
    window.speechSynthesis.cancel();
    isSpeaking = false;
    autoStartMicAfterTTS = false;
  }
}

async function fetchQuestion() {
  const res = await fetch(API, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    credentials: 'include',
    body: JSON.stringify({})
  });
  const data = await res.json();
  // Do not show server response here; user hasn't answered yet
  resp.textContent = '(waiting for answer...)';
  const nq = data?.data?.next_question;
  if (nq) {
    const isClosing = !!nq.isClosing;
    questionCard.style.display = '';
    qDiff.textContent = nq.difficulty || '-';
    qProblem.textContent = nq.problem || '';
    qIn.textContent = nq.input_format || '';
    qOut.textContent = nq.output_format || '';
    qConst.textContent = nq.constraints || '';
    qEx.textContent = nq.example || '';
    if (isClosing) {
      resp.textContent = 'Interview complete. Thanks for taking the time today.';
      setMicButtons(false);
    }
    
    // Speak only the problem statement and auto-start mic after
    const questionText = formatQuestionForSpeech(nq);
    if (questionText) {
      await speakText(questionText, !isClosing); // only auto-start mic for active questions
    }
  }
}

async function submitAnswer(answer) {
  // Stop any ongoing speech before submitting
  stopSpeaking();
  
  const payload = answer ? { answer } : {};
  const res = await fetch(API, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    credentials: 'include',
    body: JSON.stringify(payload)
  });
  const data = await res.json();
  resp.textContent = JSON.stringify(data, null, 2);
  
  // Extract evaluation feedback
  const evaluation = data?.data?.evaluation;
  const nq = data?.data?.next_question;
  
  if (nq) {
    const isClosing = !!nq.isClosing;
    questionCard.style.display = '';
    qDiff.textContent = nq.difficulty || '-';
    qProblem.textContent = nq.problem || '';
    qIn.textContent = nq.input_format || '';
    qOut.textContent = nq.output_format || '';
    qConst.textContent = nq.constraints || '';
    qEx.textContent = nq.example || '';
    if (isClosing) {
      resp.textContent = 'Interview complete. Great work!';
      setMicButtons(false);
    }
    
    // First speak the feedback if available
    if (evaluation) {
      const feedbackText = formatFeedbackForSpeech(evaluation);
      if (feedbackText) {
        console.log('Speaking feedback:', feedbackText);
        await speakText(feedbackText, false); // Don't start mic after feedback
        // Small pause between feedback and next question
        await new Promise(resolve => setTimeout(resolve, 500));
      }
    }
    
    // Then speak the next question and auto-start mic after
    const questionText = formatQuestionForSpeech(nq);
    if (questionText) {
      await speakText(questionText, !isClosing); // only auto-start mic when not closing
    }
  }
}

function setMicButtons(listening) {
  document.getElementById('btn-start-mic').disabled = listening;
  document.getElementById('btn-stop-mic').disabled = !listening;
}

function stopMic(manual = false) {
  if (!running && !manual) return;
  manuallyStopped = manual;
  running = false;
  try { 
    if (rec) {
      rec.stop();
      rec = null;
    }
  } catch (e) {
    console.error('Error stopping recognizer:', e);
  }
  // Stop the audio stream
  if (audioStream) {
    audioStream.getTracks().forEach(track => track.stop());
    audioStream = null;
  }
  if (stopTimer) {
    stopTimer();
    stopTimer = null;
  }
  // Update textarea with final transcript when stopping
  if (typedEl && finalTranscript.trim()) {
    typedEl.value = finalTranscript.trim();
  }
  setMicButtons(false);
}

// Stop both speech and microphone
function stopAll() {
  stopSpeaking();
  stopMic(true); // Mark as manually stopped
  // Even if manually stopped, submit the answer if there's any transcript
  if (finalTranscript.trim()) {
    console.log('Manually stopped, but submitting answer:', finalTranscript.trim());
    submitAnswer(finalTranscript.trim());
  }
}

async function startMicWindow() {
  // Don't start mic if TTS is currently speaking
  if (isSpeaking) {
    console.log('TTS is speaking, waiting for it to finish before starting mic...');
    return;
  }
  
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
    audioStream = await navigator.mediaDevices.getUserMedia({ audio: true });
    console.log('Microphone permission granted');
    // Note: SpeechRecognition manages its own audio stream, but we need permission first
    // We'll stop this stream after SpeechRecognition starts
  } catch (error) {
    alert('Microphone permission denied or not available. Please allow microphone access and try again.');
    console.error('Microphone permission error:', error);
    return;
  }

  rec = createRecognizer();
  if (!rec) {
    // Clean up stream if recognizer creation failed
    if (audioStream) {
      audioStream.getTracks().forEach(track => track.stop());
      audioStream = null;
    }
    return;
  }

  finalTranscript = '';
  manuallyStopped = false;
  isSubmitting = false;
  if (interimEl) interimEl.textContent = '';
  if (finalEl) finalEl.textContent = '';
  if (typedEl) typedEl.value = ''; // Clear textarea when starting
  running = true;
  setMicButtons(true);
  startCountdown(QUESTION_WINDOW_SECONDS, async () => {
    console.log('Timer expired, stopping mic and submitting answer...');
    if (isSubmitting) {
      console.log('Already submitting, skipping...');
      return;
    }
    isSubmitting = true;
    manuallyStopped = false; // Timer expired, not manually stopped
    stopMic(false); // Not manually stopped, timer expired
    // Auto-submit when timer expires
    if (finalTranscript.trim()) {
      await submitAnswer(finalTranscript.trim());
    } else {
      if (resp) resp.textContent = 'Time up. No speech detected.';
      // Still fetch next question even if no answer
      await fetchQuestion();
    }
    isSubmitting = false;
  });

  rec.onresult = (e) => {
    if (!running) return;
    console.log('Speech recognition result received:', e);
    let interim = '';
    let newFinal = '';
    
    for (let i = e.resultIndex; i < e.results.length; i++) {
      if (e.results[i] && e.results[i][0]) {
        const t = e.results[i][0].transcript;
        if (e.results[i].isFinal) {
          finalTranscript += t + ' ';
          newFinal += t + ' ';
        } else {
          interim += t;
        }
      }
    }
    
    console.log('Final transcript:', finalTranscript);
    console.log('Interim transcript:', interim);
    
    if (interimEl) interimEl.textContent = interim;
    if (finalEl) finalEl.textContent = finalTranscript;
    
    // Update the textarea with the current transcript (final + interim)
    const fullText = finalTranscript + interim;
    if (typedEl) {
      typedEl.value = fullText;
      console.log('Updated textarea with:', fullText);
    } else {
      console.error('typedEl is null!');
    }
  };

  rec.onstart = () => {
    console.log('Speech recognition started');
    if (resp) resp.textContent = 'Listening... Speak now!';
    // SpeechRecognition has its own audio stream, so we can stop the getUserMedia stream
    if (audioStream) {
      audioStream.getTracks().forEach(track => track.stop());
      audioStream = null;
      console.log('Stopped getUserMedia stream - SpeechRecognition is handling audio');
    }
  };

  rec.onspeechstart = () => {
    console.log('Speech detected');
  };

  rec.onspeechend = () => {
    console.log('Speech ended');
  };

  rec.onerror = (e) => {
    console.error('Speech recognition error:', e.error);
    if (e.error === 'not-allowed') {
      alert('Microphone permission denied. Please allow microphone access in your browser settings.');
    } else if (e.error === 'no-speech') {
      console.warn('No speech detected');
      // Don't stop on no-speech, let it continue listening
      return;
    } else if (e.error === 'aborted') {
      console.warn('Speech recognition aborted');
      // If aborted, submit what we have
      if (finalTranscript.trim() && !manuallyStopped) {
        submitAnswer(finalTranscript.trim());
      }
      return;
    } else {
      console.error('Speech recognition error:', e.error);
    }
    stopMic(false);
  };

  rec.onend = async () => {
    console.log('Speech recognition ended. Running:', running, 'Manually stopped:', manuallyStopped, 'Is submitting:', isSubmitting, 'Final transcript:', finalTranscript);
    // Clean up audio stream
    if (audioStream) {
      audioStream.getTracks().forEach(track => track.stop());
      audioStream = null;
    }
    
    // Update textarea with final transcript
    if (typedEl && finalTranscript.trim()) {
      typedEl.value = finalTranscript.trim();
      console.log('Updated textarea with final transcript:', finalTranscript.trim());
    }
    
    // If already submitting (e.g., from timer), don't submit again
    if (isSubmitting) {
      console.log('Already submitting, skipping onend submission');
      return;
    }
    
    // If manually stopped, don't auto-submit (stopAll handles submission)
    if (manuallyStopped) {
      console.log('Manually stopped - not auto-submitting');
      return;
    }
    
    // Prevent auto-restart if we're about to submit
    if (!running) {
      return;
    }
    
    // Auto-submit when mic stops naturally (not manually)
    isSubmitting = true;
    if (finalTranscript.trim()) {
      console.log('Auto-submitting answer and moving to next question...');
      running = false; // Prevent restart
      await submitAnswer(finalTranscript.trim());
    } else {
      console.log('No speech detected, fetching next question anyway...');
      running = false; // Prevent restart
      if (resp) resp.textContent = 'No speech detected. Moving to next question...';
      // Still move to next question even if no answer
      await fetchQuestion();
    }
    isSubmitting = false;
  };

  try { 
    console.log('Starting speech recognition...');
    rec.start();
    console.log('Speech recognition start() called successfully');
  } catch (error) {
    console.error('Error starting speech recognition:', error);
    alert('Failed to start microphone: ' + error.message + '. Please check your browser settings and try again.');
    // Clean up on error
    if (audioStream) {
      audioStream.getTracks().forEach(track => track.stop());
      audioStream = null;
    }
    stopMic();
  }
}

window.addEventListener('DOMContentLoaded', () => {
  console.log('DOM loaded, initializing...');
  // Initialize DOM element references
  questionCard = document.getElementById('question-card');
  qDiff = document.getElementById('q-diff');
  qProblem = document.getElementById('q-problem');
  qIn = document.getElementById('q-in');
  qOut = document.getElementById('q-out');
  qConst = document.getElementById('q-const');
  qEx = document.getElementById('q-ex');
  resp = document.getElementById('resp');
  interimEl = document.getElementById('interim');
  finalEl = document.getElementById('final');
  typedEl = document.getElementById('typed');
  timerEl = document.getElementById('timer');

  // Verify all elements are found
  console.log('Elements initialized:', {
    questionCard: !!questionCard,
    typedEl: !!typedEl,
    interimEl: !!interimEl,
    finalEl: !!finalEl,
    timerEl: !!timerEl
  });

  if (!typedEl) {
    console.error('CRITICAL: typedEl (textarea) not found!');
    alert('Error: Textarea element not found. Please refresh the page.');
  }

  document.getElementById('btn-new-q').addEventListener('click', fetchQuestion);
  document.getElementById('btn-start-mic').addEventListener('click', startMicWindow);
  document.getElementById('btn-stop-mic').addEventListener('click', stopAll); // Stop both speech and mic
  document.getElementById('btn-submit-text').addEventListener('click', async () => {
    if (typedEl) {
      await submitAnswer(typedEl.value.trim());
    }
  });
  
  console.log('Event listeners attached');
});



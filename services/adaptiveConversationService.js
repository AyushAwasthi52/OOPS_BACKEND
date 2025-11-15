const geminiService = require('./geminiService');

// Simple in-memory store keyed by a session key (from cookie or fallback)
const sessionStore = new Map();

function nowMs() {
  return Date.now();
}

function getNextDifficulty(current) {
  const order = ['easy', 'medium', 'hard'];
  const idx = order.indexOf(current);
  return idx < order.length - 1 ? order[idx + 1] : current;
}

function getPrevDifficulty(current) {
  const order = ['easy', 'medium', 'hard'];
  const idx = order.indexOf(current);
  return idx > 0 ? order[idx - 1] : current;
}

class AdaptiveConversationService {
  constructor() {
    this.topicsPool = ['data_structures', 'algorithms', 'system_design', 'database', 'networking', 'security'];
    this.randomizeTopic = true; // pick topic randomly from pool
    this.defaultDifficulty = 'easy'; // used as fallback only
    this.randomizeInitialDifficulty = true; // pick starting difficulty randomly
    this.responseTimeoutMs = 40 * 1000; // 40 seconds
    this.interviewDurationMs = 10 * 60 * 1000; // 10 minutes
    this.introQuestion = {
      question_id: 'intro',
      question_text: 'Before we dive into technical questions, please introduce yourself and highlight the experience you are most proud of.',
      expected_key_points: [],
      difficulty: 'intro'
    };
    this.defaultClosingMessage = 'That concludes our 10-minute interview. Thank you for your time—we will follow up with next steps soon.';
  }

  getOrCreateSession(sessionKey) {
    if (!sessionStore.has(sessionKey)) {
      sessionStore.set(sessionKey, {
        topic: this.getRandomTopic(),
        difficulty: this.randomizeInitialDifficulty ? this.getRandomDifficulty() : this.defaultDifficulty,
        lastQuestion: null, // { question_id, question_text, expected_key_points, asked_at }
        history: [], // { question_id, answer, evaluation, difficulty }
        introAsked: false,
        phase: 'intro', // intro | technical | ended
        started_at: nowMs(),
        finalMessage: null,
        lastFocusHint: null
      });
    }
    return sessionStore.get(sessionKey);
  }

  async handleTurn(sessionKey, latestAnswerText) {
    const session = this.getOrCreateSession(sessionKey);

    if (session.phase === 'ended') {
      return {
        evaluation: 'This interview session has already ended. Please start a new session if you need another attempt.',
        next_question: this.createClosingPayload(session.finalMessage || this.defaultClosingMessage)
      };
    }

    if (session.phase === 'intro') {
      if (!session.introAsked || !session.lastQuestion) {
        return this.deliverIntroQuestion(session);
      }
      return this.handleIntroResponse(session, latestAnswerText);
    }

    // If this is the very first turn (no previous question), generate first easy question
    if (!session.lastQuestion) {
      const first = await this.generateInitialTechnicalQuestion(session);
      return {
        evaluation: null,
        next_question: this.toQuestionPayload(first),
      };
    }

    const previousQuestionText = session.lastQuestion.question_text || '';

    // Evaluate previous answer (or timeout)
    let evaluationText = '';
    let performanceBand = 'partial'; // correct | partial | incorrect | timeout
    const timedOut = nowMs() - session.lastQuestion.asked_at > this.responseTimeoutMs;
    let scoreBreakdown = {
      correctness: 0.5,
      clarity: 0.5,
      confidence: 0.5
    };

    if (timedOut && (!latestAnswerText || latestAnswerText.trim() === '')) {
      evaluationText = 'No Response — timed out after 90 seconds.';
      performanceBand = 'timeout';
    } else if (!latestAnswerText || latestAnswerText.trim() === '') {
      evaluationText = 'No Response — empty submission.';
      performanceBand = 'incorrect';
    } else {
      const evalRes = await geminiService.evaluateAnswer(
        session.lastQuestion.question_text,
        latestAnswerText,
        session.lastQuestion.expected_key_points || []
      );
      scoreBreakdown = {
        correctness: evalRes.correctness,
        clarity: evalRes.clarity,
        confidence: evalRes.confidence
      };

      // Map numeric scores to bands
      const avg = (evalRes.correctness + evalRes.clarity + evalRes.confidence) / 3;
      if (avg >= 0.7) performanceBand = 'correct';
      else if (avg < 0.5) performanceBand = 'incorrect';
      else performanceBand = 'partial';
      evaluationText = evalRes.feedback || 'Evaluated.';
    }

    // Adjust difficulty
    let nextDifficulty = session.difficulty;
    if (performanceBand === 'correct') nextDifficulty = getNextDifficulty(session.difficulty);
    else if (performanceBand === 'incorrect' || performanceBand === 'timeout') nextDifficulty = getPrevDifficulty(session.difficulty);

    // Ensure difficulty changes every turn (never same as previous)
    if (nextDifficulty === session.difficulty) {
      nextDifficulty = this.getAlternateDifficulty(session.difficulty);
    }

    // Compute coverage of expected key points
    const expected = (session.lastQuestion.expected_key_points || []).map(k => String(k));
    const answerLower = (latestAnswerText || '').toLowerCase();
    const covered = expected.filter(k => {
      const firstToken = k.split(/\s+/)[0]?.toLowerCase();
      return firstToken && answerLower.includes(firstToken);
    });
    const missed = expected.filter(k => !covered.includes(k));

    // Persist turn in history
    session.history.push({
      question_id: session.lastQuestion.question_id,
      question_text: session.lastQuestion.question_text,
      answer: latestAnswerText || '',
      evaluation: evaluationText,
      difficulty: session.difficulty,
      performance: performanceBand,
      covered_key_points: covered,
      missed_key_points: missed,
      responded_at: nowMs()
    });

    if (this.isInterviewExpired(session)) {
      return this.buildClosingResponse(session, evaluationText);
    }

    // Upgrade feedback with contextual Gemini summary when available
    try {
      const contextual = await geminiService.generateContextualFeedback({
        question: session.lastQuestion.question_text,
        answer: latestAnswerText || '',
        evaluation: scoreBreakdown,
        coveredKeyPoints: covered,
        missedKeyPoints: missed,
        difficulty: session.difficulty,
        domain: session.lastQuestion.domain || session.topic
      });
      if (contextual) {
        evaluationText = contextual;
        session.history[session.history.length - 1].evaluation = contextual;
      }
    } catch (err) {
      console.warn('Contextual feedback generation failed:', err?.message || err);
    }

    // Generate next question
    const previousResponse = {
      performance_band: performanceBand,
      performance_score: performanceBand === 'correct' ? 0.8 : performanceBand === 'partial' ? 0.6 : 0.3,
      previous_answer: latestAnswerText || '',
      covered_key_points: covered,
      missed_key_points: missed,
      previous_question: previousQuestionText,
      recent_questions: this.getRecentQuestionTexts(session, previousQuestionText)
    };

    const planContext = {
      current_topic: session.topic,
      current_difficulty: session.difficulty,
      performance_band: performanceBand,
      scores: scoreBreakdown,
      covered_key_points: covered,
      missed_key_points: missed,
      history: session.history.slice(-3).map(entry => ({
        question_id: entry.question_id,
        question_text: entry.question_text,
        difficulty: entry.difficulty,
        performance: entry.performance,
        evaluation: entry.evaluation
      })),
      domain: session.lastQuestion.domain || session.topic
    };

    let plan = null;
    try {
      plan = await geminiService.suggestNextQuestionPlan(planContext);
    } catch (err) {
      console.warn('Next question plan suggestion failed:', err?.message || err);
    }

    if (plan?.topic) {
      session.topic = plan.topic;
    } else if (this.randomizeTopic) {
      session.topic = this.getRandomTopic();
    }

    if (plan?.difficulty && ['easy','medium','hard'].includes(plan.difficulty)) {
      nextDifficulty = plan.difficulty;
    }

    session.lastFocusHint = plan?.focus_hint || plan?.reason || null;
    previousResponse.focus_hint = session.lastFocusHint;

    let nextQuestion = await geminiService.generateAdaptiveQuestion(
      session.topic,
      nextDifficulty,
      previousResponse
    );

    let attempts = 0;
    const maxAttempts = 2;
    while (
      attempts < maxAttempts &&
      this.isSameQuestionText(previousQuestionText, nextQuestion.question_text)
    ) {
      attempts += 1;
      previousResponse.force_variation = true;
      previousResponse.variation_note = `Previous question "${previousQuestionText}" was repeated. Provide a substantially different prompt. Attempt ${attempts + 1}.`;
      nextQuestion = await geminiService.generateAdaptiveQuestion(
        session.topic,
        nextDifficulty,
        previousResponse
      );
    }

    session.difficulty = nextQuestion.difficulty || nextDifficulty;
    session.lastQuestion = {
      question_id: nextQuestion.question_id,
      question_text: nextQuestion.question_text,
      expected_key_points: nextQuestion.expected_key_points || [],
      asked_at: nowMs(),
      domain: nextQuestion.domain
    };

    return {
      evaluation: evaluationText,
      next_question: this.toQuestionPayload(nextQuestion, {
        focus_hint: session.lastFocusHint || nextQuestion.reasoning || ''
      })
    };
  }

  toQuestionPayload(q, extras = {}) {
    // Transform to required structure
    const base = {
      problem: q.question_text,
      input_format: 'As per problem statement. Provide necessary inputs described in the problem.',
      output_format: 'Return/print output exactly as specified in the problem.',
      constraints: 'Follow typical coding constraints for the topic unless specified otherwise.',
      example: 'Example I/O will be provided when relevant by the generator.',
      difficulty: (q.difficulty || this.defaultDifficulty).charAt(0).toUpperCase() + (q.difficulty || this.defaultDifficulty).slice(1)
    };

    return { ...base, ...extras };
  }

  getRandomTopic() {
    if (!Array.isArray(this.topicsPool) || this.topicsPool.length === 0) {
      return 'data_structures';
    }
    const index = Math.floor(Math.random() * this.topicsPool.length);
    return this.topicsPool[index];
  }

  getRandomDifficulty() {
    const order = ['easy', 'medium', 'hard'];
    const index = Math.floor(Math.random() * order.length);
    return order[index];
  }

  getAlternateDifficulty(current) {
    const order = ['easy', 'medium', 'hard'];
    const others = order.filter(d => d !== current);
    // pick randomly among the two others
    const idx = Math.floor(Math.random() * others.length);
    return others[idx];
  }

  isInterviewExpired(session) {
    return nowMs() - session.started_at >= this.interviewDurationMs;
  }

  getRecentQuestionTexts(session, currentQuestionText) {
    const seen = new Set();
    const list = [];
    const add = (text) => {
      const normalized = this.normalizeQuestionText(text);
      if (normalized && !seen.has(normalized)) {
        seen.add(normalized);
        list.push(text);
      }
    };
    if (currentQuestionText) {
      add(currentQuestionText);
    }
    const recentHistory = session.history.slice(-3);
    for (const entry of recentHistory) {
      if (entry?.question_text) {
        add(entry.question_text);
      }
    }
    return list;
  }

  normalizeQuestionText(text) {
    if (!text || typeof text !== 'string') return '';
    return text.replace(/\s+/g, ' ').trim().toLowerCase();
  }

  isSameQuestionText(a, b) {
    if (!a || !b) return false;
    return this.normalizeQuestionText(a) === this.normalizeQuestionText(b);
  }

  async generateInitialTechnicalQuestion(session) {
    const startDifficulty = this.randomizeInitialDifficulty ? this.getRandomDifficulty() : this.defaultDifficulty;
    const first = await geminiService.generateAdaptiveQuestion(session.topic, startDifficulty, null);
    session.difficulty = first.difficulty || startDifficulty;
    session.lastQuestion = {
      question_id: first.question_id,
      question_text: first.question_text,
      expected_key_points: first.expected_key_points || [],
      asked_at: nowMs(),
      domain: first.domain
    };
    return first;
  }

  deliverIntroQuestion(session) {
    session.lastQuestion = {
      question_id: this.introQuestion.question_id,
      question_text: this.introQuestion.question_text,
      expected_key_points: this.introQuestion.expected_key_points,
      asked_at: nowMs(),
      domain: 'intro'
    };
    session.introAsked = true;

    return {
      evaluation: null,
      next_question: this.toQuestionPayload(this.introQuestion, { isIntro: true })
    };
  }

  async handleIntroResponse(session, latestAnswerText) {
    const trimmed = (latestAnswerText || '').trim();
    const evaluationText = trimmed
      ? 'Thanks for the introduction. Let’s move into the technical portion.'
      : 'No worries if you kept it brief—let’s move into the technical portion.';

    session.history.push({
      question_id: this.introQuestion.question_id,
      question_text: this.introQuestion.question_text,
      answer: latestAnswerText || '',
      evaluation: evaluationText,
      difficulty: 'intro',
      performance: 'acknowledged',
      covered_key_points: [],
      missed_key_points: [],
      responded_at: nowMs()
    });

    session.phase = 'technical';
    session.lastQuestion = null;

    if (this.isInterviewExpired(session)) {
      return this.buildClosingResponse(session, evaluationText);
    }

    const first = await this.generateInitialTechnicalQuestion(session);
    return {
      evaluation: evaluationText,
      next_question: this.toQuestionPayload(first)
    };
  }

  buildClosingResponse(session, evaluationText) {
    session.phase = 'ended';
    session.lastQuestion = null;
    session.finalMessage = session.finalMessage || this.defaultClosingMessage;

    return {
      evaluation: evaluationText,
      next_question: this.createClosingPayload(session.finalMessage)
    };
  }

  createClosingPayload(message) {
    return {
      problem: message,
      input_format: '',
      output_format: '',
      constraints: '',
      example: '',
      difficulty: 'Completed',
      isClosing: true
    };
  }
}

module.exports = new AdaptiveConversationService();



# OOPS Backend - Interview Flow System

A comprehensive backend system for conducting adaptive technical interviews with AI-powered evaluation and dynamic difficulty adjustment.

## Features

- **Adaptive Interview Flow**: Dynamic difficulty adjustment based on candidate performance
- **AI-Powered Evaluation**: Integration with Google Gemini API for answer assessment
- **Session Management**: Complete interview session lifecycle management
- **Comprehensive Reporting**: Detailed performance analytics and feedback generation
- **Scalable Architecture**: Built with Node.js, Express, and MongoDB

## API Endpoints

### Core Interview Flow

#### 1. Start Interview Session
```http
POST /api/start-session
Content-Type: application/json

{
  "candidate_id": "C123",
  "name": "John Doe",
  "email": "john@example.com",
  "experience_level": "mid",
  "preferred_domains": ["data_structures", "algorithms"]
}
```

**Response:**
```json
{
  "success": true,
  "message": "Session started successfully",
  "data": {
    "session_id": "S101",
    "candidate_id": "C123",
    "first_question": {
      "question_id": "DS001",
      "question_text": "Explain the difference between a stack and a queue...",
      "domain": "data_structures",
      "difficulty": "medium",
      "expected_key_points": ["LIFO vs FIFO", "use cases", "operations"]
    },
    "session_info": {
      "current_domain": "data_structures",
      "current_difficulty": "medium",
      "started_at": "2024-01-15T10:00:00Z",
      "timeout_at": "2024-01-15T11:00:00Z"
    }
  }
}
```

#### 2. Evaluate Answer
```http
POST /api/evaluate-answer
Content-Type: application/json

{
  "session_id": "S101",
  "question_id": "DS001",
  "answer": "A stack follows LIFO principle...",
  "candidate_id": "C123"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Answer evaluated successfully",
  "data": {
    "evaluation": {
      "correctness": 0.8,
      "clarity": 0.7,
      "confidence": 0.6,
      "feedback": "Good understanding of basic concepts..."
    },
    "next_question": {
      "question_id": "DS002",
      "question_text": "How would you implement a hash table?",
      "domain": "data_structures",
      "difficulty": "medium",
      "expected_key_points": ["hash function", "collision handling", "load factor"]
    },
    "session_info": {
      "current_domain": "data_structures",
      "current_difficulty": "medium",
      "questions_answered": 1,
      "total_questions": 2
    },
    "adaptive_reasoning": "Maintained difficulty based on good performance"
  }
}
```

#### 3. End Session
```http
POST /api/end-session
Content-Type: application/json

{
  "session_id": "S101"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Session ended successfully",
  "data": {
    "session_summary": {
      "session_id": "S101",
      "total_questions": 5,
      "questions_answered": 5,
      "session_score": 0.75,
      "duration_minutes": 45
    },
    "report": {
      "session_summary": {
        "overall_score": 0.75,
        "performance_level": "Good"
      },
      "strengths": [
        {
          "category": "Domain Expertise",
          "description": "Strong performance in data structures",
          "score": 0.8
        }
      ],
      "weaknesses": [
        {
          "category": "Communication",
          "description": "Explanation clarity needs improvement",
          "score": 0.6,
          "priority": "medium"
        }
      ],
      "recommendations": [
        {
          "category": "Communication",
          "priority": "medium",
          "action": "Practice explaining technical concepts clearly",
          "resources": ["Technical writing courses", "Presentation practice"]
        }
      ]
    }
  }
}
```

### Additional Endpoints

#### Get Session Details
```http
GET /api/session/:session_id
```

#### Get Candidate Sessions
```http
GET /api/candidate/:candidate_id/sessions?limit=10&offset=0
```

#### Health Check
```http
GET /health
```

## Setup Instructions

### Prerequisites

- Node.js (v16 or higher)
- MongoDB (v4.4 or higher)
- Google Gemini API key

### Installation

1. **Clone the repository**
```bash
git clone <repository-url>
cd oops-backend
```

2. **Install dependencies**
```bash
npm install
```

3. **Environment Configuration**
```bash
cp env.example .env
```

Edit `.env` file with your configuration:
```env
# Database Configuration
MONGODB_URI=mongodb://localhost:27017/oops_interview
DB_NAME=oops_interview

# Server Configuration
PORT=3000
NODE_ENV=development

# Gemini API Configuration
GEMINI_API_KEY=your_gemini_api_key_here
GEMINI_API_URL=https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent

# Session Configuration
SESSION_TIMEOUT=3600000
MAX_QUESTIONS_PER_SESSION=20

# Rate Limiting
RATE_LIMIT_WINDOW_MS=900000
RATE_LIMIT_MAX_REQUESTS=100

# CORS Configuration
CORS_ORIGIN=http://localhost:3000
```

4. **Seed the database with sample questions**
```bash
npm run seed
```

5. **Start the server**
```bash
# Development mode
npm run dev

# Production mode
npm start
```

### Database Seeding

The system comes with pre-loaded sample questions across different domains and difficulty levels. To seed the database:

```bash
node scripts/seedQuestions.js
```

## Architecture

### Core Components

1. **Session Management**: Handles interview session lifecycle
2. **Adaptive Engine**: Adjusts difficulty and domain based on performance
3. **Gemini Integration**: AI-powered answer evaluation
4. **Report Generation**: Comprehensive performance analytics
5. **Data Models**: MongoDB schemas for candidates, sessions, questions, and evaluations

### Data Flow

1. **Session Start**: Create session, select first question
2. **Answer Evaluation**: Process candidate response with AI
3. **Adaptive Adjustment**: Determine next question parameters
4. **Session Continuation**: Present next question or end session
5. **Report Generation**: Generate comprehensive feedback report

### Adaptive Algorithm

The system uses a hybrid approach combining:
- **Rule-based logic**: For basic difficulty adjustments
- **AI suggestions**: Gemini API for intelligent parameter selection
- **Performance tracking**: Historical performance analysis
- **Domain progression**: Logical advancement through technical areas

## Configuration

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `MONGODB_URI` | MongoDB connection string | `mongodb://localhost:27017/oops_interview` |
| `PORT` | Server port | `3000` |
| `GEMINI_API_KEY` | Google Gemini API key | Required |
| `SESSION_TIMEOUT` | Session timeout in milliseconds | `3600000` (1 hour) |
| `MAX_QUESTIONS_PER_SESSION` | Maximum questions per session | `20` |
| `RATE_LIMIT_MAX_REQUESTS` | Rate limit per window | `100` |

### Difficulty Thresholds

- **Increase Difficulty**: Score ≥ 0.7
- **Maintain Difficulty**: Score 0.5 - 0.7
- **Decrease Difficulty**: Score < 0.5

## Error Handling

The system includes comprehensive error handling for:
- Validation errors
- Database connection issues
- API rate limiting
- Gemini API failures
- Session timeouts
- Invalid session states

## Security Features

- **Helmet.js**: Security headers
- **CORS**: Cross-origin resource sharing
- **Rate Limiting**: Request throttling
- **Input Validation**: Joi schema validation
- **Error Sanitization**: Safe error responses

## Monitoring and Logging

- Request logging with timestamps
- Error tracking and reporting
- Performance metrics
- Health check endpoint

## Development

### Scripts

```bash
npm start          # Start production server
npm run dev        # Start development server with nodemon
npm test           # Run tests
npm run seed       # Seed database with sample data
```

## Deploying a persistent backend (Render/Railway)

1. Push this repo to GitHub.
2. Create a new Web Service on Render or Railway:
   - Build command: `npm install`
   - Start command: `node server.js`
   - Health check path: `/health`
3. Configure environment variables:
   - `MONGODB_URI` = your MongoDB Atlas connection string
   - `GEMINI_API_KEY` = your Gemini API key
   - `CORS_ORIGIN` = your frontend origin (e.g., `https://yourapp.com` or `http://localhost:3000`)
   - Optional: `GEMINI_API_URL`
4. Ensure MongoDB Atlas Network Access allows your host IPs (or `0.0.0.0/0` for testing).
5. Deploy. Your endpoints will be available at `https://<your-service>/api/*`.

### Project Structure

```
oops-backend/
├── config/           # Database configuration
├── controllers/      # API route handlers
├── middleware/       # Express middleware
├── models/          # MongoDB schemas
├── routes/          # API route definitions
├── services/        # Business logic services
├── scripts/         # Database seeding scripts
├── server.js        # Main application file
└── package.json     # Dependencies and scripts
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

MIT License - see LICENSE file for details

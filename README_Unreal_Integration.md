# Unreal Engine 5 AI Interview System Integration Guide

This guide explains how to integrate your existing backend STT/TTS system into Unreal Engine 5 for an AI interview experience.

## Overview

The system consists of:
- **InterviewComponent**: C++ component that manages the interview session
- **HTTP Communication**: Connects to your backend APIs
- **Audio Recording**: Captures microphone input for STT
- **Audio Playback**: Plays TTS responses with lipsync support
- **State Management**: Handles the interview flow automatically

## Setup Instructions

### 1. Add Required Modules

In your `YourGame.Build.cs` file, add these modules:

```cpp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "HTTP",
    "Json",
    "JsonUtilities",
    "AudioMixer",
    "AudioPlatformConfiguration"
});
```

### 2. Copy Files

1. Copy `InterviewComponent.h` and `InterviewComponent.cpp` to your project's `Source/YourGame/` directory
2. Replace `YOURGAME_API` with your actual game module name (e.g., `MYPROJECT_API`)

### 3. Compile

1. Right-click your `.uproject` file
2. Select "Generate Visual Studio project files"
3. Open the solution and compile

### 4. Configure Backend URLs

In Unreal Editor:
1. Select your interviewer character (MetaHuman/NPC)
2. Add the `InterviewComponent` component
3. In the component details, set:
   - **Backend Base URL**: Your API URL (e.g., `http://localhost:3000`)
   - **Start Session Endpoint**: `/api/adaptive`
   - **Submit Answer Endpoint**: `/api/adaptive`

## Usage

### Basic Setup

1. **Attach Component**: Add `InterviewComponent` to your interviewer character
2. **Configure Settings**: Set interview duration, pause times, etc.
3. **Start Interview**: Call `StartInterview()` when player enters the room

### Blueprint Integration

The component exposes several Blueprint-callable functions and events:

**Functions:**
- `StartInterview()` - Begin the interview session
- `StopInterview()` - End the interview
- `RequestNextQuestion()` - Manually request next question

**Events:**
- `OnInterviewStateChanged` - Fired when state changes
- `OnQuestionReceived` - Fired when a question is received
- `OnFeedbackReceived` - Fired when feedback is received
- `OnInterviewEnded` - Fired when interview ends

### Example Blueprint Setup

1. Create a Blueprint based on your interviewer character
2. Add `InterviewComponent` to it
3. In Event BeginPlay or a trigger:
   - Call `StartInterview` on the component
4. Bind to events:
   - `OnQuestionReceived` → Update UI, trigger animations
   - `OnFeedbackReceived` → Show feedback to player
   - `OnInterviewEnded` → Handle interview completion

## Audio Recording Implementation

The current code has placeholder for audio recording. You need to implement one of:

### Option 1: Audio Capture Plugin (Recommended)
Use Unreal's Audio Capture system:
```cpp
#include "AudioCaptureComponent.h"

// In StartRecording():
UAudioCaptureComponent* CaptureComponent = NewObject<UAudioCaptureComponent>(this);
CaptureComponent->StartCapture();
// Capture audio samples into AudioRecordingBuffer
```

### Option 2: Third-Party Plugin
Use plugins like:
- **Voice Chat Plugin**
- **Audio Input Plugin**
- **Microphone Capture Plugin**

### Option 3: Platform-Specific APIs
Implement platform-specific microphone access (Windows, Mac, etc.)

## Text-to-Speech Integration

The component needs TTS conversion. Options:

### Option 1: Backend TTS (Recommended)
If your backend returns audio directly:
1. Modify `OnStartSessionResponse()` to handle audio data
2. Convert base64/URL to `USoundWave`
3. Play with `PlayAudioWithLipsync()`

### Option 2: Unreal TTS Plugin
Use Unreal's text-to-speech plugins:
- **Text to Speech Plugin**
- **Speech Synthesis Plugin**

### Option 3: Separate TTS API Call
Make additional HTTP request to your TTS endpoint:
```cpp
// In OnStartSessionResponse(), after getting question text:
FString TTSRequest = FString::Printf(TEXT("{\"text\":\"%s\"}"), *QuestionText);
MakeHTTPRequest(TTSEndpoint, "POST", TTSRequest, OnTTSResponse);
```

## Lipsync Integration

For MetaHuman lipsync, you have several options:

### Option 1: Audio Link (Real-time)
Use Unreal's Audio Link system for real-time lipsync:
1. Enable Audio Link plugin
2. Connect audio component to MetaHuman's Audio Link input
3. Configure lipsync settings in MetaHuman

### Option 2: Animation Curves
Generate animation curves from audio:
1. Use tools like **FaceFX** or **Live Link Face**
2. Generate curves for jaw, mouth, etc.
3. Apply to MetaHuman's animation blueprint

### Option 3: Animation Blueprint Events
Trigger animation events based on audio:
```cpp
// In PlayAudioWithLipsync():
if (UAnimInstance* AnimInstance = GetOwner()->GetMesh()->GetAnimInstance())
{
    AnimInstance->Montage_Play(TalkingMontage);
}
```

## State Flow

The interview follows this state machine:

1. **Idle** → `StartInterview()` called
2. **Starting** → HTTP request to start session
3. **WaitingForQuestion** → Waiting for backend response
4. **PlayingQuestion** → TTS audio playing
5. **RecordingAnswer** → Microphone recording
6. **ProcessingAnswer** → Sending audio to backend
7. **PlayingFeedback** → Playing feedback audio
8. **WaitingForQuestion** → Loop back to step 3
9. **Ended** → Interview complete

## Customization

### Adjust Timing
Modify these properties in the component:
- `MaxInterviewDuration` - Total interview time (default: 600s = 10 minutes)
- `PauseAfterQuestion` - Delay before recording starts
- `PauseAfterFeedback` - Delay before next question
- `MaxRecordingDuration` - Max answer length

### Add Scoring
1. Modify `ParseAnswerResponse()` to extract score from backend
2. Update `OnFeedbackReceived` delegate to include score
3. Use score in Blueprint events

### Add Question Types
Extend `ParseQuestionResponse()` to handle different question formats from your backend.

## Troubleshooting

### HTTP Requests Failing
- Check `BackendBaseURL` is correct
- Verify CORS settings on backend
- Check firewall/network settings

### Audio Not Playing
- Verify `AudioComponent` is initialized
- Check audio device settings
- Ensure `USoundWave` is valid

### Recording Not Working
- Implement actual audio capture (see Audio Recording section)
- Check microphone permissions
- Verify audio format compatibility

### Lipsync Not Working
- Enable Audio Link plugin
- Configure MetaHuman Audio Link settings
- Check animation blueprint connections

## Extension Points

### Add Visual Feedback
Bind to `OnFeedbackReceived` to show UI elements:
```cpp
OnFeedbackReceived.AddDynamic(this, &AInterviewRoom::ShowFeedbackUI);
```

### Add Question History
Store questions in an array:
```cpp
TArray<FString> QuestionHistory;
// In OnQuestionReceived:
QuestionHistory.Add(QuestionText);
```

### Add Session Analytics
Track metrics:
```cpp
float AverageAnswerTime;
int32 TotalQuestions;
float OverallScore;
```

## Next Steps

1. Implement audio recording (choose one of the options above)
2. Implement TTS conversion (integrate with your backend or plugin)
3. Set up lipsync (configure MetaHuman Audio Link)
4. Test the flow end-to-end
5. Add UI elements for feedback
6. Polish timing and pacing

## Support

For issues:
1. Check Unreal Engine logs (`Window → Developer Tools → Output Log`)
2. Enable HTTP request logging in `InterviewComponent.cpp`
3. Verify backend API responses match expected format
4. Test backend endpoints independently (Postman, curl, etc.)


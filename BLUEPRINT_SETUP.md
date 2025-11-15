# Blueprint Setup Guide for AI Interview System

This guide shows how to set up the interview system using Blueprints in Unreal Engine 5.

## Step 1: Create Interviewer Character Blueprint

1. **Create Blueprint**:
   - Right-click in Content Browser → Blueprint Class
   - Select your MetaHuman/NPC base class
   - Name it `BP_Interviewer`

2. **Add InterviewComponent**:
   - Open `BP_Interviewer`
   - Click "Add Component" → Search for "Interview Component"
   - Add it to the character

3. **Configure Component**:
   - Select the Interview Component
   - In Details panel, set:
     - **Backend Base URL**: `http://localhost:3000` (or your API URL)
     - **Start Session Endpoint**: `/api/adaptive`
     - **Submit Answer Endpoint**: `/api/adaptive`
     - **Max Interview Duration**: `600` (10 minutes)
     - **Pause After Question**: `1.0`
     - **Pause After Feedback**: `1.5`
     - **Max Recording Duration**: `90`

## Step 2: Set Up Interview Room (Optional)

1. **Create Room Blueprint**:
   - Create Blueprint Class based on `InterviewRoomActor` (if you compiled the C++ code)
   - Or create a regular Actor Blueprint

2. **Add Trigger Volume**:
   - Add a Box Collision component
   - Set collision to "Overlap" for Pawn
   - Size it to cover the interview area

3. **Set Up Events**:
   - In Event Graph:
     - **Event BeginPlay** → Get Interview Component from Interviewer
     - **On Component Begin Overlap** → Call `Start Interview` on component

## Step 3: Connect Events (Example Blueprint)

### On Question Received Event

```
Event: On Question Received (from Interview Component)
├─ Print String: "Question: " + Question Text
├─ Update UI Text: Set Question Text Widget
└─ Play Animation: Interviewer Talking Animation
```

### On Feedback Received Event

```
Event: On Feedback Received (from Interview Component)
├─ Print String: "Feedback: " + Feedback Text
├─ Update UI Text: Set Feedback Widget
├─ Update Score: Display Score
└─ Play Animation: Interviewer Nodding/Reacting
```

### On Interview State Changed Event

```
Event: On Interview State Changed
├─ Switch on Interview State
│  ├─ Case: Playing Question
│  │  └─ Show Question UI
│  ├─ Case: Recording Answer
│  │  └─ Show Recording Indicator
│  ├─ Case: Processing Answer
│  │  └─ Show Processing Spinner
│  └─ Case: Ended
│     └─ Show Completion Screen
```

## Step 4: MetaHuman Lipsync Setup

1. **Enable Audio Link**:
   - Edit → Plugins → Search "Audio Link"
   - Enable the plugin
   - Restart Unreal Editor

2. **Configure MetaHuman**:
   - Select your MetaHuman in the level
   - In Details panel, find "Audio Link" section
   - Enable "Use Audio Link"
   - Set Audio Component reference to Interview Component's Audio Component

3. **Animation Blueprint** (Alternative):
   - Create/Edit MetaHuman's Animation Blueprint
   - Add Audio Link node
   - Connect to facial animation curves
   - Blend with talking animation

## Step 5: Audio Recording Setup

Since audio recording requires platform-specific implementation, here are Blueprint workarounds:

### Option A: Use Voice Chat Plugin
1. Enable Voice Chat plugin
2. In Blueprint:
   - Start Voice Chat Recording
   - Get Voice Chat Audio Data
   - Convert to array for Interview Component

### Option B: Use Third-Party Plugin
1. Install microphone capture plugin
2. Follow plugin's Blueprint API
3. Connect to Interview Component's recording functions

### Option C: Manual Implementation
Create a custom Blueprint function that:
1. Captures audio using platform APIs
2. Converts to format expected by backend
3. Calls Interview Component's submit function

## Step 6: Testing

1. **Place Interviewer**:
   - Drag `BP_Interviewer` into level
   - Position where you want the interview

2. **Place Room Actor** (if using):
   - Drag `BP_InterviewRoom` into level
   - Set Interviewer Character reference
   - Position trigger volume

3. **Test Flow**:
   - Press Play
   - Walk into trigger volume (or manually call Start Interview)
   - Verify:
     - Interview starts
     - Question is received
     - Audio plays
     - Recording starts
     - Answer is submitted
     - Feedback is received
     - Next question appears

## Troubleshooting Blueprints

### Component Not Found
- Ensure Interview Component is added to character
- Check component is not disabled
- Verify C++ code compiled successfully

### Events Not Firing
- Check event bindings in Blueprint
- Verify component reference is valid
- Check Unreal logs for errors

### Audio Not Playing
- Verify Audio Component is set up
- Check sound wave is valid
- Test audio device settings

### Lipsync Not Working
- Enable Audio Link plugin
- Check MetaHuman Audio Link settings
- Verify audio component connection

## Advanced Blueprint Examples

### Custom UI Integration

```
Event: On Question Received
├─ Create Widget: Question UI Widget
├─ Set Text: Question Text
├─ Add to Viewport
└─ Play Animation: Fade In
```

### Score Tracking

```
Event: On Feedback Received
├─ Get Score from Feedback
├─ Add to Score Array
├─ Calculate Average Score
└─ Update Score Display Widget
```

### Interview Analytics

```
Event: On Interview Ended
├─ Get Questions Answered (from component)
├─ Get Interview Duration (from component)
├─ Save to Save Game
└─ Show Analytics Screen
```

## Next Steps

1. Implement audio recording (choose method above)
2. Set up UI widgets for questions/feedback
3. Configure MetaHuman lipsync
4. Add visual feedback (recording indicator, etc.)
5. Test end-to-end flow
6. Polish timing and animations


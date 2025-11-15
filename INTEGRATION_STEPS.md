# Step-by-Step Integration Guide for Existing UE5 Project

This guide will walk you through adding the AI Interview system to your existing Unreal Engine 5 project.

## Prerequisites

- Unreal Engine 5 project (already created)
- Visual Studio 2019/2022 (for C++ compilation)
- Basic knowledge of Unreal Editor

## Step 1: Locate Your Project's Source Directory

1. **Find your project folder** (e.g., `D:\Projects\MyGame\`)
2. **Navigate to the Source folder**:
   - Your project structure should look like:
   ```
   MyGame/
   ├── Content/
   ├── Config/
   ├── Source/
   │   └── MyGame/
   │       ├── MyGame.Build.cs
   │       ├── MyGame.cpp
   │       ├── MyGame.h
   │       └── (other files)
   └── MyGame.uproject
   ```

## Step 2: Copy the C++ Files

1. **Copy these files** to your `Source/MyGame/` folder:
   - `InterviewComponent.h`
   - `InterviewComponent.cpp`
   - `InterviewRoomActor.h`
   - `InterviewRoomActor.cpp`
   - `AudioRecorderComponent.h` (optional, for future use)

2. **Important**: Replace `YOURGAME_API` with your actual module name:
   - Open each `.h` file
   - Find `YOURGAME_API` (use Find & Replace)
   - Replace with your module name (e.g., `MYGAME_API` or `MYPROJECT_API`)
   - Your module name is in your `MyGame.h` file (look for `class MYGAME_API`)

## Step 3: Update Your Build.cs File

1. **Open** `Source/MyGame/MyGame.Build.cs`

2. **Add these modules** to `PublicDependencyModuleNames` (if not already present):
   ```csharp
   PublicDependencyModuleNames.AddRange(new string[] {
       "Core",
       "CoreUObject",   
       "Engine",
       "HTTP",           // Add this
       "Json",           // Add this
       "JsonUtilities",  // Add this
       "AudioMixer",     // Add this (for audio)
       "AudioPlatformConfiguration"  // Add this (for audio)
   });
   ```

3. **Your Build.cs should look something like this**:
   ```csharp
   using UnrealBuildTool;

   public class MyGame : ModuleRules
   {
       public MyGame(ReadOnlyTargetRules Target) : base(Target)
       {
           PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

           PublicDependencyModuleNames.AddRange(new string[] {
               "Core",
               "CoreUObject",
               "Engine",
               "InputCore",
               "HeadMountedDisplay",
               "HTTP",                    // Added
               "Json",                    // Added
               "JsonUtilities",           // Added
               "AudioMixer",              // Added
               "AudioPlatformConfiguration" // Added
           });
       }
   }
   ```

4. **Save** the file

## Step 4: Generate Project Files

1. **Close Unreal Editor** (if open)

2. **Right-click** on your `.uproject` file

3. **Select** "Generate Visual Studio project files"
   - This will update your Visual Studio solution with the new files

4. **Wait** for the generation to complete

## Step 5: Compile the Project

### Option A: Compile in Visual Studio

1. **Open** `MyGame.sln` in Visual Studio

2. **Set configuration**:
   - Solution Configuration: `Development Editor` (or `DebugGame Editor`)
   - Solution Platform: `Win64`

3. **Build**:
   - Right-click on your project name → "Build"
   - Or press `Ctrl+Shift+B`

4. **Wait** for compilation to finish
   - First compile may take a few minutes
   - Watch for errors in the Output window

### Option B: Compile in Unreal Editor

1. **Open** your project in Unreal Editor

2. If you see a prompt about missing modules, click **"Yes"** to compile

3. **Wait** for compilation to complete

## Step 6: Verify Compilation

1. **Check for errors**:
   - If compilation failed, check the Output log
   - Common issues:
     - Missing modules → Add to Build.cs
     - Wrong module name → Check `YOURGAME_API` replacement
     - Syntax errors → Check file encoding (should be UTF-8)

2. **If successful**, you should see:
   - "Build succeeded" message
   - No errors in Output window

## Step 7: Use in Unreal Editor

### Method 1: Add Component to Existing Character

1. **Open your interviewer character Blueprint** (or create one):
   - Content Browser → Right-click → Blueprint Class
   - Parent Class: Character (or your custom character)
   - Name: `BP_Interviewer`

2. **Open the Blueprint**

3. **Add Interview Component**:
   - Click "Add Component" button (top left)
   - Search for "Interview Component"
   - Click to add it

4. **Configure the component**:
   - Select the Interview Component in Components panel
   - In Details panel, set:
     - **Backend Base URL**: `http://localhost:3000` (your backend URL)
     - **Start Session Endpoint**: `/api/adaptive`
     - **Submit Answer Endpoint**: `/api/adaptive`
     - **Max Interview Duration**: `600` (10 minutes)

5. **Save** the Blueprint

### Method 2: Use Interview Room Actor

1. **Create Blueprint from C++ class**:
   - Content Browser → Right-click → Blueprint Class
   - In "Pick Parent Class" window, search for "Interview Room Actor"
   - Select it and create Blueprint named `BP_InterviewRoom`

2. **Open the Blueprint**

3. **Set up**:
   - Select the actor in Components
   - In Details, set **Interviewer Character** reference
   - Adjust **Trigger Volume** size as needed

4. **Place in level**:
   - Drag `BP_InterviewRoom` into your level
   - Position where you want the interview to start

## Step 8: Test the Integration

1. **Start your backend server** (if not running):
   ```bash
   npm start
   # or
   node server.js
   ```

2. **In Unreal Editor**:
   - Place your interviewer character in the level
   - Or place the Interview Room Actor

3. **Press Play**

4. **Test manually** (if using component directly):
   - In Blueprint, add Event BeginPlay
   - Call "Start Interview" on Interview Component
   - Check Output Log for messages

## Step 9: Connect Events (Blueprint Example)

1. **Open your interviewer Blueprint**

2. **In Event Graph**, add:

   ```
   Event BeginPlay
   └─ Get Interview Component (self)
      └─ Start Interview
   ```

3. **Bind to events**:

   ```
   Event BeginPlay
   └─ Get Interview Component
      ├─ Bind Event to On Question Received
      │  └─ Print String: "Question: " + Question Text
      ├─ Bind Event to On Feedback Received
      │  └─ Print String: "Feedback: " + Feedback Text
      └─ Bind Event to On Interview Ended
         └─ Print String: "Interview Complete"
   ```

## Common Issues and Solutions

### Issue: "Interview Component" not found in Add Component

**Solution**:
- Make sure you compiled successfully
- Restart Unreal Editor
- Check that files are in correct location (`Source/MyGame/`)

### Issue: Compilation errors about missing modules

**Solution**:
- Check `MyGame.Build.cs` has all required modules
- Regenerate project files
- Clean and rebuild solution

### Issue: "YOURGAME_API" errors

**Solution**:
- Replace all instances of `YOURGAME_API` with your module name
- Your module name is in `MyGame.h` (look for `class MYGAME_API`)

### Issue: HTTP requests failing

**Solution**:
- Check backend is running
- Verify `BackendBaseURL` is correct
- Check CORS settings on backend
- Test backend with Postman/curl first

### Issue: Component not visible in Blueprint

**Solution**:
- Ensure component is marked as `BlueprintSpawnableComponent` (it is)
- Check module name matches
- Recompile project

## Next Steps

1. **Implement Audio Recording**:
   - See `README_Unreal_Integration.md` for options
   - You may need a plugin for microphone capture

2. **Set up TTS**:
   - Integrate with your backend TTS endpoint
   - Or use Unreal's TTS plugins

3. **Configure Lipsync**:
   - Enable Audio Link plugin
   - Connect to MetaHuman

4. **Add UI**:
   - Create widgets for questions/feedback
   - Connect to component events

5. **Test End-to-End**:
   - Verify question → answer → feedback loop works
   - Check timing and pacing

## File Structure After Integration

Your project should now have:
```
MyGame/
├── Content/
├── Config/
├── Source/
│   └── MyGame/
│       ├── MyGame.Build.cs          (updated)
│       ├── MyGame.cpp
│       ├── MyGame.h
│       ├── InterviewComponent.h     (new)
│       ├── InterviewComponent.cpp   (new)
│       ├── InterviewRoomActor.h     (new)
│       ├── InterviewRoomActor.cpp   (new)
│       └── AudioRecorderComponent.h (new, optional)
└── MyGame.uproject
```

## Quick Reference

**Key Functions** (callable from Blueprint):
- `StartInterview()` - Begin interview session
- `StopInterview()` - End interview
- `RequestNextQuestion()` - Get next question

**Key Events** (bind in Blueprint):
- `OnInterviewStateChanged` - State machine updates
- `OnQuestionReceived` - New question available
- `OnFeedbackReceived` - Feedback after answer
- `OnInterviewEnded` - Interview complete

**Key Properties** (set in Details panel):
- `BackendBaseURL` - Your API URL
- `StartSessionEndpoint` - API endpoint
- `MaxInterviewDuration` - Interview length
- `PauseAfterQuestion` - Delay before recording
- `PauseAfterFeedback` - Delay before next question

## Need Help?

1. Check Unreal Engine logs: `Window → Developer Tools → Output Log`
2. Enable HTTP logging in `InterviewComponent.cpp`
3. Test backend endpoints independently
4. Verify all files are in correct locations
5. Ensure module name is consistent throughout


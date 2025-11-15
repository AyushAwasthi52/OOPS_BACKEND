# Quick Start - 5 Minute Setup

Fastest way to get the interview system working in your existing UE5 project.

## 1. Copy Files (2 minutes)

1. Copy these files to `Source/YourProjectName/`:
   - `InterviewComponent.h`
   - `InterviewComponent.cpp`
   - `InterviewRoomActor.h`
   - `InterviewRoomActor.cpp`

2. **Find and Replace** in all `.h` files:
   - Find: `YOURGAME_API`
   - Replace: `YOURPROJECTNAME_API` (use your actual project name)

## 2. Update Build.cs (1 minute)

Open `Source/YourProjectName/YourProjectName.Build.cs` and add to `PublicDependencyModuleNames`:
```csharp
"HTTP",
"Json",
"JsonUtilities",
"AudioMixer",
"AudioPlatformConfiguration"
```

## 3. Compile (2 minutes)

1. Right-click `.uproject` → "Generate Visual Studio project files"
2. Open `.sln` in Visual Studio
3. Build → Build Solution (Ctrl+Shift+B)
4. Wait for "Build succeeded"

## 4. Use in Editor (1 minute)

1. Open Unreal Editor
2. Create Blueprint: Right-click → Blueprint Class → Character
3. Name it `BP_Interviewer`
4. Open Blueprint → Add Component → "Interview Component"
5. Set Backend Base URL: `http://localhost:3000`
6. Save

## 5. Test

1. Start your backend: `npm start`
2. Place `BP_Interviewer` in level
3. In Blueprint Event Graph:
   - Event BeginPlay → Get Interview Component → Start Interview
4. Press Play
5. Check Output Log for "Question received" messages

## Done! ✅

Your interview system is now integrated. See `INTEGRATION_STEPS.md` for detailed setup and `README_Unreal_Integration.md` for advanced features.


// AudioRecorderComponent.cpp
// Implementation of audio recording component using Unreal's Audio Capture plugin

#include "AudioRecorderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

// Forward declare AudioCaptureComponent - user will need to include the correct header
// The header path depends on your Unreal version and Audio Capture plugin location
// Common paths: "AudioCaptureComponent.h", "Components/AudioCaptureComponent.h", "Audio/AudioCaptureComponent.h"
// TODO: Uncomment and adjust the include path below once you locate the AudioCaptureComponent header:
// #include "AudioCaptureComponent.h"

// Forward declaration for now to allow compilation
class UAudioCaptureComponent;

// Constructor
UAudioRecorderComponent::UAudioRecorderComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize member variables
	bIsRecording = false;
	CurrentSampleRate = 44100;
	CurrentNumChannels = 1;
	RecordingStartTime = 0.0f;
	AudioCaptureComponentRaw = nullptr;
	
	// Set this component to tick every frame (needed for duration tracking)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAudioRecorderComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("AudioRecorderComponent initialized"));
}

void UAudioRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stop recording if still active
	if (bIsRecording)
	{
		TArray<uint8> DummyBuffer;
		StopRecording(DummyBuffer);
	}
	
	// Clean up audio capture component
	if (AudioCaptureComponentRaw)
	{
		// TODO: Stop capture and unbind delegate when AudioCaptureComponent header is included
		// This requires including the actual AudioCaptureComponent header with full type definition
		AudioCaptureComponentRaw = nullptr;
	}
	
	// Clear audio buffer
	AudioBuffer.Empty();
	
	Super::EndPlay(EndPlayReason);
}

void UAudioRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Additional tick logic can be added here if needed
	// For example, auto-stop after maximum duration, silence detection, etc.
}

bool UAudioRecorderComponent::StartRecording(int32 SampleRate, int32 NumChannels)
{
	// Don't start if already recording
	if (bIsRecording)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already recording. Stop current recording first."));
		return false;
	}
	
	// Validate parameters
	if (SampleRate <= 0 || SampleRate > 192000)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid sample rate: %d"), SampleRate);
		return false;
	}
	
	if (NumChannels < 1 || NumChannels > 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid number of channels: %d (only 1 or 2 supported)"), NumChannels);
		return false;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Starting audio recording - Sample Rate: %d, Channels: %d"), SampleRate, NumChannels);
	
	// Store parameters
	CurrentSampleRate = SampleRate;
	CurrentNumChannels = NumChannels;
	RecordingStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	
	// Clear previous buffer
	AudioBuffer.Empty();
	
	// TODO: Create and configure audio capture component
	// This requires including the AudioCaptureComponent header to get the full type definition
	// Example:
	// #include "AudioCaptureComponent.h"  // Add this at the top of the file once you know the correct path
	// if (!AudioCaptureComponentRaw)
	// {
	//     UAudioCaptureComponent* CaptureComp = NewObject<UAudioCaptureComponent>(this);
	//     AudioCaptureComponentRaw = CaptureComp;
	//     // Configure and start capture
	// }
	
	if (!AudioCaptureComponentRaw)
	{
		UE_LOG(LogTemp, Error, TEXT("AudioCaptureComponent creation requires including the AudioCaptureComponent header. Please add the include and uncomment the creation code."));
		return false;
	}
	
	// TODO: Start capturing audio
	// The method name may vary based on your Audio Capture plugin version
	// Common method names: StartCapture(), BeginCapture(), Start(), ActivateCapture()
	// Uncomment and adjust once you have the full type:
	// UAudioCaptureComponent* CaptureComp = static_cast<UAudioCaptureComponent*>(AudioCaptureComponentRaw);
	// bool bStarted = CaptureComp->StartCapture();
	
	bool bStarted = false; // Set to true once the correct API call is configured above
	
	if (!bStarted)
	{
		UE_LOG(LogTemp, Error, TEXT("Audio capture not started. Please configure StartCapture() method call in AudioRecorderComponent.cpp line ~160. Check Audio Capture plugin API documentation."));
		return false;
	}
	
	// Set recording flag
	bIsRecording = true;
	
	UE_LOG(LogTemp, Log, TEXT("Audio recording started successfully."));
	
	return true;
}

bool UAudioRecorderComponent::StopRecording(TArray<uint8>& OutAudioData)
{
	// Don't stop if not recording
	if (!bIsRecording)
	{
		UE_LOG(LogTemp, Warning, TEXT("Not currently recording."));
		OutAudioData.Empty();
		return false;
	}
	
	UE_LOG(LogTemp, Log, TEXT("Stopping audio recording..."));
	
	// Set recording flag to false first to prevent new samples from being added
	bIsRecording = false;
	
	// TODO: Stop audio capture
	// The method name may vary based on your Audio Capture plugin version
	// Common method names: StopCapture(), EndCapture(), Stop(), DeactivateCapture()
	// Uncomment and adjust once you have the full type:
	if (AudioCaptureComponentRaw)
	{
		// UAudioCaptureComponent* CaptureComp = static_cast<UAudioCaptureComponent*>(AudioCaptureComponentRaw);
		// CaptureComp->StopCapture();
		// CaptureComp->EndCapture();
		// CaptureComp->Stop();
		UE_LOG(LogTemp, Warning, TEXT("StopCapture() method needs to be configured. Audio capture may still be active. Check AudioRecorderComponent.cpp line ~190."));
		UE_LOG(LogTemp, Log, TEXT("Audio capture stopped (if configured)."));
	}
	
	// Copy buffer to output
	OutAudioData = AudioBuffer;
	
	// Log recording statistics
	float Duration = GetRecordingDuration();
	int32 NumBytes = AudioBuffer.Num();
	int32 NumSamples = NumBytes / sizeof(int16);
	float EstimatedDurationFromSamples = NumSamples / (float)(CurrentSampleRate * CurrentNumChannels);
	
	UE_LOG(LogTemp, Log, TEXT("Recording stopped. Time-based duration: %.2f seconds, Sample-based duration: %.2f seconds, Bytes: %d, Samples: %d"), 
		Duration, EstimatedDurationFromSamples, NumBytes, NumSamples);
	
	// Clear internal buffer (optional - you may want to keep it for reuse)
	// AudioBuffer.Empty();
	
	return true;
}

float UAudioRecorderComponent::GetRecordingDuration() const
{
	if (!bIsRecording)
	{
		return 0.0f;
	}
	
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentTime - RecordingStartTime;
}

void UAudioRecorderComponent::OnAudioGenerated(const TArray<float>& AudioData)
{
	// This method is called by the AudioCaptureComponent when new audio samples are available
	// Using TArray instead of pointer to avoid UHT001 error
	
	if (!bIsRecording || AudioData.Num() <= 0)
	{
		return;
	}
	
	int32 NumSamples = AudioData.Num();
	
	// Calculate number of bytes needed (PCM16 format: 2 bytes per sample)
	int32 NumBytes = NumSamples * sizeof(int16);
	int32 CurrentBufferSize = AudioBuffer.Num();
	AudioBuffer.SetNumUninitialized(CurrentBufferSize + NumBytes);
	
	// Get pointer to the end of the buffer where we'll write new data
	int16* BufferPtr = reinterpret_cast<int16*>(AudioBuffer.GetData() + CurrentBufferSize);
	
	// Convert float samples [-1.0, 1.0] to int16 PCM format [-32768, 32767]
	// This is the standard PCM16 format used by most audio APIs
	for (int32 i = 0; i < NumSamples; ++i)
	{
		// Clamp to valid range to prevent overflow
		float ClampedSample = FMath::Clamp(AudioData[i], -1.0f, 1.0f);
		
		// Convert to int16
		// Using 32767.0f instead of 32768.0f to avoid potential overflow at +1.0
		BufferPtr[i] = static_cast<int16>(ClampedSample * 32767.0f);
	}
	
	// Optional: Log periodically for debugging (only in verbose mode)
	static float LastLogTime = 0.0f;
	float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - LastLogTime > 2.0f) // Log every 2 seconds to avoid spam
	{
		int32 TotalSamples = AudioBuffer.Num() / sizeof(int16);
		float EstimatedDuration = TotalSamples / (float)(CurrentSampleRate * CurrentNumChannels);
		UE_LOG(LogTemp, Verbose, TEXT("Audio captured: %d new samples, Total: %d samples (%.2f sec), Buffer: %d bytes"), 
			NumSamples, TotalSamples, EstimatedDuration, AudioBuffer.Num());
		LastLogTime = CurrentTime;
	}
}


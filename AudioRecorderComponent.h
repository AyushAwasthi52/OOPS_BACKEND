// AudioRecorderComponent.h
// Helper component for recording microphone audio using Unreal's Audio Capture plugin
// Requires: Audio Capture plugin enabled in project settings and "AudioCapture" module in Build.cs

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundWave.h"
#include "AudioRecorderComponent.generated.h"

// Forward declarations
class USoundWave;
class UAudioCaptureComponent;
class AActor;

/**
 * AudioRecorderComponent - Handles microphone audio recording using Unreal's Audio Capture plugin
 * 
 * This component uses Unreal Engine's Audio Capture plugin to record audio from the microphone.
 * Requires the Audio Capture plugin to be enabled in your project.
 * 
 * IMPORTANT: Replace YOURGAME_API with your actual module name (e.g., MYGAME_API)
 * This is typically found in your main module header file (e.g., MyGame.h).
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OOPS_API UAudioRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAudioRecorderComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Start recording audio from microphone
	 * @param SampleRate - Audio sample rate (default: 44100)
	 * @param NumChannels - Number of audio channels (1 = mono, 2 = stereo)
	 * @return True if recording started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio Recording")
	bool StartRecording(int32 SampleRate = 44100, int32 NumChannels = 1);

	/**
	 * Stop recording and get the audio data
	 * @param OutAudioData - Output array containing recorded audio samples
	 * @return True if recording was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio Recording")
	bool StopRecording(TArray<uint8>& OutAudioData);

	/**
	 * Check if currently recording
	 */
	UFUNCTION(BlueprintPure, Category = "Audio Recording")
	bool IsRecording() const { return bIsRecording; }

	/**
	 * Get current recording duration in seconds
	 */
	UFUNCTION(BlueprintPure, Category = "Audio Recording")
	float GetRecordingDuration() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Whether currently recording */
	bool bIsRecording;

	/** Audio sample rate */
	int32 CurrentSampleRate;

	/** Number of audio channels */
	int32 CurrentNumChannels;

	/** Recording start time */
	float RecordingStartTime;

	/** Audio data buffer */
	TArray<uint8> AudioBuffer;

	/** Audio capture component (from Audio Capture plugin)
	 * Note: Using void* pointer to avoid requiring full type definition
	 * This will be cast to the appropriate type in the implementation
	 * Not using UPROPERTY() because void* cannot be exposed to Blueprint
	 */
	void* AudioCaptureComponentRaw;

	/** Process audio samples (called by audio capture component)
	 * Note: Using TArray instead of pointer to avoid UHT001 error
	 */
	void OnAudioGenerated(const TArray<float>& AudioData);
};


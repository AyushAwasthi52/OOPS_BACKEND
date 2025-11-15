// InterviewComponent.h
// Core component for managing AI interview sessions in Unreal Engine 5
// Attach this component to your MetaHuman/NPC interviewer character

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Http.h"
#include "Sound/SoundWave.h"
#include "Animation/AnimInstance.h"
#include "InterviewComponent.generated.h"

// Forward declarations
class USoundWave;
class UAudioComponent;
class UAnimInstance;

/**
 * Enum to track the current state of the interview session
 * Used by the InterviewComponent to manage conversation flow
 */
UENUM(BlueprintType)
enum class EInterviewState : uint8
{
	Idle			UMETA(DisplayName = "Idle"),
	Starting		UMETA(DisplayName = "Starting Session"),
	WaitingForQuestion	UMETA(DisplayName = "Waiting For Question"),
	PlayingQuestion		UMETA(DisplayName = "Playing Question Audio"),
	RecordingAnswer		UMETA(DisplayName = "Recording Answer"),
	ProcessingAnswer	UMETA(DisplayName = "Processing Answer"),
	PlayingFeedback		UMETA(DisplayName = "Playing Feedback"),
	Ended			UMETA(DisplayName = "Interview Ended")
};

/**
 * Delegate for interview events - can be bound in Blueprints or C++
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInterviewStateChanged, EInterviewState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestionReceived, const FString&, QuestionText, const FString&, QuestionID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFeedbackReceived, const FString&, FeedbackText, float, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInterviewEnded);

/**
 * InterviewComponent - Main component for AI interview system
 * 
 * This component handles:
 * - HTTP communication with backend STT/TTS APIs
 * - Audio recording and playback
 * - Interview session state management
 * - Automatic conversation flow (question -> answer -> feedback loop)
 * 
 * Usage:
 * 1. Attach to your MetaHuman/NPC interviewer character
 * 2. Set backend API URLs in component properties
 * 3. Call StartInterview() when player enters the interview room
 * 4. Component automatically manages the conversation flow
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YOURGAME_API UInterviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInterviewComponent(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * Starts the interview session
	 * Call this when the player enters the interview room
	 */
	UFUNCTION(BlueprintCallable, Category = "Interview")
	void StartInterview();

	/**
	 * Stops the interview session
	 */
	UFUNCTION(BlueprintCallable, Category = "Interview")
	void StopInterview();

	/**
	 * Manually trigger next question (usually automatic)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interview")
	void RequestNextQuestion();

	// ========== BACKEND API CONFIGURATION ==========
	// Set these URLs to point to your backend endpoints
	
	/** Base URL for your backend API (e.g., "http://localhost:3000" or "https://your-api.com") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Configuration")
	FString BackendBaseURL = TEXT("http://localhost:3000");

	/** Endpoint for starting a new interview session (e.g., "/api/adaptive") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Configuration")
	FString StartSessionEndpoint = TEXT("/api/adaptive");

	/** Endpoint for submitting answers (e.g., "/api/adaptive") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Configuration")
	FString SubmitAnswerEndpoint = TEXT("/api/adaptive");

	/** Endpoint for TTS conversion (if separate, otherwise leave empty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Configuration")
	FString TTSEndpoint = TEXT("");

	// ========== INTERVIEW SETTINGS ==========
	
	/** Maximum interview duration in seconds (default: 600 = 10 minutes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interview Settings")
	float MaxInterviewDuration = 600.0f;

	/** Time to wait after question audio finishes before starting recording (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interview Settings")
	float PauseAfterQuestion = 1.0f;

	/** Time to wait after feedback before next question (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interview Settings")
	float PauseAfterFeedback = 1.5f;

	/** Maximum recording duration per answer (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interview Settings")
	float MaxRecordingDuration = 90.0f;

	/** Minimum silence duration before auto-submitting answer (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interview Settings")
	float SilenceThreshold = 2.0f;

	// ========== AUDIO SETTINGS ==========
	
	/** Audio component for playing TTS audio */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* AudioComponent;

	/** Sound wave for current question/feedback audio */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundWave* CurrentSoundWave;

	// ========== DELEGATES (Events) ==========
	// Bind to these in Blueprints or C++ to respond to interview events
	
	/** Called when interview state changes */
	UPROPERTY(BlueprintAssignable, Category = "Interview Events")
	FOnInterviewStateChanged OnInterviewStateChanged;

	/** Called when a new question is received from backend */
	UPROPERTY(BlueprintAssignable, Category = "Interview Events")
	FOnQuestionReceived OnQuestionReceived;

	/** Called when feedback is received after submitting an answer */
	UPROPERTY(BlueprintAssignable, Category = "Interview Events")
	FOnFeedbackReceived OnFeedbackReceived;

	/** Called when interview ends */
	UPROPERTY(BlueprintAssignable, Category = "Interview Events")
	FOnInterviewEnded OnInterviewEnded;

	// ========== STATE INFORMATION ==========
	
	/** Current interview state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interview State")
	EInterviewState CurrentState = EInterviewState::Idle;

	/** Time elapsed since interview started */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interview State")
	float InterviewElapsedTime = 0.0f;

	/** Number of questions answered */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interview State")
	int32 QuestionsAnswered = 0;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// ========== INTERNAL STATE ==========
	
	/** Session ID from backend (for maintaining conversation context) */
	FString SessionID;

	/** Current question text */
	FString CurrentQuestionText;

	/** Current question ID */
	FString CurrentQuestionID;

	/** Timer for interview duration */
	float InterviewTimer = 0.0f;

	/** Timer for pauses between states */
	float PauseTimer = 0.0f;

	/** Whether we're currently recording audio */
	bool bIsRecording = false;

	/** Audio recording buffer */
	TArray<uint8> AudioRecordingBuffer;

	// ========== HTTP REQUEST HANDLERS ==========
	
	/** Handles response from starting interview session */
	void OnStartSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/** Handles response from submitting answer */
	void OnSubmitAnswerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/** Handles response from TTS API (if using separate endpoint) */
	void OnTTSResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	// ========== INTERNAL METHODS ==========
	
	/** Changes the current interview state */
	void SetInterviewState(EInterviewState NewState);

	/** Processes the next step in the interview flow */
	void ProcessInterviewFlow(float DeltaTime);

	/** Starts recording audio from microphone */
	void StartRecording();

	/** Stops recording and processes the audio */
	void StopRecording();

	/** Converts audio buffer to base64 for API submission */
	FString AudioBufferToBase64(const TArray<uint8>& AudioData);

	/** Creates a sound wave from audio data for playback */
	USoundWave* CreateSoundWaveFromData(const TArray<uint8>& AudioData, int32 SampleRate = 44100);

	/** Plays audio and triggers lipsync */
	void PlayAudioWithLipsync(USoundWave* SoundWave);

	/** Stops current audio playback */
	void StopAudio();

	/** Parses JSON response from backend */
	bool ParseQuestionResponse(const FString& ResponseBody, FString& OutQuestionText, FString& OutQuestionID);
	bool ParseAnswerResponse(const FString& ResponseBody, FString& OutFeedbackText, FString& OutNextQuestionText, FString& OutNextQuestionID);

	/** Makes HTTP request to backend */
	void MakeHTTPRequest(const FString& Endpoint, const FString& Verb, const FString& Content, TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback);

	/** Gets session cookie from response headers */
	FString GetSessionCookie(const FHttpResponsePtr& Response);
};


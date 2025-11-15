// InterviewComponent.cpp
// Implementation of the AI interview system component

#include "InterviewComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Sound/SoundWaveProcedural.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Json.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Base64.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

// Constructor
UInterviewComponent::UInterviewComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to tick every frame
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// Initialize audio component
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("InterviewAudioComponent"));
	AudioComponent->bAutoActivate = false;
	AudioComponent->SetVolumeMultiplier(1.0f);

	// Initialize state
	CurrentState = EInterviewState::Idle;
	InterviewElapsedTime = 0.0f;
	QuestionsAnswered = 0;
	bIsRecording = false;
}

void UInterviewComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure audio component is initialized
	if (!AudioComponent)
	{
		AudioComponent = NewObject<UAudioComponent>(this);
		AudioComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		AudioComponent->bAutoActivate = false;
	}

	UE_LOG(LogTemp, Log, TEXT("InterviewComponent initialized. Backend URL: %s"), *BackendBaseURL);
}

void UInterviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up: stop recording, stop audio, cancel HTTP requests
	StopInterview();
	
	Super::EndPlay(EndPlayReason);
}

void UInterviewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process interview flow state machine
	if (CurrentState != EInterviewState::Idle && CurrentState != EInterviewState::Ended)
	{
		InterviewElapsedTime += DeltaTime;

		// Check if interview duration exceeded
		if (InterviewElapsedTime >= MaxInterviewDuration)
		{
			UE_LOG(LogTemp, Warning, TEXT("Interview duration exceeded. Ending interview."));
			StopInterview();
			return;
		}

		// Process current state
		ProcessInterviewFlow(DeltaTime);
	}
}

void UInterviewComponent::StartInterview()
{
	if (CurrentState != EInterviewState::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("Interview already in progress. Current state: %d"), (int32)CurrentState);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Starting interview session..."));

	// Reset state
	InterviewElapsedTime = 0.0f;
	QuestionsAnswered = 0;
	SessionID = TEXT("");
	CurrentQuestionText = TEXT("");
	CurrentQuestionID = TEXT("");

	// Change state to starting
	SetInterviewState(EInterviewState::Starting);

	// Make HTTP request to start session
	// Your backend expects: POST /api/adaptive with empty body or {}
	FString RequestBody = TEXT("{}");
	MakeHTTPRequest(StartSessionEndpoint, TEXT("POST"), RequestBody, 
		[this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			OnStartSessionResponse(Request, Response, bWasSuccessful);
		});
}

void UInterviewComponent::StopInterview()
{
	UE_LOG(LogTemp, Log, TEXT("Stopping interview session..."));

	// Stop recording if active
	if (bIsRecording)
	{
		StopRecording();
	}

	// Stop audio playback
	StopAudio();

	// Change state to ended
	SetInterviewState(EInterviewState::Ended);

	// Broadcast event
	OnInterviewEnded.Broadcast();
}

void UInterviewComponent::RequestNextQuestion()
{
	if (CurrentState == EInterviewState::Idle || CurrentState == EInterviewState::Ended)
	{
		StartInterview();
		return;
	}

	// Request next question from backend
	SetInterviewState(EInterviewState::WaitingForQuestion);
	
	FString RequestBody = TEXT("{}");
	MakeHTTPRequest(StartSessionEndpoint, TEXT("POST"), RequestBody,
		[this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			OnStartSessionResponse(Request, Response, bWasSuccessful);
		});
}

void UInterviewComponent::SetInterviewState(EInterviewState NewState)
{
	if (CurrentState != NewState)
	{
		EInterviewState OldState = CurrentState;
		CurrentState = NewState;
		
		UE_LOG(LogTemp, Log, TEXT("Interview state changed: %d -> %d"), (int32)OldState, (int32)NewState);
		
		// Broadcast state change event
		OnInterviewStateChanged.Broadcast(NewState);
	}
}

void UInterviewComponent::ProcessInterviewFlow(float DeltaTime)
{
	switch (CurrentState)
	{
	case EInterviewState::PlayingQuestion:
		// Check if audio finished playing
		if (AudioComponent && !AudioComponent->IsPlaying())
		{
			PauseTimer += DeltaTime;
			if (PauseTimer >= PauseAfterQuestion)
			{
				PauseTimer = 0.0f;
				SetInterviewState(EInterviewState::RecordingAnswer);
				StartRecording();
			}
		}
		break;

	case EInterviewState::RecordingAnswer:
		// Check if recording duration exceeded
		if (InterviewElapsedTime - PauseTimer >= MaxRecordingDuration)
		{
			StopRecording();
		}
		// Note: In a real implementation, you'd also check for silence detection here
		break;

	case EInterviewState::PlayingFeedback:
		// Check if feedback audio finished
		if (AudioComponent && !AudioComponent->IsPlaying())
		{
			PauseTimer += DeltaTime;
			if (PauseTimer >= PauseAfterFeedback)
			{
				PauseTimer = 0.0f;
				// Request next question
				RequestNextQuestion();
			}
		}
		break;

	default:
		break;
	}
}

void UInterviewComponent::OnStartSessionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start interview session. HTTP request failed."));
		SetInterviewState(EInterviewState::Idle);
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start interview session. Response code: %d"), ResponseCode);
		SetInterviewState(EInterviewState::Idle);
		return;
	}

	// Extract session cookie if present
	SessionID = GetSessionCookie(Response);

	// Parse response to get question
	FString ResponseBody = Response->GetContentAsString();
	FString QuestionText, QuestionID;
	
	if (ParseQuestionResponse(ResponseBody, QuestionText, QuestionID))
	{
		CurrentQuestionText = QuestionText;
		CurrentQuestionID = QuestionID;

		UE_LOG(LogTemp, Log, TEXT("Question received: %s"), *QuestionText);

		// Broadcast question received event
		OnQuestionReceived.Broadcast(QuestionText, QuestionID);

		// Convert question text to speech and play
		// For now, we'll use Unreal's built-in TTS or your backend TTS
		// If your backend returns audio directly, parse it here
		// Otherwise, make a TTS request
		
		SetInterviewState(EInterviewState::PlayingQuestion);
		
		// TODO: Implement TTS conversion here
		// Option 1: If backend returns audio URL/data, fetch and play it
		// Option 2: Use Unreal's text-to-speech plugin
		// Option 3: Make separate TTS API call to your backend
		
		// For now, we'll simulate by directly playing (you'll need to implement TTS)
		// PlayAudioWithLipsync(nullptr); // Replace with actual TTS audio
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse question from response."));
		SetInterviewState(EInterviewState::Idle);
	}
}

void UInterviewComponent::OnSubmitAnswerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to submit answer. HTTP request failed."));
		SetInterviewState(EInterviewState::Idle);
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to submit answer. Response code: %d"), ResponseCode);
		SetInterviewState(EInterviewState::Idle);
		return;
	}

	// Parse response
	FString ResponseBody = Response->GetContentAsString();
	FString FeedbackText, NextQuestionText, NextQuestionID;
	
	if (ParseAnswerResponse(ResponseBody, FeedbackText, NextQuestionText, NextQuestionID))
	{
		QuestionsAnswered++;

		// Broadcast feedback event
		OnFeedbackReceived.Broadcast(FeedbackText, 0.0f); // Score would come from backend

		// Play feedback audio
		if (!FeedbackText.IsEmpty())
		{
			SetInterviewState(EInterviewState::PlayingFeedback);
			// TODO: Convert feedback to speech and play
			// PlayAudioWithLipsync(nullptr); // Replace with actual TTS audio
		}

		// If there's a next question, it will be requested after feedback
		if (!NextQuestionText.IsEmpty())
		{
			CurrentQuestionText = NextQuestionText;
			CurrentQuestionID = NextQuestionID;
		}
		else
		{
			// No more questions, end interview
			StopInterview();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse answer response."));
		SetInterviewState(EInterviewState::Idle);
	}
}

void UInterviewComponent::StartRecording()
{
	if (bIsRecording)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Starting audio recording..."));

	// Clear previous recording
	AudioRecordingBuffer.Empty();

	// TODO: Implement microphone recording
	// This requires Unreal's Audio Capture plugin or third-party solution
	// Example approach:
	// 1. Use UAudioCaptureComponent or similar
	// 2. Capture audio samples into AudioRecordingBuffer
	// 3. Stop after MaxRecordingDuration or silence detection

	bIsRecording = true;
	PauseTimer = InterviewElapsedTime; // Track when recording started
}

void UInterviewComponent::StopRecording()
{
	if (!bIsRecording)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Stopping audio recording..."));

	bIsRecording = false;

	// Process the recorded audio
	if (AudioRecordingBuffer.Num() > 0)
	{
		SetInterviewState(EInterviewState::ProcessingAnswer);

		// Convert audio to base64 for API submission
		FString AudioBase64 = AudioBufferToBase64(AudioRecordingBuffer);

		// Create JSON payload
		TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
		JsonObject->SetStringField(TEXT("answer"), AudioBase64); // Or send as audio data
		// If your backend expects text, you'll need STT conversion first
		
		// For now, assuming backend handles audio directly
		// Otherwise, make STT call first, then submit text

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		// Submit answer to backend
		MakeHTTPRequest(SubmitAnswerEndpoint, TEXT("POST"), OutputString,
			[this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
			{
				OnSubmitAnswerResponse(Request, Response, bWasSuccessful);
			});
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No audio recorded. Requesting next question."));
		RequestNextQuestion();
	}
}

void UInterviewComponent::PlayAudioWithLipsync(USoundWave* SoundWave)
{
	if (!SoundWave || !AudioComponent)
	{
		return;
	}

	CurrentSoundWave = SoundWave;
	AudioComponent->SetSound(SoundWave);
	AudioComponent->Play();

	// TODO: Trigger lipsync animation
	// This depends on your MetaHuman setup:
	// 1. Use Unreal's Audio Link system for real-time lipsync
	// 2. Or use pre-generated animation curves
	// 3. Or trigger animation blueprint events

	UE_LOG(LogTemp, Log, TEXT("Playing audio with lipsync..."));
}

void UInterviewComponent::StopAudio()
{
	if (AudioComponent)
	{
		AudioComponent->Stop();
	}
	CurrentSoundWave = nullptr;
}

FString UInterviewComponent::AudioBufferToBase64(const TArray<uint8>& AudioData)
{
	return FBase64::Encode(AudioData);
}

USoundWave* UInterviewComponent::CreateSoundWaveFromData(const TArray<uint8>& AudioData, int32 SampleRate)
{
	// Create a procedural sound wave from audio data
	USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(this);
	if (SoundWave)
	{
		SoundWave->SetSampleRate(SampleRate);
		SoundWave->NumChannels = 1; // Mono
		SoundWave->Duration = (float)AudioData.Num() / (SampleRate * 2.0f); // Approximate
		SoundWave->QueueAudio(AudioData.GetData(), AudioData.Num());
	}
	return SoundWave;
}

void UInterviewComponent::MakeHTTPRequest(const FString& Endpoint, const FString& Verb, const FString& Content, TFunction<void(FHttpRequestPtr, FHttpResponsePtr, bool)> Callback)
{
	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	// Build full URL
	FString FullURL = BackendBaseURL + Endpoint;
	Request->SetURL(FullURL);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// Add session cookie if available
	if (!SessionID.IsEmpty())
	{
		Request->SetHeader(TEXT("Cookie"), FString::Printf(TEXT("adaptive_sid=%s"), *SessionID));
	}

	// Set content
	if (!Content.IsEmpty())
	{
		Request->SetContentAsString(Content);
	}

	// Bind callback
	Request->OnProcessRequestComplete().BindLambda(Callback);

	// Process request
	Request->ProcessRequest();

	UE_LOG(LogTemp, Log, TEXT("HTTP Request: %s %s"), *Verb, *FullURL);
}

FString UInterviewComponent::GetSessionCookie(const FHttpResponsePtr& Response)
{
	if (!Response.IsValid())
	{
		return TEXT("");
	}

	// Extract Set-Cookie header
	FString SetCookieHeader = Response->GetHeader(TEXT("Set-Cookie"));
	if (SetCookieHeader.IsEmpty())
	{
		return TEXT("");
	}

	// Parse cookie value (simplified - assumes format: "adaptive_sid=value; ...")
	FString CookieValue;
	if (SetCookieHeader.Split(TEXT("="), nullptr, &CookieValue))
	{
		// Remove trailing semicolon and other parameters
		int32 SemicolonIndex;
		if (CookieValue.FindChar(TEXT(';'), SemicolonIndex))
		{
			CookieValue = CookieValue.Left(SemicolonIndex);
		}
		return CookieValue.TrimStartAndEnd();
	}

	return TEXT("");
}

bool UInterviewComponent::ParseQuestionResponse(const FString& ResponseBody, FString& OutQuestionText, FString& OutQuestionID)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	// Parse based on your backend response structure
	// Example: { "success": true, "data": { "next_question": { "problem": "...", "question_id": "..." } } }
	
	bool bSuccess = JsonObject->GetBoolField(TEXT("success"));
	if (!bSuccess)
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	if (!JsonObject->TryGetObjectField(TEXT("data"), DataObject))
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* NextQuestionObject;
	if (!(*DataObject)->TryGetObjectField(TEXT("next_question"), NextQuestionObject))
	{
		return false;
	}

	// Extract question text
	(*NextQuestionObject)->TryGetStringField(TEXT("problem"), OutQuestionText);
	if (OutQuestionText.IsEmpty())
	{
		(*NextQuestionObject)->TryGetStringField(TEXT("question_text"), OutQuestionText);
	}

	// Extract question ID (optional)
	(*NextQuestionObject)->TryGetStringField(TEXT("question_id"), OutQuestionID);

	return !OutQuestionText.IsEmpty();
}

bool UInterviewComponent::ParseAnswerResponse(const FString& ResponseBody, FString& OutFeedbackText, FString& OutNextQuestionText, FString& OutNextQuestionID)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	bool bSuccess = JsonObject->GetBoolField(TEXT("success"));
	if (!bSuccess)
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* DataObject;
	if (!JsonObject->TryGetObjectField(TEXT("data"), DataObject))
	{
		return false;
	}

	// Extract evaluation/feedback
	const TSharedPtr<FJsonObject>* EvaluationObject;
	if ((*DataObject)->TryGetObjectField(TEXT("evaluation"), EvaluationObject))
	{
		(*EvaluationObject)->TryGetStringField(TEXT("feedback"), OutFeedbackText);
	}
	else
	{
		// Try direct feedback field
		(*DataObject)->TryGetStringField(TEXT("evaluation"), OutFeedbackText);
	}

	// Extract next question
	const TSharedPtr<FJsonObject>* NextQuestionObject;
	if ((*DataObject)->TryGetObjectField(TEXT("next_question"), NextQuestionObject))
	{
		(*NextQuestionObject)->TryGetStringField(TEXT("problem"), OutNextQuestionText);
		if (OutNextQuestionText.IsEmpty())
		{
			(*NextQuestionObject)->TryGetStringField(TEXT("question_text"), OutNextQuestionText);
		}
		(*NextQuestionObject)->TryGetStringField(TEXT("question_id"), OutNextQuestionID);
	}

	return true;
}


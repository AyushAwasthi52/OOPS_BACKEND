// InterviewRoomActor.cpp
// Implementation of the interview room actor

#include "InterviewRoomActor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

AInterviewRoomActor::AInterviewRoomActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Create trigger volume
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetBoxExtent(FVector(500.0f, 500.0f, 300.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void AInterviewRoomActor::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap event
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AInterviewRoomActor::OnTriggerBeginOverlap);

	// Find interviewer component and bind events
	if (InterviewerCharacter)
	{
		UInterviewComponent* InterviewComp = InterviewerCharacter->FindComponentByClass<UInterviewComponent>();
		if (InterviewComp)
		{
			InterviewComp->OnInterviewStateChanged.AddDynamic(this, &AInterviewRoomActor::OnInterviewStateChanged);
			InterviewComp->OnQuestionReceived.AddDynamic(this, &AInterviewRoomActor::OnQuestionReceived);
			InterviewComp->OnFeedbackReceived.AddDynamic(this, &AInterviewRoomActor::OnFeedbackReceived);
			InterviewComp->OnInterviewEnded.AddDynamic(this, &AInterviewRoomActor::OnInterviewEnded);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InterviewRoomActor: InterviewerCharacter does not have InterviewComponent!"));
		}
	}
}

void AInterviewRoomActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if it's the player
	if (OtherActor && OtherActor->IsA<ACharacter>() && !bInterviewStarted)
	{
		UE_LOG(LogTemp, Log, TEXT("Player entered interview room. Starting interview..."));

		// Start interview
		if (InterviewerCharacter)
		{
			UInterviewComponent* InterviewComp = InterviewerCharacter->FindComponentByClass<UInterviewComponent>();
			if (InterviewComp)
			{
				InterviewComp->StartInterview();
				bInterviewStarted = true;
			}
		}
	}
}

void AInterviewRoomActor::OnInterviewStateChanged(EInterviewState NewState)
{
	UE_LOG(LogTemp, Log, TEXT("Interview state changed: %d"), (int32)NewState);

	// You can add UI updates, animations, etc. here based on state
	switch (NewState)
	{
	case EInterviewState::PlayingQuestion:
		// Show question UI, play animations, etc.
		break;
	case EInterviewState::RecordingAnswer:
		// Show recording indicator, start visual feedback
		break;
	case EInterviewState::ProcessingAnswer:
		// Show processing indicator
		break;
	default:
		break;
	}
}

void AInterviewRoomActor::OnQuestionReceived(const FString& QuestionText, const FString& QuestionID)
{
	UE_LOG(LogTemp, Log, TEXT("Question received: %s (ID: %s)"), *QuestionText, *QuestionID);

	// Update UI with question text
	// Trigger animations
	// etc.
}

void AInterviewRoomActor::OnFeedbackReceived(const FString& FeedbackText, float Score)
{
	UE_LOG(LogTemp, Log, TEXT("Feedback received: %s (Score: %.2f)"), *FeedbackText, Score);

	// Show feedback UI
	// Update score display
	// etc.
}

void AInterviewRoomActor::OnInterviewEnded()
{
	UE_LOG(LogTemp, Log, TEXT("Interview ended"));

	// Show completion UI
	// Save results
	// Transition to next scene
	// etc.

	bInterviewStarted = false;
}


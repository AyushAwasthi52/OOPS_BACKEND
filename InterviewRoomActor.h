// InterviewRoomActor.h
// Example Actor class for the interview room
// This demonstrates how to use the InterviewComponent in a complete setup

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "InterviewComponent.h"
#include "InterviewRoomActor.generated.h"

/**
 * InterviewRoomActor - Example actor for the interview room
 * 
 * This actor:
 * - Detects when player enters the room
 * - Automatically starts the interview
 * - Manages interview state and UI
 * 
 * Usage:
 * 1. Place this actor in your level
 * 2. Set the trigger volume to cover the interview area
 * 3. Assign the interviewer character reference
 * 4. The interview will start automatically when player enters
 */
UCLASS(BlueprintType, Blueprintable)
class YOURGAME_API AInterviewRoomActor : public AActor
{
	GENERATED_BODY()

public:
	AInterviewRoomActor(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

	/** Trigger volume for detecting player entry */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerVolume;

	/** Reference to the interviewer character (with InterviewComponent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interview Setup")
	AActor* InterviewerCharacter;

	/** Whether interview has started */
	UPROPERTY(BlueprintReadOnly, Category = "Interview State")
	bool bInterviewStarted = false;

	/** Handle player entering trigger volume */
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, 
		const FHitResult& SweepResult);

	/** Handle interview state changes */
	UFUNCTION()
	void OnInterviewStateChanged(EInterviewState NewState);

	/** Handle question received */
	UFUNCTION()
	void OnQuestionReceived(const FString& QuestionText, const FString& QuestionID);

	/** Handle feedback received */
	UFUNCTION()
	void OnFeedbackReceived(const FString& FeedbackText, float Score);

	/** Handle interview ended */
	UFUNCTION()
	void OnInterviewEnded();
};


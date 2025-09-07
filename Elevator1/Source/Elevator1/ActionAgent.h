// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ActionAgent.generated.h"

UCLASS()
class ELEVATOR1_API AActionAgent : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AActionAgent();

	// It now includes the location the agent should face upon successful arrival.
	UFUNCTION(BlueprintImplementableEvent, Category = "Agent Actions")
	void MoveAgentToLocation(const FVector& TargetLocation, const FVector& LookAtOnArrival, bool bShouldTurnOnArrival, AActor* ElevatorActor);
	
	/** Sets the floor number this agent entered the lift on. Called by the sequencer. */
	void SetEnteredOnFloor(int32 FloorNumber) { EnteredOnFloor = FloorNumber; }

	/** Gets the floor number this agent entered the lift on. */
	int32 GetEnteredOnFloor() const { return EnteredOnFloor; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Event called when this agent is commanded to leave the elevator. */
	// We are going back to the two-stage entry, but adding the final look-at location.
	UFUNCTION(BlueprintImplementableEvent, Category = "AI Actions")
	void ExecuteTwoStageEntry(const FVector& StagingLocation, const FVector& FinalLocation, const FVector& LookAtOnArrival, AActor* ElevatorActor);

	/** Event called when this agent is commanded to leave the elevator. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Agent Actions")
	void WalkOutOfElevatorAndDestroy(const FVector& ExitLocation, AActor* ElevatorActor);

	/** Event called for any non-movement social action, like making eye contact. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Agent Actions")
	void PerformSocialAction(const FString& ActionName);

	// --- NEW FUNCTION TO RESET THE DOONCE NODE ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Agent Actions")
	void ResetActionState();

private:
	/** The floor number the agent was on when it entered the elevator. -1 means not set. */
	int32 EnteredOnFloor = -1;
};
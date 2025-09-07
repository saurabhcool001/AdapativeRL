// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIAgentSequencerComponent.generated.h"

class ACharacter;
class URlManagerComponent;
class UAgentSpawnerComponent;
class UAIElevatorNavigationComponent;
class UBoxComponent;
class URLEnvironmentManager;
class AActionAgent;

// --- NEW DELEGATE DECLARATION ---
// This defines a new event signature that Blueprints can bind to.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSequenceFinished);

// Enum to manage the state of the door sequence
UENUM(BlueprintType)
enum class ESequenceState : uint8
{
    Idle,
    ProcessingExits,
    ProcessingEntries
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ELEVATOR1_API UAIAgentSequencerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAIAgentSequencerComponent();

    /** The main function to call when the elevator doors open at a new floor. */
    UFUNCTION(BlueprintCallable, Category = "AI Sequencer")
    void BeginDoorSequence(int32 NumberOfExits, int32 CurrentFloor);

    /** Checks if the sequencer is currently idle AND all dispatched agents have finished moving. */
    UFUNCTION(BlueprintPure, Category = "AI Sequencer")
    bool IsSequenceFinished() const;

    /** Called from an agent's Blueprint when it has successfully completed its move command. */
    UFUNCTION(BlueprintCallable, Category = "AI Sequencer")
    void ReportAgentMoveComplete();

    // --- DELEGATE PROPERTY ---
    /** This event is broadcast whenever the sequencer finishes all its tasks and becomes idle. */
    UPROPERTY(BlueprintAssignable, Category = "AI Sequencer|Events")
    FOnSequenceFinished OnSequenceFinished;

    /** The maximum number of agents that will be chosen to enter the lift from the waiting group. */
    UPROPERTY(EditAnywhere, Category = "AI Sequencer", meta=(ClampMin="1"))
    int32 MaxAgentsToEnter = 5;

protected:
    virtual void BeginPlay() override;

    // Functions to process the next agent in the queue
    void ProcessNextExit();
    void ProcessNextEntry();

    /** Contains the logic to make a specific agent exit. Called by a timer. */
    UFUNCTION()
    void ExecuteExitForAgent(AActionAgent* AgentToExit);

private: // Changed to private for better encapsulation
    // The main state of our sequencer
    ESequenceState CurrentState;

    // --- Component References (found automatically on BeginPlay) ---
    UPROPERTY()
    TObjectPtr<URlManagerComponent> RlManager;
    UPROPERTY()
    TObjectPtr<UAgentSpawnerComponent> SpawnerComponent;
    UPROPERTY()
    TObjectPtr<UAIElevatorNavigationComponent> NavComponent;
    UPROPERTY()
    TObjectPtr<URLEnvironmentManager> RlEnvManager;

    // --- Queues for managing agents ---
    UPROPERTY()
    TArray<TObjectPtr<ACharacter>> ExitQueue;

    UPROPERTY()
    TArray<TObjectPtr<ACharacter>> EntryQueue;

    // --- NEW COUNTER ---
    /** Keeps track of how many agents have been commanded to move but have not yet reported completion. */
    int32 PendingMoveCount;

    // --- Configuration ---
    // How long to wait between sending each agent through the door.
    UPROPERTY(EditAnywhere, Category = "AI Sequencer")
    float DelayBetweenAgents = 2.0f;

    // Assign your BP_Hana class here in the editor
    UPROPERTY(EditAnywhere, Category = "AI Sequencer")
    TSubclassOf<ACharacter> AgentClassToSpawn;

    // Timer handle for processing the sequence
    FTimerHandle SequenceTimerHandle;
    
    /** The floor number for the sequence currently in progress. */
    int32 SequenceCurrentFloor;

    /** How long an agent should wait after the doors open before starting to exit. */
    UPROPERTY(EditAnywhere, Category = "AI Sequencer")
    float ExitDelay = 0.5f; // Default to half a second
};
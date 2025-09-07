// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RlManagerComponent.generated.h"

// Forward-declare classes to avoid circular dependencies
class ACharacter;
class UBoxComponent;
class USceneComponent;
class UAIElevatorNavigationComponent;
class AActionAgent;
class UStaticMeshComponent;
class UMaterialInterface;

// --- NEW DELEGATE DECLARATION (with TWO parameters) ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStareStateChanged, AActionAgent*, TargetAgent, bool, bIsStaring);

// Add this new enum declaration above the UCLASS
UENUM()
enum class EAgentInteractionState : uint8
{
    None,  // No agent is actively interacting
    Close, // The agent has moved close to the player
    Away,   // The agent has moved away from the player
    BlockingDoor // The agent move to block door 
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ELEVATOR1_API URlManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    URlManagerComponent();

    // Attempts to have a specific agent enter the elevator.
    UFUNCTION(BlueprintCallable, Category = "RL Manager")
    bool AttemptEnterLift(ACharacter* AgentToEnter);

    // Attempts to have a specific agent leave the elevator. Called by the sequencer.
    UFUNCTION(BlueprintCallable, Category = "RL Manager")
    bool AttemptLeaveLift(ACharacter* AgentToLeave);

    // Relays a social action (e.g., "stare") to a random agent currently in the lift.
    UFUNCTION(BlueprintCallable, Category = "RL Manager")
    void RelayActionToRandomAgent(const FString& Action);

    // --- Helper functions for other components ---

    // --- MODIFIED FUNCTION SIGNATURE ---
    /** Gets a specified number of agents to exit in LIFO (Last-In, First-Out) order. */
    TArray<ACharacter*> GetLIFOAgentsToExit(int32 Count, int32 CurrentFloor);

    /** Provides read-only access to the list of agents currently inside. */
    const TArray<TObjectPtr<ACharacter>>& GetAgentsInLift() const { return AgentsInLift; }

    /** Provides access to the walkable area box for navigation purposes. */
    UBoxComponent* GetWalkableArea() const { return WalkableArea; }

    /** Provides access to the exit point scene component for navigation purposes. */
    UFUNCTION(BlueprintPure, Category = "RL Manager")   
    USceneComponent* GetExitPoint() const { return ExitPoint; }

    /** Returns the component designated as the entry staging point. */
    UFUNCTION(BlueprintPure, Category = "RL Manager")
    USceneComponent* GetEntryPoint() const { return EntryPoint; }

    // --- Public Properties ---
    // The maximum number of agents allowed in the elevator. Made public for the sequencer.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RL Config")
    int32 MaxAgentCapacity = 5;

    /** Finds a free slot, marks it as occupied by the agent, and returns its location. */
    FVector FindAndOccupySlot(ACharacter* Agent);

    /** Finds the slot occupied by the given agent and marks it as free. */
    void VacateSlot(ACharacter* Agent);

    /** The main function to handle the 'move_closer' action from Python. */
    void HandleMoveCloserAction();

    // --- NEW CONFIG PROPERTIES ---
    /** When handling a 'move_closer' action, an agent must be at least this far from the player to be considered. */
    //UPROPERTY(EditAnywhere, Category = "RL Config|Movement")
    //float MinDistanceToPlayerForCloseMove = 80.0f;

    /** When handling a 'move_closer' action, an agent must be closer than this distance to the player to be considered. */
    //UPROPERTY(EditAnywhere, Category = "RL Config|Movement")
    //float MaxDistanceToPlayerForCloseMove = 300.0f;

    /** The main function to handle the 'move_away' action from Python. */
    void HandleMoveAwayAction();

    /** Resets the interacting agent, original slot, and state. Called when a new door sequence begins. */
    void ResetInteractionState();

    // --- NEW CONFIG PROPERTIES for Moving Away ---
    /** When handling a 'move_away' action, the agent must be closer than this distance to you. */
    //UPROPERTY(EditAnywhere, Category = "RL Config|Movement")
    //float MaxDistanceForMoveAway = 200.0f;

    /** How far back the agent should try to move when moving away. */
    //UPROPERTY(EditAnywhere, Category = "RL Config|Movement")
    //float MoveAwayDistance = 100.0f;

    /** Called by an agent's Blueprint if it fails to exit, allowing it to re-enter the lift's roster. */
    UFUNCTION(BlueprintCallable, Category = "RL Manager")
    void ReboardFailedAgent(ACharacter* AgentToReboard);

    /** Called from an agent's Blueprint to report that it has successfully completed an interactive move (e.g., move closer, move away). */
    UFUNCTION(BlueprintCallable, Category = "RL Manager|Reporting")
    void ReportInteractiveMoveComplete(ACharacter* ReportingAgent);

    /** Called from an agent's Blueprint to report that it has FAILED an interactive move. */
    UFUNCTION(BlueprintCallable, Category = "RL Manager|Reporting")
    void ReportInteractiveMoveFailed(ACharacter* ReportingAgent);

    /** Sets the stare state on the primary agent. */
    //UFUNCTION(BlueprintCallable, Category = "Agent Actions")
    void SetStareState(bool bIsStaring);

    /** The main function to handle the 'block_door' action from Python. */
    void HandleBlockDoorAction();

    // --- NEW DELEGATE PROPERTY ---
    /** Broadcasts whenever the stare state is changed by a command from Python. Passes the Target Agent and the new state. */
    UPROPERTY(BlueprintAssignable, Category = "Agent Actions|Events")
    FOnStareStateChanged OnStareStateChanged;

    /** Returns the agent currently performing an interactive move, or nullptr if none. */
    UFUNCTION(BlueprintPure, Category = "RL Manager")
    AActionAgent* GetAgentInteractingWithPlayer() const;

    // Called every frame
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

    // --- Configuration ---
    // A reference to the box component that defines the walkable area inside the elevator.
    // This must be set from the owning Elevator Blueprint.
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RL Config", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBoxComponent> WalkableArea;

    /** A component that marks the exact location agents should walk to when EXITIING. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RL Config", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> ExitPoint;
    
    // --- NEW PROPERTY ---
    /** A component that marks the staging location agents should walk to before ENTERING. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RL Config", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> EntryPoint;

    // --- State ---
    // Array holding references to all agent actors currently inside the elevator.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RL State")
    TArray<TObjectPtr<ACharacter>> AgentsInLift;

    // --- NEW COMPONENT REFERENCE ---
    UPROPERTY()
    TObjectPtr<UAIElevatorNavigationComponent> NavComponent;

    // --- NEW STATE MANAGEMENT VARIABLES ---
    /** A reference to the agent that last moved close to the player. */
    UPROPERTY()
    TObjectPtr<AActionAgent> AgentInteractingWithPlayer;

    /** The current state of the interaction. */
    EAgentInteractionState InteractionState;

    // This TMap will hold the visible sphere meshes for our slots.
    UPROPERTY()
    TMap<TObjectPtr<USceneComponent>, TObjectPtr<UStaticMeshComponent>> SlotVisualizers;

    // A reference to the sphere mesh we will use for visualization.
    UPROPERTY()
    TObjectPtr<UStaticMesh> VisualizerMesh;
    
    // A reference to the dynamic material we create for changing colors.
    UPROPERTY()
    TObjectPtr<UMaterialInterface> VisualizerMaterial;

    USceneComponent* FindSlotForAgent(ACharacter* Agent) const;

    /** A component that marks the exact location an agent should stand to block the door. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RL Config", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> DoorBlockLocationMarker;
    
private:
    // --- NEW STATE MANAGEMENT VARIABLE ---
    /** A reference to the slot that the interacting agent came from. */
    UPROPERTY()
    TObjectPtr<USceneComponent> OriginalSlotOfInteractingAgent;

    // This TMap will hold all our designated slots and track which agent is in which slot.
    UPROPERTY()
    TMap<TObjectPtr<USceneComponent>, TWeakObjectPtr<ACharacter>> AgentSlots;

    // --- NEW VARIABLE ---
    /** A sorted list of the agent slots, used to determine priority for interactions. */
    UPROPERTY()
    TArray<TObjectPtr<USceneComponent>> SortedAgentSlots;
};
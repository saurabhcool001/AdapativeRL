// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RLEnvironmentManager.generated.h"

// Forward Declarations
class UVRUDPComponent;
class URlManagerComponent;
class UAgentSpawnerComponent;
class UAIAgentSequencerComponent; // Added Sequencer
class ACharacter;

// --- NEW ENUM ---
// Describes the primary agent activity that has occurred at the current floor stop.
/*UENUM(BlueprintType)
enum class EFloorActivityState : uint8
{
    None,  // No entry or exit has happened yet.
    Entry, // An entry sequence has started.
    Exit   // An exit sequence has started.
};*/

// ENUM FOR THE EXPERIMENTAL GROUPS
UENUM(BlueprintType)
enum class EExperimentalGroup : uint8
{
    // A static elevator with no active virtual agents.
    BaselineControl       UMETA(DisplayName = "Group 1: Baseline Control"),

    // Virtual agents perform random valid actions.
    RandomActionControl   UMETA(DisplayName = "Group 2: Random Action Control"),

    // The full scenario with the RL agent adapting to the user.
    AdaptiveRLAgent       UMETA(DisplayName = "Group 3: Adaptive RL Agent")
};

// Add this new enum above the UCLASS declaration
UENUM(BlueprintType)
enum class EElevatorSimulationState : uint8
{
    AtFloor,    // Doors are open, allowing entry/exit
    Traveling   // Doors are closed, allowing social actions
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ELEVATOR1_API URLEnvironmentManager : public UActorComponent
{
    GENERATED_BODY()

public:
    URLEnvironmentManager();

protected:
    virtual void BeginPlay() override;

public:
    // --- NEW: Overridden from AActor ---
    // Called when the component is destroyed or the game ends.
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Call this from your Elevator Blueprint to start the entire simulation.
    UFUNCTION(BlueprintCallable, Category = "RL Environment")
    void StartSimulation();

    // Call this to pause the simulation (e.g., when elevator doors open).
    UFUNCTION(BlueprintCallable, Category = "RL Environment")
    void PauseSimulation();

    // Call this to set the panic button state from your Player Blueprint.
    UFUNCTION(BlueprintCallable, Category = "RL Environment")
    void SetPanicButtonPressed(bool bIsPressed);

    // --- NEW FUNCTION TO RECEIVE HMD DATA ---
    UFUNCTION(BlueprintCallable, Category = "RL Environment")
    void UpdateHMDData(const FVector& HMDPosition, const FRotator& HMDRotation);

    /** Call this from your Elevator Blueprint when it arrives at a new floor to reset the logic. */
    UFUNCTION(BlueprintCallable, Category = "RL Env Manager")
    void OnElevatorArrivedAtNewFloor(int32 NewFloor);

    /** Call this from your Elevator Blueprint when the doors close to begin the travel phase. */
    UFUNCTION(BlueprintCallable, Category = "RL Environment")
    void OnElevatorDoorsClosed();

    /** Returns true if the simulation has reached its final floor and is no longer running. */
    UFUNCTION(BlueprintPure, Category = "RL Environment")
    bool IsSimulationFinished() const { return bIsSimulationFinished; }

    // --- NEW FUNCTION TO RECEIVE JOYSTICK DATA ---
    UFUNCTION(BlueprintCallable, Category = "RL Environment")
    void UpdateJoystickYValue(float Value);

protected:
    // This function will be called on a timer to send state to Python.
    void TickSimulation();

    // This function will be bound to the UDP Component's OnMessageReceived delegate.
    UFUNCTION()
    void OnActionReceivedFromPython(const FString& Message);

    // Gathers all player state and formats it into a JSON string.
    FString GetPlayerStateAsJSON() const;

    /** The floor number at which to stop the RL simulation. Set to 0 or less to disable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL Config")
    int32 FinalFloorNumber = 0;

    /** Select the experimental group to run for this session. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RL Config")
    EExperimentalGroup CurrentExperimentalGroup = EExperimentalGroup::AdaptiveRLAgent;

private:
    // --- NEW: Helper function to write the collected data to our file. ---
    void SaveLogFile();

    // --- COMPONENT REFERENCES (found automatically on BeginPlay) ---
    UPROPERTY()
    TObjectPtr<UVRUDPComponent> UdpComponent;
    UPROPERTY()
    TObjectPtr<URlManagerComponent> RlManager;
    UPROPERTY()
    TObjectPtr<UAgentSpawnerComponent> SpawnerComponent;
    UPROPERTY()
    TObjectPtr<UAIAgentSequencerComponent> SequencerComponent; // Added reference

    // --- STATE & CONFIG ---
    FTimerHandle SimulationTimerHandle;
    FTimerHandle ActionDurationTimerHandle;
    UPROPERTY(EditAnywhere, Category = "RL Environment")
    float TickInterval = 1.0f; // Set to a more reasonable default

    // --- NEW: Map for defining action durations in the editor ---
    UPROPERTY(EditAnywhere, Category = "RL Config")
    TMap<FString, float> ActionDurations;
    bool bIsPanicButtonPressed = false;

    // --- NEW: HMD Data Storage ---
    FVector LatestHMDPosition;
    FRotator LatestHMDRotation;
    bool bHasHMDData = false;

    // --- NEW: Logging variables ---
    FString SessionLogFileName;
    // Stores all the JSON state strings for the current session.
    TArray<FString> SessionLogData;

    // --- NEW PROPERTY ---
    /** Tracks whether agents have been entering or exiting at the current floor. */
    /*EFloorActivityState CurrentFloorActivity;*/

    // --- NEW STATE VARIABLE ---
    /** Tracks the overall state of the elevator for the simulation. */
    EElevatorSimulationState ElevatorState;

    /** Becomes true when the final floor is reached, stopping all RL-related activity. */
    bool bIsSimulationFinished = false;

    // --- NEW VARIABLE TO STORE JOYSTICK VALUE ---
    float LatestJoystickYValue = 0.0f;

    /** Counts the total number of times the panic button (joystick trigger) has been pressed. */
    int32 PanicButtonPressCount;

    /** State variable to ensure we only count a joystick trigger pull once until it's released. */
    bool bIsJoystickTriggerPressed;


public:
    // Made public so Elevator BP can set it
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RL Environment")
    int32 CurrentFloor = 0;

    /** Returns the currently selected experimental group. */
    UFUNCTION(BlueprintPure, Category = "RL Environment")
    EExperimentalGroup GetCurrentExperimentalGroup() const { return CurrentExperimentalGroup; }
};
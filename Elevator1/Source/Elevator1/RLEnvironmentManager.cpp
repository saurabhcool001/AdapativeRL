// Fill out your copyright notice in the Description page of Project Settings.

#include "RLEnvironmentManager.h"
#include "ElevatorBase/VRUDPComponent.h"
#include "RlManagerComponent.h"
#include "AgentSpawnerComponent.h"
#include "AIAgentSequencerComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ActionAgent.h"
// --- NEW INCLUDES FOR LOGGING ---
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

URLEnvironmentManager::URLEnvironmentManager()
{
    PrimaryComponentTick.bCanEverTick = false;

    // --- NEW: Initialize HMD variables ---
    LatestHMDPosition = FVector::ZeroVector;
    LatestHMDRotation = FRotator::ZeroRotator;
    bHasHMDData = false;

    // --- NEW: Initialize panic button and joystick variables ---
    PanicButtonPressCount = 0;
    bIsJoystickTriggerPressed = false;

    // --- UNNECESSARY CODE COMMENTED OUT ---
    // This state variable was used to lock the floor into an "entry-only" or "exit-only" mode.
    // We remove it to give the RL agent full control.
    /*CurrentFloorActivity = EFloorActivityState::None;*/
    // ... other constructor code ...
    ElevatorState = EElevatorSimulationState::AtFloor; // Start at a floor by default

    // --- NEW: Initialize default action durations for social actions ONLY ---
    // Note: come_in_lift and go_out_of_lift are NOT included here - they are handled separately
    ActionDurations.Add(TEXT("move_closer"), 2.0f);
    ActionDurations.Add(TEXT("move_away"), 2.0f);
    ActionDurations.Add(TEXT("make_eye_contact"), 3.0f);
    ActionDurations.Add(TEXT("block_door"), 4.0f);

    // Debug: Print what's in the ActionDurations map
    UE_LOG(LogTemp, Warning, TEXT("ActionDurations map initialized with %d entries:"), ActionDurations.Num());
    for (const auto& Pair : ActionDurations)
    {
        UE_LOG(LogTemp, Warning, TEXT("  - '%s': %.1f"), *Pair.Key, Pair.Value);
    }
}

void URLEnvironmentManager::BeginPlay()
{
    Super::BeginPlay();

    // Find the other components on the same actor. This makes setup automatic.
    AActor* Owner = GetOwner();
    if (Owner)
    {
        UdpComponent = Owner->FindComponentByClass<UVRUDPComponent>();
        RlManager = Owner->FindComponentByClass<URlManagerComponent>();
        SpawnerComponent = Owner->FindComponentByClass<UAgentSpawnerComponent>();
        SequencerComponent = Owner->FindComponentByClass<UAIAgentSequencerComponent>(); // Find the sequencer
    }

    // Bind our C++ function to the UDP component's delegate.
    if (UdpComponent)
    {
        UdpComponent->OnMessageReceived.AddDynamic(this, &URLEnvironmentManager::OnActionReceivedFromPython);
    }
}

void URLEnvironmentManager::StartSimulation()
{
    // If the simulation is finished, do not start it again.
    if (bIsSimulationFinished) return;

    // --- MODIFIED: Create a unique log file for this session ONCE ---
    if (SessionLogFileName.IsEmpty())
    {
        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
        // Change the extension to .json
        FString FileName = FString::Printf(TEXT("unreal_log_%s.json"), *Timestamp);
        SessionLogFileName = FPaths::ProjectDir() + TEXT("data/") + FileName;

        // Ensure the "data" directory exists
        FString DirectoryPath = FPaths::GetPath(SessionLogFileName);
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*DirectoryPath))
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*DirectoryPath);
        }
    }

    GetWorld()->GetTimerManager().SetTimer(SimulationTimerHandle, this, &URLEnvironmentManager::TickSimulation, TickInterval, true, 0.0f);
    UE_LOG(LogTemp, Warning, TEXT("[RL Env Manager] Simulation Started. Logging to %s"), *SessionLogFileName);
}

void URLEnvironmentManager::PauseSimulation()
{
    GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle);
    UE_LOG(LogTemp, Warning, TEXT("[RL Env Manager] Simulation Paused."));
}

void URLEnvironmentManager::SetPanicButtonPressed(bool bIsPressed)
{
    bIsPanicButtonPressed = bIsPressed;
}

// Add this new function to the .cpp file
void URLEnvironmentManager::OnElevatorDoorsClosed()
{
    // If the simulation is finished, do not switch to traveling state.
    if (bIsSimulationFinished) return;

    UE_LOG(LogTemp, Log, TEXT("Doors closed. Switching to TRAVELING state and starting social action simulation."));
    ElevatorState = EElevatorSimulationState::Traveling;
    StartSimulation(); // This starts the social action loop
}

// --- NEW FUNCTION TO RECEIVE HMD DATA FROM BLUEPRINT ---
void URLEnvironmentManager::UpdateHMDData(const FVector& HMDPosition, const FRotator& HMDRotation)
{
    LatestHMDPosition = HMDPosition;
    LatestHMDRotation = HMDRotation;
    bHasHMDData = true;
}

void URLEnvironmentManager::TickSimulation()
{
    // If the simulation is finished, do not send any more state.
    if (bIsSimulationFinished)
    {
        // Also clear the timer one last time just in case.
        GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle);
        return;
    }

    if (!UdpComponent) return;

    FString StateJSON = GetPlayerStateAsJSON();
    UdpComponent->SendUDPMessage(StateJSON);

    // --- MODIFIED: Append the state to our in-memory array ---
    SessionLogData.Add(StateJSON);

    // Reset panic button after sending, so it only triggers once per press.
    if (bIsPanicButtonPressed)
    {
        bIsPanicButtonPressed = false;
    }
}

// --- NEW FUNCTION IMPLEMENTATION ---
void URLEnvironmentManager::OnElevatorArrivedAtNewFloor(int32 NewFloor)
{
    // --- NEW "SOFT STOP" LOGIC ---
    // Check if the final floor feature is enabled and if we have arrived at that floor.
    if (FinalFloorNumber > 0 && NewFloor == FinalFloorNumber)
    {
        UE_LOG(LogTemp, Warning, TEXT("Arrived at final floor %d. Stopping RL simulation and saving log."), NewFloor);
        PauseSimulation(); // Stops sending state to Python
        bIsSimulationFinished = true; // Set our flag to block new spawns and commands
        SaveLogFile(); // Save the collected data to the JSON file
        return; // Stop further execution for this event
    }
    // --- END OF NEW LOGIC ---

    CurrentFloor = NewFloor;

    // --- UNNECESSARY CODE COMMENTED OUT ---
    // We no longer need to reset this state on every floor.
    //CurrentFloorActivity = EFloorActivityState::None; 

    UE_LOG(LogTemp, Log, TEXT("[RL Env Manager] Arrived at floor %d. Pausing simulation and switching to AT_FLOOR state."), NewFloor);

    // Clear ALL timers that could restart the simulation.
    GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle);
    GetWorld()->GetTimerManager().ClearTimer(ActionDurationTimerHandle);

    ElevatorState = EElevatorSimulationState::AtFloor;

    // --- FIX: Immediately send the new state to Python to get an action ---
    if (CurrentFloor > 0)
    {
        TickSimulation();
    }
}

void URLEnvironmentManager::OnActionReceivedFromPython(const FString& Message)
{
    // --- NEW CHECK AT THE TOP ---
    // If the simulation has finished, ignore all incoming commands from Python.
    if (bIsSimulationFinished)
    {
        UE_LOG(LogTemp, Warning, TEXT("Simulation has finished. Ignoring action: %s"), *Message);
        return;
    }
    // --- END OF NEW CHECK ---

    // --- Add a check for the "do_nothing" action from Python ---
    if (Message == "do_nothing")
    {
        // Don't log an error, just silently ignore it.
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[RL Env Manager] Action Received: %s"), *Message);

    // --- CRITICAL FIX: Check sequencer busy state FIRST, before any other logic ---
    if (SequencerComponent && !SequencerComponent->IsSequenceFinished())
    {
        UE_LOG(LogTemp, Error, TEXT("Sequencer is still busy! Rejecting %s action."), *Message);
        return;
    }

    // --- FLOOR 0 RULE: No actions allowed at floor 0 ---
    if (CurrentFloor == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Action '%s' REJECTED because we are at floor 0. No agent actions allowed here. Resuming simulation."), *Message);
        StartSimulation();
        return;
    }

    // --- UNNECESSARY CODE COMMENTED OUT ---
    // This restrictive rule has been removed to allow the RL agent to learn freely.
    /*if (CurrentFloorActivity == EFloorActivityState::Entry && Message == "go_out_of_lift")
    {
        UE_LOG(LogTemp, Error, TEXT("Action 'go_out_of_lift' REJECTED because floor %d already has Entry activity. Only one action type per floor allowed!"), CurrentFloor);
        StartSimulation();
        return;
    }
    if (CurrentFloorActivity == EFloorActivityState::Exit && Message == "come_in_lift")
    {
        UE_LOG(LogTemp, Error, TEXT("Action 'come_in_lift' REjected because floor %d already has Exit activity. Only one action type per floor allowed!"), CurrentFloor);
        StartSimulation();
        return;
    }*/

    if (!RlManager || !SequencerComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("[RL Env Manager] RlManager or SequencerComponent is missing!"));
        return;
    }

    // --- NEW STARE COMMAND LOGIC ---
    // Before processing the action, update the stare state on the RL Manager.
    // This function will need to be added to your RlManagerComponent.
    if (Message == "make_eye_contact")
    {
        RlManager->SetStareState(true);
    }
    else
    {
        // For any other action, we ensure the stare state is turned off.
        RlManager->SetStareState(false);
    }
    // --- END NEW STARE COMMAND LOGIC ---

    // --- NEW, STRICT STATE VALIDATION ---
    bool bIsSocialAction = ActionDurations.Contains(Message);
    bool bIsSequencedAction = (Message == "go_out_of_lift" || Message == "come_in_lift");

    if (bIsSequencedAction)
    {
        GetWorld()->GetTimerManager().ClearTimer(ActionDurationTimerHandle);
    }

    if (ElevatorState == EElevatorSimulationState::AtFloor && bIsSocialAction)
    {
        UE_LOG(LogTemp, Error, TEXT("Action '%s' REJECTED. Social actions are not allowed when elevator is AtFloor."), *Message);
        return;
    }

    if (ElevatorState == EElevatorSimulationState::Traveling && bIsSequencedAction)
    {
        UE_LOG(LogTemp, Error, TEXT("Action '%s' REJECTED. Entry/Exit actions are not allowed when elevator is Traveling."), *Message);
        return;
    }
    // --- END NEW VALIDATION ---

    // --- NEW DURATION-BASED LOGIC ---
    PauseSimulation();

    // Check if it's a long, sequenced action
    if (Message == "go_out_of_lift" || Message == "come_in_lift")
    {
        // --- UNNECESSARY CODE COMMENTED OUT ---
        // All of this logic for locking the floor state is now removed.
        /*
        FString CurrentStateString = "Unknown";
        switch(CurrentFloorActivity)
        {
            case EFloorActivityState::None:  CurrentStateString = "None"; break;
            case EFloorActivityState::Entry: CurrentStateString = "Entry"; break;
            case EFloorActivityState::Exit:  CurrentStateString = "Exit"; break;
        }
        UE_LOG(LogTemp, Warning, TEXT("Processing %s action. Current floor activity state: %s"), *Message, *CurrentStateString);

        if (CurrentFloorActivity == EFloorActivityState::None)
        {
            if (Message == "go_out_of_lift")
            {
                CurrentFloorActivity = EFloorActivityState::Exit;
                UE_LOG(LogTemp, Log, TEXT("Floor activity state locked to: Exit - only exits allowed for this floor"));
            }
            else
            {
                CurrentFloorActivity = EFloorActivityState::Entry;
                UE_LOG(LogTemp, Log, TEXT("Floor activity state locked to: Entry - only entries allowed for this floor"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("VIOLATION: %s action REJECTED because floor %d already has %s activity. Only one action type per floor allowed!"),
                *Message, CurrentFloor, *CurrentStateString);
            StartSimulation();
            return;
        }
        */

        // The sequencer will call StartSimulation() when it is completely finished.
        int32 Exits = 0;
        if (Message == "go_out_of_lift")
        {
            TArray<ACharacter*> EligibleAgents = RlManager->GetLIFOAgentsToExit(1, CurrentFloor);
            Exits = EligibleAgents.Num();
            UE_LOG(LogTemp, Log, TEXT("Exit action requested. Found %d eligible agents to exit on floor %d"), Exits, CurrentFloor);

            if (Exits == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Exit action REJECTED - no agents eligible to exit on floor %d. Resuming simulation."), CurrentFloor);
                StartSimulation();
                return;
            }
        }
        else if (Message == "come_in_lift")
        {
            TArray<ACharacter*> WaitingAgents = SpawnerComponent->GetWaitingAgents();
            UE_LOG(LogTemp, Log, TEXT("Entry action requested. Found %d waiting agents to enter on floor %d"), WaitingAgents.Num(), CurrentFloor);

            if (WaitingAgents.Num() == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("Entry action REJECTED - no agents waiting to enter on floor %d. Resuming simulation."), CurrentFloor);
                StartSimulation();
                return;
            }
        }
        UE_LOG(LogTemp, Log, TEXT("Calling sequencer with %d exits for floor %d"), Exits, CurrentFloor);
        SequencerComponent->BeginDoorSequence(Exits, CurrentFloor);
    }
    // Check if it's a simple action with a defined duration
    else
    {
        if (ActionDurations.Contains(Message))
        {
            const float Duration = ActionDurations[Message];
            UE_LOG(LogTemp, Warning, TEXT("Action '%s' found in ActionDurations with duration %.1f"), *Message, Duration);

            if (Message == "move_closer")
            {
                RlManager->HandleMoveCloserAction();
            }
            else if (Message == "move_away")
            {
                RlManager->HandleMoveAwayAction();
            }
            else if (Message == "block_door")
            {
                RlManager->HandleBlockDoorAction();
            }
            else
            {
                RlManager->RelayActionToRandomAgent(Message);
            }

            GetWorld()->GetTimerManager().SetTimer(ActionDurationTimerHandle, this, &URLEnvironmentManager::StartSimulation, Duration, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[RL Env Manager] Action '%s' is unknown. Resuming simulation."), *Message);
            StartSimulation();
        }
    }
}

// --- MODIFIED FUNCTION ---
FString URLEnvironmentManager::GetPlayerStateAsJSON() const
{
    if (!RlManager || !SpawnerComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("GetPlayerStateAsJSON failed: RlManagerComponent or AgentSpawnerComponent is missing!"));
        return TEXT("{\"error\": \"Missing required components\"}");
    }

    int32 NumAgents = RlManager->GetAgentsInLift().Num();
    FString DistanceStr = "empty";
    FString GazeStr = "true";
    ACharacter* TargetAgent = nullptr; // This will hold the agent we use for calculations

    if (RlManager)
    {
        NumAgents = RlManager->GetAgentsInLift().Num();
    }

    if (bHasHMDData && RlManager && NumAgents > 0)
    {
        // --- NEW HYBRID LOGIC TO SELECT THE MOST RELEVANT AGENT ---

        // 1. Prioritize the agent that is actively interacting with the player.
        TargetAgent = Cast<ACharacter>(RlManager->GetAgentInteractingWithPlayer());

        // 2. If no agent is actively interacting, fall back to the closest agent.
        if (!IsValid(TargetAgent))
        {
            const TArray<TObjectPtr<ACharacter>>& AgentsInLift = RlManager->GetAgentsInLift();
            float MinDistance = -1.0f;

            for (ACharacter* Agent : AgentsInLift)
            {
                if (IsValid(Agent))
                {
                    float Distance = FVector::Dist(LatestHMDPosition, Agent->GetActorLocation());
                    if (TargetAgent == nullptr || Distance < MinDistance)
                    {
                        MinDistance = Distance;
                        TargetAgent = Agent;
                    }
                }
            }
        }
        // --- END OF NEW LOGIC ---

        // Now, perform the calculations using the selected TargetAgent
        if (IsValid(TargetAgent))
        {
            FVector AgentLocation = TargetAgent->GetActorLocation();
            float DistanceToTarget = FVector::Dist(LatestHMDPosition, AgentLocation);

            // Log DistanceToTarget
            UE_LOG(LogTemp, Log, TEXT("DistanceToTarget: %f"), DistanceToTarget);
            if (GEngine)
            {
                //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("DistanceToTarget: %f"), DistanceToTarget));
            }

            // Distance calculation
            if (DistanceToTarget < 3300.0) DistanceStr = "near";
            else if (DistanceToTarget < 3700.0) DistanceStr = "mid";
            else DistanceStr = "far";

            // Gaze calculation
            FVector PlayerForward = LatestHMDRotation.Vector();
            FVector DirToAgent = (AgentLocation - LatestHMDPosition).GetSafeNormal();
            float DotProduct = FVector::DotProduct(PlayerForward, DirToAgent);

            // Log DotProduct
            UE_LOG(LogTemp, Log, TEXT("DotProduct: %f"), DotProduct);
            if (GEngine)
            {
                //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("DotProduct: %f"), DotProduct));
            }

            if (FVector::DotProduct(PlayerForward, DirToAgent) < 0.3)
            {
                GazeStr = "false";
            }
            else {
                GazeStr = "true";
            }
        }
    }

    FString PanicStr = bIsPanicButtonPressed ? "true" : "false";

    TArray<FString> ValidActions;

    if (CurrentFloor == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("GetPlayerStateAsJSON: At floor 0 - no valid actions allowed."));
    }
    else
    {
        TArray<FString> SocialActions = { TEXT("move_closer"), TEXT("move_away"), TEXT("make_eye_contact"), TEXT("block_door") };

        // --- UNNECESSARY DEBUG LOG COMMENTED OUT ---
        /*
        FString ActivityStateString = "Unknown";
        switch(CurrentFloorActivity)
        {
            case EFloorActivityState::None:  ActivityStateString = "None"; break;
            case EFloorActivityState::Entry: ActivityStateString = "Entry"; break;
            case EFloorActivityState::Exit:  ActivityStateString = "Exit"; break;
        }
        UE_LOG(LogTemp, Warning, TEXT("GetPlayerStateAsJSON: Using floor activity state -> %s"), *ActivityStateString);
        */

        switch (ElevatorState)
        {
        case EElevatorSimulationState::AtFloor:
            UE_LOG(LogTemp, Log, TEXT("GetPlayerStateAsJSON: State is AtFloor. Checking for Entry/Exit actions."));

            // --- UNNECESSARY CODE COMMENTED OUT ---
            // This old logic was too restrictive.
            /*
            switch (CurrentFloorActivity)
            {
                case EFloorActivityState::None:
                    if (NumAgents < RlManager->MaxAgentCapacity && SpawnerComponent->GetWaitingAgents().Num() > 0)
                    {
                        ValidActions.Add(TEXT("come_in_lift"));
                    }
                    if (NumAgents > 0 && RlManager->GetLIFOAgentsToExit(1, CurrentFloor).Num() > 0)
                    {
                        ValidActions.Add(TEXT("go_out_of_lift"));
                    }
                    break;
                case EFloorActivityState::Entry:
                    if (NumAgents < RlManager->MaxAgentCapacity && SpawnerComponent->GetWaitingAgents().Num() > 0)
                    {
                        ValidActions.Add(TEXT("come_in_lift"));
                    }
                    break;
                case EFloorActivityState::Exit:
                    if (NumAgents > 0 && RlManager->GetLIFOAgentsToExit(1, CurrentFloor).Num() > 0)
                    {
                        ValidActions.Add(TEXT("go_out_of_lift"));
                    }
                    break;
            }
            */

            // --- NEW UNRESTRICTIVE LOGIC ---
            // This new logic allows Python to choose from all possible actions.

            // Can an agent come in?
            if (NumAgents < RlManager->MaxAgentCapacity && SpawnerComponent->GetWaitingAgents().Num() > 0)
            {
                ValidActions.Add(TEXT("come_in_lift"));
            }
            // Can an agent go out?
            if (NumAgents > 0 && RlManager->GetLIFOAgentsToExit(1, CurrentFloor).Num() > 0)
            {
                ValidActions.Add(TEXT("go_out_of_lift"));
            }
            break;

        case EElevatorSimulationState::Traveling:
            UE_LOG(LogTemp, Log, TEXT("GetPlayerStateAsJSON: State is Traveling. Adding social actions."));
            if (NumAgents > 0)
            {
                ValidActions.Append(SocialActions);
            }
            break;
        }

    }

    FString ValidActionsJsonString = FString::Join(ValidActions, TEXT("\",\""));
    if (!ValidActionsJsonString.IsEmpty())
    {
        ValidActionsJsonString = FString::Printf(TEXT("\"%s\""), *ValidActionsJsonString);
    }

    // Helper to convert our enum to a string for the JSON
    FString GroupStr = "AdaptiveRLAgent";
    switch (CurrentExperimentalGroup)
    {
    case EExperimentalGroup::BaselineControl:
        GroupStr = "BaselineControl";
        break;
    case EExperimentalGroup::RandomActionControl:
        GroupStr = "RandomActionControl";
        break;
    case EExperimentalGroup::AdaptiveRLAgent:
        GroupStr = "AdaptiveRLAgent";
        break;
    }

    // --- MODIFIED: Added a new "panic_button_press_count" key-value pair to the JSON string ---
    return FString::Printf(TEXT("{\"distance_to_virtual_agent\": \"%s\", \"gaze_alignment\": %s, \"panic_triggered\": %s, \"num_agents_in_lift\": %d, \"joystick_y\": %.2f, \"panic_button_press_count\": %d, \"experimental_group\": \"%s\", \"valid_actions\": [%s]}"),
        *DistanceStr,
        *GazeStr,
        *PanicStr,
        NumAgents,
        LatestJoystickYValue,
        PanicButtonPressCount, // Pass the new count here
        *GroupStr,
        *ValidActionsJsonString
    );
}

// --- MODIFIED FUNCTION ---
void URLEnvironmentManager::UpdateJoystickYValue(float Value)
{
    LatestJoystickYValue = Value;

    // --- NEW: Logic to treat the joystick Y-axis as a panic button trigger ---
    const float TriggerThreshold = 0.6f; // The joystick value must be > 0.8 to count as a press.

    // 1. Check if the trigger is pulled past the threshold AND it wasn't already pressed.
    if (Value > TriggerThreshold && !bIsJoystickTriggerPressed)
    {
        // Set the main panic button flag so it gets sent in the next JSON state update.
        bIsPanicButtonPressed = true;

        // Set our state variable to true to prevent counting this press again on the next frame.
        bIsJoystickTriggerPressed = true;

        // Increment the total panic counter.
        PanicButtonPressCount++;

        // Log the event for immediate debugging in the Unreal editor.
        UE_LOG(LogTemp, Warning, TEXT("PANIC TRIGGERED BY JOYSTICK! Total count: %d"), PanicButtonPressCount);
    }
    // 2. Check if the trigger has been released below the threshold.
    else if (Value < TriggerThreshold && bIsJoystickTriggerPressed)
    {
        // Reset our state variable so that the trigger can be pulled again.
        bIsJoystickTriggerPressed = false;
    }
}

// --- NEW FUNCTIONS FOR SINGLE-FILE LOGGING ---

void URLEnvironmentManager::SaveLogFile()
{
    // Only write the file if we actually have data and a valid filename.
    if (SessionLogData.Num() == 0 || SessionLogFileName.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Saving session log with %d entries to %s"), SessionLogData.Num(), *SessionLogFileName);

    // Join all the individual JSON strings with a comma and a newline for readability.
    FString JsonArrayContent = FString::Join(SessionLogData, TEXT(",\n"));

    // Wrap the joined content in square brackets to form a valid JSON array.
    FString FinalJsonString = FString::Printf(TEXT("[\n%s\n]"), *JsonArrayContent);

    // Save the final, complete JSON string to the file, overwriting anything that was there.
    FFileHelper::SaveStringToFile(FinalJsonString, *SessionLogFileName);

    // Clear the data array to prevent accidentally writing the same data twice.
    SessionLogData.Empty();
}

void URLEnvironmentManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // This is our safety net. Make sure to save any pending log data
    // when the simulation ends for any reason (e.g., stopping the PIE session).
    SaveLogFile();

    Super::EndPlay(EndPlayReason);
}
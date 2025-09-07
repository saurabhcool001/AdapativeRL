// Fill out your copyright notice in the Description page of Project Settings.


// AIAgentSequencerComponent.cpp
#include "AIAgentSequencerComponent.h"
#include "ActionAgent.h" // Using our new C++ base class
#include "RlManagerComponent.h"
#include "AgentSpawnerComponent.h"
#include "AIElevatorNavigationComponent.h"
#include "RLEnvironmentManager.h" // Added include
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UAIAgentSequencerComponent::UAIAgentSequencerComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;
    CurrentState = ESequenceState::Idle;
    PendingMoveCount = 0; // Initialize counter
}


void UAIAgentSequencerComponent::BeginPlay()
{
    Super::BeginPlay();
    // Automatically find the other required components on the same actor
    AActor* Owner = GetOwner();
    if (Owner)
    {
        RlManager = Owner->FindComponentByClass<URlManagerComponent>();
        SpawnerComponent = Owner->FindComponentByClass<UAgentSpawnerComponent>();
        NavComponent = Owner->FindComponentByClass<UAIElevatorNavigationComponent>();
        RlEnvManager = Owner->FindComponentByClass<URLEnvironmentManager>();
    }
}

void UAIAgentSequencerComponent::BeginDoorSequence(int32 NumberOfExits, int32 CurrentFloor)
{
    // --- ADD THIS AT THE VERY TOP OF THE FUNCTION ---
    if (RlManager)
    {
        RlManager->ResetInteractionState();
    }
    // --- END OF ADDITION ---
    
    // --- SIMPLIFIED LOGIC: Always allow new sequences ---
    // The RLEnvironmentManager already handles the busy check, so we don't need to duplicate it here
    
    // --- NEW: Log the action type clearly ---
    if (NumberOfExits > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Sequencer: EXIT action requested - will only process exits, no entries"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Sequencer: ENTRY action requested - will only process entries, no exits"));
    }

    // --- NEW: Store the floor for this sequence ---
    this->SequenceCurrentFloor = CurrentFloor;

    // --- SIMPLIFIED: Always reset state for new sequence ---
    CurrentState = ESequenceState::Idle;
    PendingMoveCount = 0;
    ExitQueue.Empty();
    EntryQueue.Empty();

    // --- 1. Populate the Exit Queue (THIS LINE IS MODIFIED) ---
    // Now we pass the current floor to GetRandomAgents so it can filter them.

    //ExitQueue = RlManager->GetRandomAgents(NumberOfExits, this->SequenceCurrentFloor);

    // --- 2. ONLY populate Entry Queue if this is an ENTRY action ---
    // This is the key fix: only allow entries when the action is "come_in_lift"
    
    // Get waiting agents count for logging (regardless of action type)
    TArray<ACharacter*> WaitingAgents = SpawnerComponent->GetWaitingAgents();
    
    // --- FIX: Populate queues based on the action type ONLY ---
    if (NumberOfExits > 0)
    {
        // --- MODIFIED TO CALL THE NEW LIFO FUNCTION ---
        ExitQueue = RlManager->GetLIFOAgentsToExit(NumberOfExits, this->SequenceCurrentFloor);
    }

    else
    {
        // --- THIS IS THE CORRECTED LOGIC FOR CALCULATING HOW MANY AGENTS CAN ENTER ---
        if (RlManager)
        {
            // First, calculate the available capacity in the lift.
            int32 CurrentAgentCount = RlManager->GetAgentsInLift().Num();
            int32 AvailableCapacity = RlManager->MaxAgentCapacity - CurrentAgentCount;

            // Then, determine how many agents are waiting outside.
            int32 NumWaiting = WaitingAgents.Num();

            if (AvailableCapacity > 0 && NumWaiting > 0)
            {
                // Decide how many agents to actually let in. It's the smaller of the two values.
                int32 MaxCanEnter = FMath::Min(AvailableCapacity, NumWaiting);

                // Now, pick a random number up to this safe maximum.
                int32 NumToEnter = FMath::RandRange(1, MaxCanEnter);

                UE_LOG(LogTemp, Log, TEXT("Sequencer: Lift has %d available slots. Commanding %d agents to enter."), AvailableCapacity, NumToEnter);

                // Populate the queue with the correct number of agents.
                for (int32 i = WaitingAgents.Num() - 1; i >= 0 && EntryQueue.Num() < NumToEnter; --i)
                {
                    EntryQueue.Add(WaitingAgents[i]);
                }
            }
        }
        // --- END OF CORRECTION ---
    }
    //else
    //{
    //    // --- THIS IS THE LIFO ENTRY LOGIC ---
    //    // We decide how many to let in, then pull them from the END of the waiting list.
    //    int32 NumToEnter = FMath::RandRange(1, MaxAgentsToEnter);

    //    for (int32 i = WaitingAgents.Num() - 1; i >= 0 && EntryQueue.Num() < NumToEnter; --i)
    //    {
    //        EntryQueue.Add(WaitingAgents[i]);
    //    }
    //    UE_LOG(LogTemp, Log, TEXT("Sequencer: Populating EntryQueue with LIFO logic."));
    //}
    
    /*UE_LOG(LogTemp, Warning, TEXT("Sequencer: Queued %d agents to exit and %d agents to enter."), ExitQueue.Num(), EntryQueue.Num());*/
    // --- THIS LINE IS UPDATED FOR MORE DETAILED LOGGING ---
    UE_LOG(LogTemp, Log, TEXT("Sequencer State -- Total waiting outside: %d | Queued to exit: %d | Queued to enter: %d"),
        WaitingAgents.Num(),
        ExitQueue.Num(),
        EntryQueue.Num()
    );

    // --- 3. Start the Sequence ---
    if (ExitQueue.Num() > 0)
    {
        CurrentState = ESequenceState::ProcessingExits;
        ProcessNextExit();
    }
    else if (EntryQueue.Num() > 0)
    {
        CurrentState = ESequenceState::ProcessingEntries;
        ProcessNextEntry();
    }
    else
    {
        // If there's nothing to do, fire the event immediately.
        UE_LOG(LogTemp, Log, TEXT("Sequencer: No agents to process. Ending sequence immediately."));
        OnSequenceFinished.Broadcast();
        
        if (RlEnvManager)
        {
            RlEnvManager->StartSimulation();
        }
    }
}

void UAIAgentSequencerComponent::ExecuteExitForAgent(AActionAgent* AgentToExit)
{
    if (!AgentToExit) return;

    if (RlManager && RlManager->AttemptLeaveLift(AgentToExit))
    {
        UE_LOG(LogTemp, Log, TEXT("Sequencer: Agent %s commanded to exit after delay."), *AgentToExit->GetName());
        PendingMoveCount++;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Sequencer: Agent %s could not leave lift after delay (was it already removed or invalid?)."), *AgentToExit->GetName());
    }

    // After this agent's action has been triggered, check if there are more agents in the queue.
    if (ExitQueue.Num() > 0)
    {
        // If so, set a timer to process the *next* agent after the configured delay between agents.
        GetWorld()->GetTimerManager().SetTimer(SequenceTimerHandle, this, &UAIAgentSequencerComponent::ProcessNextExit, DelayBetweenAgents, false);
    }
    else
    {
        // If the queue is now empty, we check if the entire sequence is over.
        if (IsSequenceFinished())
        {
            OnSequenceFinished.Broadcast();
            if (RlEnvManager)
            {
                RlEnvManager->StartSimulation();
            }
        }
    }
}

void UAIAgentSequencerComponent::ProcessNextExit()
{
    if (ExitQueue.Num() > 0)
    {
        AActionAgent* AgentToExit = Cast<AActionAgent>(ExitQueue.Pop());
        if (AgentToExit)
        {
            UE_LOG(LogTemp, Log, TEXT("Sequencer: Starting %.2fs exit delay for agent %s."), ExitDelay, *AgentToExit->GetName());

            // Instead of moving the agent immediately, we set a timer to call our new function.
            FTimerHandle ExitTimerHandle;
            FTimerDelegate TimerDelegate;

            // We use a delegate to pass the 'AgentToExit' parameter to the function that the timer will call.
            TimerDelegate.BindUFunction(this, FName("ExecuteExitForAgent"), AgentToExit);
            GetWorld()->GetTimerManager().SetTimer(ExitTimerHandle, TimerDelegate, ExitDelay, false);
        }
    }
    
    // After dispatching the LAST agent, we must set the state to Idle.
    if (ExitQueue.Num() == 0)
    {
        CurrentState = ESequenceState::Idle;
        UE_LOG(LogTemp, Log, TEXT("Sequencer: All exit commands have been issued. State set to Idle. Waiting for %d agents to finish moving."), PendingMoveCount + 1); // +1 for the one we just dispatched
    }
}

void UAIAgentSequencerComponent::ProcessNextEntry()
{
    if (EntryQueue.Num() > 0)
    {
        if (RlManager && RlManager->GetAgentsInLift().Num() >= RlManager->MaxAgentCapacity)
        {
            
            UE_LOG(LogTemp, Warning, TEXT("Sequencer: Lift is full. Clearing remaining entry queue."));
            EntryQueue.Empty(); // Clear the queue, but don't destroy the agents.
            CurrentState = ESequenceState::Idle;

            UE_LOG(LogTemp, Log, TEXT("Sequencer is now Idle (lift full). Firing OnSequenceFinished event."));
            OnSequenceFinished.Broadcast(); // Fire the event because no agents are moving

            if (RlEnvManager)
            {
                RlEnvManager->StartSimulation(); // Resume the simulation
            }
            return;
        }

        AActionAgent* ActionAgent = Cast<AActionAgent>(EntryQueue.Pop());
        if (!ActionAgent)
        {
            UE_LOG(LogTemp, Error, TEXT("Sequencer tried to process an invalid agent. Skipping."));
            GetWorld()->GetTimerManager().SetTimer(SequenceTimerHandle, this, &UAIAgentSequencerComponent::ProcessNextEntry, 0.1f, false);
            return;
        }

        // --- NEW: Set the entry floor on the agent itself ---
        ActionAgent->SetEnteredOnFloor(this->SequenceCurrentFloor);

        /*FVector SafeLocation;
        TArray<ACharacter*> CharactersToAvoid = RlManager->GetAgentsInLift();
        CharactersToAvoid.Add(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
        UBoxComponent* WalkableArea = RlManager->GetWalkableArea();*/

        // --- WITH THIS NEW LOGIC ---
        FVector FinalSlotLocation = RlManager->FindAndOccupySlot(ActionAgent);
        if (!FinalSlotLocation.IsZero()) 
        {
            // --- THIS IS THE CORRECTED LOGIC ---
            // We now get all three locations and call the updated two-stage entry event.
            if (RlManager && RlManager->GetEntryPoint() && RlManager->GetExitPoint())
            {
                FVector StagingLocation = RlManager->GetEntryPoint()->GetComponentLocation();
                FVector ExitLocation = RlManager->GetExitPoint()->GetComponentLocation();
                
                // Call the restored event with all necessary information
                ActionAgent->ExecuteTwoStageEntry(StagingLocation, FinalSlotLocation, ExitLocation, RlManager->GetOwner());
                UE_LOG(LogTemp, Log, TEXT("Sequencer: Commanding %s to perform two-stage entry and face exit."), *ActionAgent->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Sequencer: Cannot command agent entry, RlManager is missing EntryPoint or ExitPoint!"));
            }
            // --- END OF CORRECTION ---

            PendingMoveCount++;
            RlManager->AttemptEnterLift(ActionAgent);
            SpawnerComponent->AgentSuccessfullyEntered(ActionAgent);
            GetWorld()->GetTimerManager().SetTimer(SequenceTimerHandle, this, &UAIAgentSequencerComponent::ProcessNextEntry, DelayBetweenAgents, false);
        }
        else
        {
            // The logic for when a safe spot cannot be found
            UE_LOG(LogTemp, Warning, TEXT("Sequencer: Could not find a safe spot for %s. Stopping further entries for this sequence."), *ActionAgent->GetName());

            // We no longer destroy the agents here. We just empty the queue to stop the process.
            EntryQueue.Empty();
            CurrentState = ESequenceState::Idle;

            // Check if the sequence should end now.
            if (IsSequenceFinished())
            {
                UE_LOG(LogTemp, Log, TEXT("No pending agents after failing to find spot. Firing OnSequenceFinished event."));
                OnSequenceFinished.Broadcast();

                if (RlEnvManager)
                {
                    RlEnvManager->StartSimulation();
                }
            }
        }
    }
    else
    {
        // All entry queues are processed, sequence is over for now.
        CurrentState = ESequenceState::Idle;
        // The event will be fired by the last agent calling ReportAgentMoveComplete()
    }
}

// This will be called by each agent from its Blueprint when its AI Move To succeeds.
void UAIAgentSequencerComponent::ReportAgentMoveComplete()
{
    UE_LOG(LogTemp, Log, TEXT("Agent move complete. Pending count before: %d"), PendingMoveCount);
    
    if (PendingMoveCount > 0)
    {
        PendingMoveCount--; // An agent has finished its move
    }

    UE_LOG(LogTemp, Log, TEXT("Agent move complete. Pending count after: %d, State: %d"), PendingMoveCount, (int32)CurrentState);

    // Check if the sequencer has finished sending all commands AND if this was the last agent to finish moving
    if (IsSequenceFinished())
    {
        UE_LOG(LogTemp, Log, TEXT("All pending agents have completed their moves. Firing OnSequenceFinished event."));
        OnSequenceFinished.Broadcast();

        /*if (RlEnvManager)
        {
            RlEnvManager->StartSimulation(); // Resume the simulation
        }*/
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Sequence not finished yet. Pending: %d, State: %d"), PendingMoveCount, (int32)CurrentState);
    }
}

// SIMPLIFIED: The sequence is finished when we're in Idle state
bool UAIAgentSequencerComponent::IsSequenceFinished() const
{
    // The sequence is truly finished ONLY when we are in an idle state
    // AND there are no more agents on their way to a target.
    return CurrentState == ESequenceState::Idle && PendingMoveCount == 0;
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "RlManagerComponent.h"
#include "ActionAgent.h"
#include "AIElevatorNavigationComponent.h"
#include "Interfaces/AgentActionsInterface.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/OutputDeviceNull.h"

// Sets default values for this component's properties
URlManagerComponent::URlManagerComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    InteractionState = EAgentInteractionState::None;

    // --- Asset Loading with LOGS ---
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMeshFinder.Succeeded())
    {
        VisualizerMesh = SphereMeshFinder.Object;
        UE_LOG(LogTemp, Log, TEXT("Constructor: Successfully found VisualizerMesh."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Constructor: FAILED to find VisualizerMesh at path /Engine/BasicShapes/Sphere!"));
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Custom/Materials/M_SlotVisualizer"));
    if (MaterialFinder.Succeeded())
    {
        VisualizerMaterial = MaterialFinder.Object;
        UE_LOG(LogTemp, Log, TEXT("Constructor: Successfully found VisualizerMaterial."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Constructor: FAILED to find VisualizerMaterial at path /Game/Custom/Materials/M_SlotVisualizer!"));
    }
}

void URlManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    // --- Auto-find WalkableArea ---
    if (WalkableArea == nullptr)
    {
        TArray<UBoxComponent*> BoxComponents;
        Owner->GetComponents<UBoxComponent>(BoxComponents);
        for (UBoxComponent* Box : BoxComponents)
        {
            if (Box && Box->GetName() == TEXT("WalkableArea"))
            {
                WalkableArea = Box;
                UE_LOG(LogTemp, Log, TEXT("RlManagerComponent: WalkableArea auto-assigned successfully."));
                break;
            }
        }
    }

    // --- Auto-find ExitPoint by name ---
    if (ExitPoint == nullptr)
    {
        TArray<USceneComponent*> SceneComponents;
        Owner->GetComponents<USceneComponent>(SceneComponents);
        for (USceneComponent* SceneComp : SceneComponents)
        {
            // We are looking for a component with the exact name "ExitLocationMarker"
            if (SceneComp && SceneComp->GetName() == TEXT("ExitLocationMarker"))
            {
                ExitPoint = SceneComp;
                UE_LOG(LogTemp, Log, TEXT("RlManagerComponent: ExitPoint auto-assigned successfully to component 'ExitLocationMarker'."));
                break; // Found it, no need to keep searching
            }
        }
    }

    // --- Auto-find EntryPoint by name ---
    if (EntryPoint == nullptr)
    {
        TArray<USceneComponent*> SceneComponents;
        Owner->GetComponents<USceneComponent>(SceneComponents);
        for (USceneComponent* SceneComp : SceneComponents)
        {
            // We are looking for a component with the exact name "EntryLocationMarker"
            if (SceneComp && SceneComp->GetName() == TEXT("EntryLocationMarker"))
            {
                EntryPoint = SceneComp;
                UE_LOG(LogTemp, Log, TEXT("RlManagerComponent: EntryPoint auto-assigned successfully to component 'EntryLocationMarker'."));
                break; // Found it, no need to keep searching
            }
        }
    }

    // --- Auto-find DoorBlockLocationMarker by name ---
    if (DoorBlockLocationMarker == nullptr)
    {
        TArray<USceneComponent*> SceneComponents;
        Owner->GetComponents<USceneComponent>(SceneComponents);
        for (USceneComponent* SceneComp : SceneComponents)
        {
            if (SceneComp && SceneComp->GetName() == TEXT("DoorBlockLocationMarker"))
            {
                DoorBlockLocationMarker = SceneComp;
                UE_LOG(LogTemp, Log, TEXT("RlManagerComponent: DoorBlockLocationMarker auto-assigned successfully."));
                break;
            }
        }
    }

    // --- Validation and Logging ---
    if (WalkableArea == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RlManagerComponent on Actor %s has no WalkableArea! Please add a BoxComponent named 'WalkableArea'."), *Owner->GetName());
    }
    if (ExitPoint == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RlManagerComponent on Actor %s has no ExitPoint! Please add a SceneComponent named 'ExitLocationMarker'."), *Owner->GetName());
    }
    if (EntryPoint == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RlManagerComponent on Actor %s has no EntryPoint! Please add a SceneComponent named 'EntryLocationMarker'."), *Owner->GetName());
    }
    if (DoorBlockLocationMarker == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RlManagerComponent on Actor %s has no DoorBlockLocationMarker Please add a SceneComponent named 'DoorBlockLocationMarker'."), *Owner->GetName())
    }

    // --- Auto-find NavComponent ---
    NavComponent = Owner->FindComponentByClass<UAIElevatorNavigationComponent>();
    if (NavComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RlManagerComponent on Actor %s has no UAIElevatorNavigationComponent! Movement actions will fail."), *Owner->GetName());
    }

    // --- Slot Initialization with LOGS ---
    UE_LOG(LogTemp, Log, TEXT("BeginPlay: Searching for agent slots..."));
    TArray<USceneComponent*> Components;
    Owner->GetComponents(Components);
    for (USceneComponent* Component : Components)
    {
        if (Component && Component->ComponentHasTag(TEXT("ElevatorAgentSlot")))
        {
            UE_LOG(LogTemp, Log, TEXT("BeginPlay: Found tagged slot component: %s"), *Component->GetName());
            AgentSlots.Add(Component, nullptr);
			
            /*// Check if the mesh asset was loaded correctly before trying to use it.
            if (VisualizerMesh)
            {
                UE_LOG(LogTemp, Log, TEXT("BeginPlay: VisualizerMesh is valid, attempting to create sphere for %s."), *Component->GetName());
                UStaticMeshComponent* SphereVisualizer = NewObject<UStaticMeshComponent>(Owner);
                SphereVisualizer->SetStaticMesh(VisualizerMesh);
                SphereVisualizer->SetMaterial(0, VisualizerMaterial);
                SphereVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                SphereVisualizer->AttachToComponent(Component, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
                SphereVisualizer->SetRelativeScale3D(FVector(0.50f));
                SphereVisualizer->RegisterComponent();
				
                //SlotVisualizers.Add(Component, SphereVisualizer);
            }
            else
            {
                // This log will tell us if the problem is the mesh pointer being null.
                UE_LOG(LogTemp, Error, TEXT("BeginPlay: Cannot create sphere for slot %s because VisualizerMesh is NULL! Check constructor logs."), *Component->GetName());
            } */
        }
    }
    UE_LOG(LogTemp, Log, TEXT("RlManagerComponent: Found and initialized %d agent slots."), AgentSlots.Num());

    // --- ADD THIS BLOCK TO SORT THE SLOTS BY NAME FOR PRIORITY ---
    TArray<TObjectPtr<USceneComponent>> TempSlots;
    AgentSlots.GetKeys(TempSlots);
    TempSlots.Sort([](const USceneComponent& A, const USceneComponent& B) {
        return A.GetName().Compare(B.GetName()) < 0;
    });
    SortedAgentSlots = TempSlots;

    UE_LOG(LogTemp, Log, TEXT("Sorted agent slots by name for priority selection."));
    for (const auto& Slot : SortedAgentSlots)
    {
        UE_LOG(LogTemp, Log, TEXT("  - Priority: %s"), *Slot->GetName());
    }
    // --- END OF ADDITION ---
}

bool URlManagerComponent::AttemptEnterLift(ACharacter* AgentToEnter)
{
    if (AgentToEnter == nullptr) return false;

    if (AgentsInLift.Num() < MaxAgentCapacity)
    {
        UE_LOG(LogTemp, Log, TEXT("[RL Manager] Agent %s officially entering roster."), *AgentToEnter->GetName());
        AgentsInLift.Add(AgentToEnter);
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("[RL Manager] Lift is full. Agent %s cannot enter and will be destroyed."), *AgentToEnter->GetName());
    AgentToEnter->Destroy();
    return false;
}

bool URlManagerComponent::AttemptLeaveLift(ACharacter* AgentToLeave)
{
    if (AgentToLeave && AgentsInLift.Contains(AgentToLeave))
    {
        UE_LOG(LogTemp, Log, TEXT("[RL Manager] Agent %s is leaving the roster."), *AgentToLeave->GetName());
        AgentsInLift.Remove(AgentToLeave);
        VacateSlot(AgentToLeave);

        FVector TargetExitLocation;
        if (ExitPoint)
        {
            TargetExitLocation = ExitPoint->GetComponentLocation();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RlManagerComponent: ExitPoint is not set or found! Using fallback location."));
            TargetExitLocation = GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * 150.0f);
        }

        if (AActionAgent* ActionAgent = Cast<AActionAgent>(AgentToLeave))
        {
            ActionAgent->WalkOutOfElevatorAndDestroy(TargetExitLocation, GetOwner());
        }
        return true;
    }
    return false;
}

void URlManagerComponent::RelayActionToRandomAgent(const FString& Action)
{
    // Make sure there are agents in the lift to perform the action.
    if (AgentsInLift.Num() > 0)
    {
        // Select a random agent from the list.
        int32 AgentIndex = FMath::RandRange(0, AgentsInLift.Num() - 1);
        // Ensure the selected agent is our custom ActionAgent class.
        AActionAgent* TargetAgent = Cast<AActionAgent>(AgentsInLift[AgentIndex]);

        if (TargetAgent)
        {
            // Call the new BlueprintImplementableEvent directly on the agent.
            TargetAgent->PerformSocialAction(Action);
            UE_LOG(LogTemp, Log, TEXT("Relayed social action '%s' to agent %s"), *Action, *TargetAgent->GetName());
        }
    }
}

TArray<ACharacter*> URlManagerComponent::GetLIFOAgentsToExit(int32 Count, int32 CurrentFloor)
{
    TArray<ACharacter*> SelectedAgents;
    if (Count <= 0) return SelectedAgents;

    // Iterate backwards through the AgentsInLift array.
    // This array naturally has the last agent that entered at the end.
    for (int32 i = AgentsInLift.Num() - 1; i >= 0; --i)
    {
        ACharacter* AgentInLift = AgentsInLift[i];
        if (AActionAgent* ActionAgent = Cast<AActionAgent>(AgentInLift))
        {
            // Check if the agent is eligible to leave on this floor.
            if (ActionAgent->GetEnteredOnFloor() != CurrentFloor)
            {
                SelectedAgents.Add(ActionAgent);
                UE_LOG(LogTemp, Log, TEXT("GetLIFOAgentsToExit: Selected agent %s (entered on floor %d)"), *ActionAgent->GetName(), ActionAgent->GetEnteredOnFloor());

                // Stop once we have selected the number of agents we need.
                if (SelectedAgents.Num() >= Count)
                {
                    break;
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GetLIFOAgentsToExit: Selected %d agents out of a possible %d."), SelectedAgents.Num(), AgentsInLift.Num());
    return SelectedAgents;
}

// Add the new handler function at the end of the file
void URlManagerComponent::HandleMoveCloserAction()
{
    // --- BUG FIX ---
    // If an agent has already moved and is in the "Away" state, this new action can proceed.
    // We reset the state to None, allowing a new interaction to begin.
    if (InteractionState == EAgentInteractionState::Away)
    {
        ResetInteractionState();
    }
    
    // 1. --- PRE-REQUISITE CHECKS ---
    // Only proceed if no other agent is already interacting.
    if (InteractionState != EAgentInteractionState::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleMoveCloserAction: Aborted, an agent is already interacting (state: %d)."), (int32)InteractionState);
        return;
    }
    if (AgentsInLift.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleMoveCloserAction: Aborted, no agents in lift."));
        return;
    }
    if (!NavComponent || !WalkableArea)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleMoveCloserAction: Aborted, missing NavComponent or WalkableArea."));
        return;
    }
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleMoveCloserAction: Aborted, cannot find PlayerCharacter."));
        return;
    }
    
    // --- 2. PRIORITY-BASED AGENT SELECTION ---
    AActionAgent* AgentToMove = nullptr;
    UE_LOG(LogTemp, Log, TEXT("HandleMoveCloserAction: Searching for highest-priority agent..."));
    
    // Iterate through the sorted slots to find the first occupied one.
    for (USceneComponent* Slot : SortedAgentSlots)
    {
        // Find if an agent is occupying this slot
        /*if (AgentSlots.Contains(Slot) && IsValid(AgentSlots[Slot].Get()))*/
        if (AgentSlots.Contains(Slot) && IsValid(AgentSlots[Slot].Get()))
        {
            AgentToMove = Cast<AActionAgent>(AgentSlots[Slot].Get());
            UE_LOG(LogTemp, Log, TEXT("HandleMoveCloserAction: Selected agent %s from highest-priority slot %s."), *AgentToMove->GetName(), *Slot->GetName());
            break; // Found our agent, stop searching.
        }
    }

    // If no agent was found (e.g., all slots are empty), abort.
    /*if (!AgentToMove)
    {
        UE_LOG(LogTemp, Log, TEXT("HandleMoveCloserAction: No agent found within the required distance range (%.1f - %.1f cm) to move closer."), MinDistanceToPlayerForCloseMove, MaxDistanceToPlayerForCloseMove);
        return;
    }*/

    if (!AgentToMove)
    {
        UE_LOG(LogTemp, Log, TEXT("HandleMoveCloserAction: No agent found within the required distance range to move closer."));
        return;
    }

    // 3. --- STORE THE AGENT AND ITS ORIGINAL SLOT ---
    USceneComponent* OriginalSlot = FindSlotForAgent(AgentToMove);
    if (!OriginalSlot)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleMoveCloserAction: Could not find original slot for agent %s!"), *AgentToMove->GetName());
        return;
    }

    OriginalSlotOfInteractingAgent = OriginalSlot;
    AgentInteractingWithPlayer = AgentToMove;

    UE_LOG(LogTemp, Log, TEXT("HandleMoveCloserAction: Selected agent %s from slot %s to move closer."), *AgentToMove->GetName(), *OriginalSlot->GetName());
    
    // 4. --- FIND A SAFE DESTINATION NEAR THE PLAYER (this part is the same as before) ---
    TArray<ACharacter*> CharactersToAvoid;
    CharactersToAvoid.Add(PlayerCharacter);
    for (ACharacter* Agent : AgentsInLift)
    {
        if (Agent != AgentToMove) CharactersToAvoid.Add(Agent);
    }

    FVector DesiredLocation = FMath::VInterpTo(AgentToMove->GetActorLocation(), PlayerCharacter->GetActorLocation(), 0.5f, 1.0f);

    // Force the desired location's Z-height to match the agent's current height BEFORE finding a nav point.
    DesiredLocation.Z = AgentToMove->GetActorLocation().Z;
    
    //DrawDebugSphere(GetWorld(), DesiredLocation, 30.f, 12, FColor::Magenta, false, 5.0f);

    FVector SafeLocation;
    if (NavComponent->FindSafeLocationNearTarget(WalkableArea, CharactersToAvoid, DesiredLocation, SafeLocation))
    {
        UE_LOG(LogTemp, Log, TEXT("HandleMoveCloserAction: Commanding agent to move to safe location near player."));
        // We pass 'false' for bShouldTurnOnArrival because the agent should keep facing the player.
        FVector LookAtLocation = ExitPoint ? ExitPoint->GetComponentLocation() : GetOwner()->GetActorLocation();
        AgentToMove->MoveAgentToLocation(SafeLocation, LookAtLocation, false, GetOwner());
        InteractionState = EAgentInteractionState::Close;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleMoveCloserAction: Could not find a safe location for agent %s to move closer."), *AgentToMove->GetName());
        ResetInteractionState(); // Failed, so reset state immediately.
    }
}

void URlManagerComponent::HandleMoveAwayAction()
{
    // 1. --- STATE AND PREREQUISITE CHECKS ---
    // Only proceed if an agent has previously moved "Close" and hasn't already moved "Away".
    if (InteractionState != EAgentInteractionState::Close)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleMoveAwayAction: Aborted. InteractionState is not 'Close' (current: %d)."), (int32)InteractionState);
        return;
    }
    if (!AgentInteractingWithPlayer || !OriginalSlotOfInteractingAgent)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleMoveAwayAction: Aborted. Interacting agent or its original slot is invalid!"));
        ResetInteractionState();
        return;
    }
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (!PlayerCharacter || !NavComponent || !WalkableArea)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleMoveAwayAction: Aborted, missing Player, NavComponent, or WalkableArea."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("HandleMoveAwayAction: State checks passed. Interacting agent is '%s'."), *AgentInteractingWithPlayer->GetName());

    // 2. --- GET DESTINATION FROM SAVED SLOT AND COMMAND MOVE ---
    FVector TargetLocation = OriginalSlotOfInteractingAgent->GetComponentLocation();
    FVector AgentLocation = AgentInteractingWithPlayer->GetActorLocation();
    // Force the target's Z-height to match the agent's current height.
    TargetLocation.Z = AgentLocation.Z;
    
    UE_LOG(LogTemp, Log, TEXT("HandleMoveAwayAction: Commanding agent %s to return to original slot location: %s"), *AgentInteractingWithPlayer->GetName(), *TargetLocation.ToString());
    // We pass 'true' for bShouldTurnOnArrival because the agent is returning to its slot.
    FVector LookAtLocation = ExitPoint ? ExitPoint->GetComponentLocation() : GetOwner()->GetActorLocation();
    AgentInteractingWithPlayer->MoveAgentToLocation(TargetLocation, LookAtLocation, true, GetOwner());
    
    // --- LOGIC CHANGE ---
    // Instead of resetting completely, we set the state to "Away".
    // This signifies the interaction is over, but blocks a new "move_closer" until the state is properly reset.
    InteractionState = EAgentInteractionState::Away;
    UE_LOG(LogTemp, Log, TEXT("HandleMoveAwayAction: State set to 'Away'."));
}

void URlManagerComponent::ResetInteractionState()
{
    UE_LOG(LogTemp, Log, TEXT("Resetting agent interaction state."));

    // --- ADD THIS LOGIC ---
    // If there was a busy agent, call the new reset event on it before clearing the reference.
    if (IsValid(AgentInteractingWithPlayer))
    {
        AgentInteractingWithPlayer->ResetActionState();
    }
    // --- END OF ADDITION ---
    
    AgentInteractingWithPlayer = nullptr;
    OriginalSlotOfInteractingAgent = nullptr; // <-- Add this line
    InteractionState = EAgentInteractionState::None;
}

void URlManagerComponent::ReboardFailedAgent(ACharacter* AgentToReboard)
{
    if (AgentToReboard && !AgentsInLift.Contains(AgentToReboard))
    {
        UE_LOG(LogTemp, Warning, TEXT("[RL Manager] Agent %s failed to exit and is being re-boarded."), *AgentToReboard->GetName());
        AgentsInLift.Add(AgentToReboard);
    }
}

FVector URlManagerComponent::FindAndOccupySlot(ACharacter* Agent)
{
    if (!Agent) return FVector::ZeroVector;

    // Find the first available slot (where the value is nullptr)
    for (auto& SlotPair : AgentSlots)
    {
        if (SlotPair.Value == nullptr) // Check if the slot is free
        {
            SlotPair.Value = Agent; // Assign the agent to this slot
            UE_LOG(LogTemp, Log, TEXT("Agent %s assigned to slot %s."), *Agent->GetName(), *SlotPair.Key->GetName());
            return SlotPair.Key->GetComponentLocation(); // Return the location of the slot
        }
    }

    // If we get here, no free slots were found
    UE_LOG(LogTemp, Warning, TEXT("Could not find a free agent slot for %s."), *Agent->GetName());
    return FVector::ZeroVector;
}

void URlManagerComponent::VacateSlot(ACharacter* Agent)
{
    if (!Agent) return;

    // Find the agent in the map and set its value back to nullptr
    for (auto& SlotPair : AgentSlots)
    {
        if (SlotPair.Value == Agent)
        {
            UE_LOG(LogTemp, Log, TEXT("Agent %s is vacating slot %s."), *Agent->GetName(), *SlotPair.Key->GetName());
            SlotPair.Value = nullptr; // Mark the slot as free
            return; // Exit once found
        }
    }
}

// In TickComponent(), UPDATE the sphere colors.
// Replace your entire existing TickComponent function with this new version.
void URlManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Loop through all agent slots and update the color of the visible sphere
    /*for (const auto& SlotPair : AgentSlots)
    {
        // Find the visualizer mesh component for this slot
        if (UStaticMeshComponent* Visualizer = SlotVisualizers.FindRef(SlotPair.Key))
        {
            // Get or create a dynamic material instance to change its parameters
            UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(Visualizer->GetMaterial(0));
            if (!DynMaterial)
            {
                // Create one if it doesn't exist yet
                DynMaterial = UMaterialInstanceDynamic::Create(VisualizerMaterial, this);
                Visualizer->SetMaterial(0, DynMaterial);
            }

            // Set the color based on whether the slot is occupied
            if (DynMaterial)
            {
                FColor SphereColor = (SlotPair.Value == nullptr) ? FColor::Green : FColor::Red;
                DynMaterial->SetVectorParameterValue(TEXT("Color"), SphereColor);
            }
        }
    }*/
}

USceneComponent* URlManagerComponent::FindSlotForAgent(ACharacter* Agent) const
{
    if (!Agent) return nullptr;

    for (const auto& SlotPair : AgentSlots)
    {
        if (SlotPair.Value == Agent)
        {
            return SlotPair.Key;
        }
    }
    return nullptr;
}

void URlManagerComponent::HandleBlockDoorAction()
{
    // 1. --- Prerequisite Checks ---
    if (InteractionState != EAgentInteractionState::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleBlockDoorAction: Aborted, an agent is already interacting (state: %d)."), (int32)InteractionState);
        return;
    }
    if (!DoorBlockLocationMarker)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleBlockDoorAction: Aborted, DoorBlockLocationMarker is not set!"));
        return;
    }
    if (AgentsInLift.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleBlockDoorAction: Aborted, no agents in lift."));
        return;
    }

    // --- 2. NEW: Conditional Agent Selection Logic ---
    AActionAgent* AgentToMove = nullptr;
    const bool bIsElevatorFull = (AgentsInLift.Num() >= AgentSlots.Num() && AgentSlots.Num() > 0);

    UE_LOG(LogTemp, Log, TEXT("HandleBlockDoorAction: Elevator is %s."), bIsElevatorFull ? TEXT("considered FULL") : TEXT("NOT full"));

    // If the elevator is full, prioritize the agent in the LAST slot.
    if (bIsElevatorFull)
    {
        UE_LOG(LogTemp, Log, TEXT("...Elevator is full. Attempting to select agent from the last priority slot."));
        if (SortedAgentSlots.Num() > 0)
        {
            // Get the last slot from our priority-sorted array.
            USceneComponent* LastSlot = SortedAgentSlots.Last();
            if (AgentSlots.Contains(LastSlot) && IsValid(AgentSlots[LastSlot].Get()))
            {
                AgentToMove = Cast<AActionAgent>(AgentSlots[LastSlot].Get());
                UE_LOG(LogTemp, Log, TEXT("...Selected agent %s from last slot %s."), *AgentToMove->GetName(), *LastSlot->GetName());
            }
        }
    }

    // If no agent has been selected yet (either because the lift wasn't full,
    // or the last slot was unexpectedly empty), fall back to the normal priority selection.
    if (!AgentToMove)
    {
        UE_LOG(LogTemp, Log, TEXT("...Falling back to normal priority selection (highest-priority agent)."));
        for (USceneComponent* Slot : SortedAgentSlots)
        {
            if (AgentSlots.Contains(Slot) && IsValid(AgentSlots[Slot].Get()))
            {
                AgentToMove = Cast<AActionAgent>(AgentSlots[Slot].Get());
                UE_LOG(LogTemp, Log, TEXT("...Selected agent %s from highest-priority slot %s."), *AgentToMove->GetName(), *Slot->GetName());
                break; // Found the first available agent, stop searching.
            }
        }
    }

    // If we still haven't found an agent, abort.
    if (!AgentToMove)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleBlockDoorAction: Aborted, could not find any agent to perform the action."));
        return;
    }

    // 3. --- Command the Agent to Move ---
    FVector TargetLocation = DoorBlockLocationMarker->GetComponentLocation();
    // Match the agent's current height to ensure reliable navigation
    TargetLocation.Z = AgentToMove->GetActorLocation().Z;

    UE_LOG(LogTemp, Log, TEXT("HandleBlockDoorAction: Commanding agent %s to block the door."), *AgentToMove->GetName());

    // Set the state and store the interacting agent
    InteractionState = EAgentInteractionState::BlockingDoor;
    AgentInteractingWithPlayer = AgentToMove; // We use the same variable to track the active agent
    
    // We pass 'true' for bShouldTurnOnArrival because the agent is returning to its slot.
    /*FVector LookAtLocation = ExitPoint ? ExitPoint->GetComponentLocation() : GetOwner()->GetActorLocation();
    AgentInteractingWithPlayer->MoveAgentToLocation(TargetLocation, LookAtLocation, true, GetOwner());*/

    // --- THIS LINE WAS CORRECTED ---
    // The call now correctly targets the 'AgentToMove' that was selected.
    FVector LookAtLocation = ExitPoint ? ExitPoint->GetComponentLocation() : GetOwner()->GetActorLocation();
    AgentToMove->MoveAgentToLocation(TargetLocation, LookAtLocation, true, GetOwner());
	
}

void URlManagerComponent::ReportInteractiveMoveComplete(ACharacter* ReportingAgent)
{
    if (!ReportingAgent) return;

    // This log message will be our proof that a move was successfully completed.
    UE_LOG(LogTemp, Warning, TEXT("REPORT RECEIVED: Agent '%s' has successfully completed its interactive move."), *ReportingAgent->GetName());
}

void URlManagerComponent::ReportInteractiveMoveFailed(ACharacter* ReportingAgent)
{
    if (!ReportingAgent) return;

    // This red error message will clearly tell us when a move has failed.
    UE_LOG(LogTemp, Error, TEXT("REPORT RECEIVED: Agent '%s' has FAILED its interactive move. The AI could not find a path to the destination."), *ReportingAgent->GetName());

    // NEW LOGIC: Only reset the state if the agent that failed is the one
    // that the system currently thinks is busy. This prevents a "stuck" state.
    if (ReportingAgent == AgentInteractingWithPlayer)
    {
        UE_LOG(LogTemp, Error, TEXT("The failed agent was the interacting agent. Resetting interaction state to allow new commands."));
        ResetInteractionState();
    }
}

void URlManagerComponent::SetStareState(bool bIsStaring)
{
    AActionAgent* AgentToStare = nullptr;
    UE_LOG(LogTemp, Log, TEXT("SetStareState: Searching for an available agent to perform stare action."));

    if (InteractionState != EAgentInteractionState::None && IsValid(AgentInteractingWithPlayer))
    {
        UE_LOG(LogTemp, Log, TEXT("... An agent (%s) is already busy with state: %d. Will attempt to find a different agent."), 
            *AgentInteractingWithPlayer->GetName(), (int32)InteractionState);
    }

    // Find the first available, non-busy agent.
    for (USceneComponent* Slot : SortedAgentSlots)
    {
        if (AgentSlots.Contains(Slot) && IsValid(AgentSlots[Slot].Get()))
        {
            AActionAgent* CandidateAgent = Cast<AActionAgent>(AgentSlots[Slot].Get());
            if (CandidateAgent)
            {
                if (CandidateAgent == AgentInteractingWithPlayer)
                {
                    UE_LOG(LogTemp, Log, TEXT("... Skipping agent %s because they are currently performing an interactive move."), *CandidateAgent->GetName());
                    continue; 
                }
                AgentToStare = CandidateAgent;
                UE_LOG(LogTemp, Log, TEXT("SetStareState: Selected available agent %s for stare action."), *AgentToStare->GetName());
                break; 
            }
        }
    }
    
    // --- THIS IS THE KEY CHANGE ---
    // We now broadcast the event with BOTH the selected agent and the true/false value.
    OnStareStateChanged.Broadcast(AgentToStare, bIsStaring);
    // --- END OF CHANGE ---

    // Command the selected agent (only if staring is true, as the stare command has its own body turn)
    if (AgentToStare && bIsStaring)
    {
        FOutputDeviceNull ar; 
        AgentToStare->CallFunctionByNameWithArguments(TEXT("ExecuteStareCommand"), ar, NULL, true);
    }
}

AActionAgent* URlManagerComponent::GetAgentInteractingWithPlayer() const
{
    // Simply returns the private member variable.
    // It's good practice to use IsValid to ensure the agent hasn't been destroyed.
    if (IsValid(AgentInteractingWithPlayer))
    {
        return AgentInteractingWithPlayer;
    }
    return nullptr;
}
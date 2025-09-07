// AgentSpawnerComponent.cpp

#include "AgentSpawnerComponent.h"
#include "AgentSpawnPoint.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RLEnvironmentManager.h"
#include "DrawDebugHelpers.h"
#include "NavigationSystem.h"

UAgentSpawnerComponent::UAgentSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAgentSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
    RlEnvManager = GetOwner()->FindComponentByClass<URLEnvironmentManager>();
}

void UAgentSpawnerComponent::CleanupAllWaitingAgentsInternal()
{
    for (ACharacter* Agent : WaitingAgents)
    {
        if (IsValid(Agent))
        {
            Agent->Destroy();
        }
    }
    WaitingAgents.Empty();
}

void UAgentSpawnerComponent::CleanupAgentsForFloor(int32 CurrentFloor)
{
    if (CurrentFloor == 0)
    {
        return;
    }
    CleanupAllWaitingAgentsInternal();
}

void UAgentSpawnerComponent::PreSpawnAgentsForFloor(int32 FloorNumber)
{
    // THIS BLOCK TO DISABLE SPAWNING FOR THE BASELINE GROUP
    if (RlEnvManager && RlEnvManager->GetCurrentExperimentalGroup() == EExperimentalGroup::BaselineControl)
    {
        UE_LOG(LogTemp, Log, TEXT("AgentSpawner: Baseline Control group is active. Skipping agent spawning."));
        return;
    }

    if (RlEnvManager && RlEnvManager->IsSimulationFinished()) return;
    if (FloorNumber == 0) return;
    if (AgentClassesToSpawn.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("AgentSpawner: AgentClassesToSpawn array is empty!"));
        return;
    }
    UWorld* World = GetWorld();
    if (!World) return;

    // --- Step 1: Find the SINGLE spawn point for the target floor ---
    TArray<AActor*> AllSpawnPointsActors;
    UGameplayStatics::GetAllActorsOfClass(World, AAgentSpawnPoint::StaticClass(), AllSpawnPointsActors);
    AAgentSpawnPoint* FloorSpawnPoint = nullptr;
    for (AActor* Actor : AllSpawnPointsActors)
    {
        if (AAgentSpawnPoint* SpawnPoint = Cast<AAgentSpawnPoint>(Actor))
        {
            if (SpawnPoint->FloorNumber == FloorNumber)
            {
                FloorSpawnPoint = SpawnPoint;
                break;
            }
        }
    }

    if (!FloorSpawnPoint)
    {
        UE_LOG(LogTemp, Error, TEXT("Spawn Error: No spawn point found for floor %d."), FloorNumber);
        return;
    }
    
    // --- Step 2: Decide how many agents to spawn ---
    int32 NumToSpawn = FMath::RandRange(1, MaxAgentsToSpawn);
    UE_LOG(LogTemp, Log, TEXT("AgentSpawner: Attempting to spawn %d agents around the point for floor %d."), NumToSpawn, FloorNumber);

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        UE_LOG(LogTemp, Error, TEXT("AgentSpawner: Navigation System not found. Cannot spawn agents."));
        return;
    }

    FVector CenterPoint = FloorSpawnPoint->GetActorLocation();
    TArray<FVector> SpawnedLocations;

    // --- Step 3: Loop for each agent we want to spawn ---
    for (int32 i = 0; i < NumToSpawn; ++i)
    {
        bool bSuccessfullySpawned = false;
        int32 Attempts = 0;

        while (Attempts < 50 && !bSuccessfullySpawned)
        {
            Attempts++;
            
            // --- THIS IS THE KEY FIX ---
            // The variable must be of type FNavLocation, not FVector.
            FNavLocation RandomNavLocation; 
            
            if (NavSys->GetRandomReachablePointInRadius(CenterPoint, SpawnRadius, RandomNavLocation))
            {
                FVector RandomPoint = RandomNavLocation.Location; // Get the FVector from the FNavLocation
                //DrawDebugSphere(GetWorld(), RandomPoint, 25.f, 12, FColor::Yellow, false, 15.0f);

                bool bIsLocationFarEnough = true;
                for (const FVector& SpawnedLocation : SpawnedLocations)
                {
                    if (FVector::Dist(RandomPoint, SpawnedLocation) < MinDistanceBetweenSpawns)
                    {
                        bIsLocationFarEnough = false;
                        //DrawDebugSphere(GetWorld(), RandomPoint, 30.f, 12, FColor::Red, false, 15.0f);
                        break;
                    }
                }

                if (bIsLocationFarEnough)
                {
                    int32 RandomClassIndex = FMath::RandRange(0, AgentClassesToSpawn.Num() - 1);
                    if (TSubclassOf<ACharacter> CharacterClassToSpawn = AgentClassesToSpawn[RandomClassIndex])
                    {
                        FTransform SpawnTransform = FloorSpawnPoint->GetActorTransform();
                        SpawnTransform.SetLocation(RandomPoint); // Use the corrected FVector
                        
                        if (ACharacter* NewCharacter = World->SpawnActor<ACharacter>(CharacterClassToSpawn, SpawnTransform))
                        {
                            WaitingAgents.Add(NewCharacter);
                            SpawnedLocations.Add(RandomPoint);
                            bSuccessfullySpawned = true;
                            //DrawDebugSphere(GetWorld(), RandomPoint, 35.f, 12, FColor::Green, false, 20.0f);
                        }
                    }
                }
            }
        } 

        if (!bSuccessfullySpawned)
        {
            UE_LOG(LogTemp, Warning, TEXT("AgentSpawner: Could not find a valid spot for agent #%d after 50 attempts."), i + 1);
            break;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AgentSpawner: Successfully spawned %d agents."), SpawnedLocations.Num());
}

TArray<ACharacter*> UAgentSpawnerComponent::GetWaitingAgents() const
{
    return WaitingAgents;
}

void UAgentSpawnerComponent::AgentSuccessfullyEntered(ACharacter* Agent)
{
    if (Agent)
    {
        WaitingAgents.Remove(Agent);
    }
}
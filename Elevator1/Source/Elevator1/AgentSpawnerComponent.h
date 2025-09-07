// AgentSpawnerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AgentSpawnerComponent.generated.h"

// Forward declaration for ACharacter
class ACharacter;
class AAgentSpawnPoint;
class URLEnvironmentManager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ELEVATOR1_API UAgentSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAgentSpawnerComponent();


	/**
	 * Destroys all agents currently waiting outside the elevator.
	 * This function will automatically do nothing if the floor number is 0.
	 * @param CurrentFloor The floor the elevator is currently at.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Spawner")
	void CleanupAgentsForFloor(int32 CurrentFloor); // <-- RENAMED and now takes a parameter

	/**
	 * Spawns a new random group of agents for a given floor.
	 * This function will automatically do nothing if the floor number is 0.
	 * @param CharacterClass - The specific character Blueprint you want to spawn (e.g., BP_Hana).
	 * @param FloorNumber - The floor number to pre-spawn the new group on.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI Spawner")
	void PreSpawnAgentsForFloor(int32 FloorNumber);

	/** Returns the list of agents currently waiting outside. For C++ components to use. */
	TArray<ACharacter*> GetWaitingAgents() const;

	/** Called by the sequencer when an agent is officially inside the lift, removing it from the waiting list. */
	void AgentSuccessfullyEntered(ACharacter* Agent);

	/** The maximum number of agents that can be randomly spawned for a floor. Defaults to 5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawner", meta = (ClampMin = "1"))
	int32 MaxAgentsToSpawn = 5;

	/** The list of possible MetaHuman Blueprints that can be spawned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawner")
	TArray<TSubclassOf<ACharacter>> AgentClassesToSpawn;

	/** The minimum distance (in cm) required between two agents when they are spawned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawner|Spacing")
	float MinDistanceBetweenSpawns = 100.0f; // Default to 1 meter

	/** The radius around the single spawn point in which to find random locations for agents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Spawner|Spacing")
	float SpawnRadius = 300.0f; // Default to a 3-meter radius

protected:
	virtual void BeginPlay() override;

private:
	/** Internal function to destroy all agents in the WaitingAgents array. */
	void CleanupAllWaitingAgentsInternal();

	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> WaitingAgents;

	UPROPERTY()
	TObjectPtr<URLEnvironmentManager> RlEnvManager;
};
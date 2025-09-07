// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AgentSpawnPoint.generated.h"

UCLASS()
class ELEVATOR1_API AAgentSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAgentSpawnPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// The floor this spawn point belongs to
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SpawnPoint")
	int32 FloorNumber = 0;
};

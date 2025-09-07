// Fill out your copyright notice in the Description page of Project Settings.


#include "AgentSpawnPoint.h"

// Sets default values
AAgentSpawnPoint::AAgentSpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AAgentSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAgentSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


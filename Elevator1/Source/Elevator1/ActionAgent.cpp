// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionAgent.h"

// Sets default values
AActionAgent::AActionAgent()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AActionAgent::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AActionAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AActionAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


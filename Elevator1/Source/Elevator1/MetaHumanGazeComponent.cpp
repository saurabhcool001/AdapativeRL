// Fill out your copyright notice in the Description page of Project Settings.

#include "MetaHumanGazeComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UMetaHumanGazeComponent::UMetaHumanGazeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default values
	LookAtLocation = FVector::ZeroVector;
	TargetActor = nullptr;
	LookAtHeightOffset = 75.0f;
	FullHeadTurnDistance = 100.0f; // Head is fully engaged at 1.5 meters
	EyesOnlyDistance = 100.0f;     // Head stops turning at 4 meters
	HeadLookAtAlpha = 0.0f;
}

// Called when the game starts
void UMetaHumanGazeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetActor)
	{
		TargetActor = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}

	AActor* Owner = GetOwner();
	if (Owner)
	{
		LookAtLocation = Owner->GetActorLocation() + (Owner->GetActorForwardVector() * 1000.0f);
	}
}

void UMetaHumanGazeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateLookAtLogic(DeltaTime);
}

void UMetaHumanGazeComponent::UpdateLookAtLogic(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	FVector TargetLocation;

	if (TargetActor)
	{
		TargetLocation = TargetActor->GetActorLocation();
		TargetLocation.Z += LookAtHeightOffset;

		// Calculate distance between owner and target
		const float Distance = FVector::Dist(Owner->GetActorLocation(), TargetActor->GetActorLocation());

		// Map the distance to a 0-1 alpha value.
		// When distance is far (>= EyesOnlyDistance), alpha is 0.
		// When distance is close (<= FullHeadTurnDistance), alpha is 1.
		// It transitions smoothly in between.
		HeadLookAtAlpha = FMath::GetMappedRangeValueClamped(
			FVector2D(EyesOnlyDistance, FullHeadTurnDistance),
			FVector2D(0.0f, 1.0f),
			Distance
		);
	}
	else
	{
		TargetLocation = Owner->GetActorLocation() + (Owner->GetActorForwardVector() * 1000.0f);
		HeadLookAtAlpha = 0.0f; // No target, so no head turning
	}

	LookAtLocation = TargetLocation;
}
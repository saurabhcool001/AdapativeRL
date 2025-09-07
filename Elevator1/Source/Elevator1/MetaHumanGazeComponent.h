// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MetaHumanGazeComponent.generated.h"

class AActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ELEVATOR1_API UMetaHumanGazeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMetaHumanGazeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// The Actor that the MetaHuman should look at.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze Settings")
	AActor* TargetActor;

	// How high to look on the target actor (in cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze Settings")
	float LookAtHeightOffset;

	// The distance at which the head is fully turned to the target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze Settings")
	float FullHeadTurnDistance;

	// The distance at which the head stops turning and only the eyes track.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gaze Settings")
	float EyesOnlyDistance;

	// The location for the eyes and head to look at.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Gaze Debug")
	FVector LookAtLocation;

	// The calculated alpha (0-1) for how much the head should turn. Read by the Anim BP.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Gaze Debug")
	float HeadLookAtAlpha;

private:
	void UpdateLookAtLogic(float DeltaTime);
};
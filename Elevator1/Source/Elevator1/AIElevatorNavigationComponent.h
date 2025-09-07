// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIElevatorNavigationComponent.generated.h"

class UBoxComponent;
class ACharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ELEVATOR1_API UAIElevatorNavigationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UAIElevatorNavigationComponent();

    /**
         * Finds a valid, safe location inside a defined area, avoiding proximity to other characters.
         * @param WalkableArea - The Box Component defining the zone where agents can stand.
         * @param OtherCharacters - An array of all other characters to avoid (the player and other agents).
         * @param OutLocation - The safe location that was found.
         * @param MinSafeDistance - The minimum personal space required around each character.
         * @param MaxAttempts - How many times to try finding a spot before giving up.
         * @return True if a safe location was found, false otherwise.
         */
    UFUNCTION(BlueprintCallable, Category = "AI Navigation")
    bool FindSafeLocationInElevator(
        UBoxComponent* WalkableArea,
        const TArray<ACharacter*>& OtherCharacters,
        FVector& OutLocation,
        float MinSafeDistance = 7.0f,
        int32 MaxAttempts = 20
    );
    
    UFUNCTION(BlueprintCallable, Category = "AI Navigation")
    bool FindSafeLocationNearTarget(
        UBoxComponent* WalkableArea,
        const TArray<ACharacter*>& OtherCharacters,
        const FVector& TargetLocation,
        FVector& OutLocation,
        float SearchRadius = 150.0f,
        float MinSafeDistance = 20.0f,
        int32 MaxAttempts = 20
    );
};
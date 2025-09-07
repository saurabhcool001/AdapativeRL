// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AgentActionsInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class UAgentActionsInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for defining actions that an RL-controlled agent can perform.
 */
class ELEVATOR1_API IAgentActionsInterface
{
    GENERATED_BODY()

    // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    // All functions below are declared as BlueprintNativeEvents.
    // This means they require a C++ implementation function (with an _Implementation suffix)
    // but can be overridden with logic in a Blueprint.

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Agent Actions")
    void WalkIntoElevator();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Agent Actions")
    void WalkOutOfElevatorAndDestroy();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Agent Actions")
    void PerformSocialAction(const FString& Action);

    // --- ADD THIS NEW FUNCTION ---
    // This function will be called to tell an agent to move to a specific point.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Agent Actions")
    void WalkToLocation(const FVector& TargetLocation);
};
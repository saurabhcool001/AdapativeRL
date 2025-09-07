// Fill out your copyright notice in the Description page of Project Settings.


#include "AIElevatorNavigationComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UAIElevatorNavigationComponent::UAIElevatorNavigationComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    // ...
}


bool UAIElevatorNavigationComponent::FindSafeLocationInElevator(
    UBoxComponent* WalkableArea,
    const TArray<ACharacter*>& OtherCharacters,
    FVector& OutLocation,
    float MinSafeDistance,
    int32 MaxAttempts)
{
    if (!WalkableArea || !GetWorld()) return false;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return false;

    // --- NEW: Calculate a smarter target zone away from the door ---
    // This logic pushes the search area towards the back of the elevator to clear the doorway.

    // We push the center of our search area forward into the elevator (assuming +X is forward).
    const float ForwardOffset = 75.0f;
    FVector TargetZoneCenter = WalkableArea->GetComponentLocation() + (WalkableArea->GetForwardVector() * ForwardOffset);

    // We also shrink the search area slightly to avoid the extreme edges.
    const float ShrinkAmount = 50.0f;
    FVector TargetZoneExtent = WalkableArea->GetScaledBoxExtent() - FVector(ShrinkAmount, ShrinkAmount, 0);


    for (int32 i = 0; i < MaxAttempts; ++i)
    {
        // 1. Get a random point inside our new, smarter target zone.
        FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(
            TargetZoneCenter,
            TargetZoneExtent
        );

        // 2. Find the closest walkable point on the NavMesh near our random point.
        FNavLocation ProjectedPoint;
        bool bProjected = NavSys->GetRandomPointInNavigableRadius(RandomPoint, 100.0f, ProjectedPoint);

        if (bProjected)
        {
            bool bIsLocationSafe = true;
            // 3. Check this point against ALL other characters (player and other agents).
            for (ACharacter* OtherCharacter : OtherCharacters)
            {
                if (IsValid(OtherCharacter))
                {
                    // If the distance is less than our personal space bubble, this spot is not safe.
                    if (FVector::Dist(ProjectedPoint.Location, OtherCharacter->GetActorLocation()) < MinSafeDistance)
                    {
                        bIsLocationSafe = false;
                        break; // No need to check other characters; we've already failed.
                    }
                }
            }

            // 4. If the location passed all safety checks, we've found our spot!
            if (bIsLocationSafe)
            {
                OutLocation = ProjectedPoint.Location;
                return true;
            }
        }
    }

    // If we finished all attempts and couldn't find a safe spot, return false.
    return false;
}

bool UAIElevatorNavigationComponent::FindSafeLocationNearTarget(
    UBoxComponent* WalkableArea,
    const TArray<ACharacter*>& OtherCharacters,
    const FVector& TargetLocation,
    FVector& OutLocation,
    float SearchRadius,
    float MinSafeDistance,
    int32 MaxAttempts)
{
    if (!WalkableArea || !GetWorld()) return false;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return false;

    // --- NEW: Variables to track the best location found so far ---
    bool bFoundAtLeastOneSafeLocation = false;
    FVector BestLocationSoFar = FVector::ZeroVector;
    double BestDistanceSq = -1.0; // Using squared distance is more efficient

    for (int32 i = 0; i < MaxAttempts; ++i)
    {
        FNavLocation ProjectedPoint;
        if (NavSys->GetRandomPointInNavigableRadius(TargetLocation, SearchRadius, ProjectedPoint))
        {
            bool bIsLocationSafe = true;
            for (ACharacter* OtherCharacter : OtherCharacters)
            {
                if (IsValid(OtherCharacter))
                {
                    if (FVector::DistSquared(ProjectedPoint.Location, OtherCharacter->GetActorLocation()) < FMath::Square(MinSafeDistance))
                    {
                        bIsLocationSafe = false;
                        break;
                    }
                }
            }

            // --- NEW: If the location is safe, check if it's the best one yet ---
            if (bIsLocationSafe)
            {
                bFoundAtLeastOneSafeLocation = true;
                double DistanceSqToTarget = FVector::DistSquared(ProjectedPoint.Location, TargetLocation);

                // If this is the first safe spot, or if it's closer than our previous best, store it.
                if (BestDistanceSq < 0 || DistanceSqToTarget < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSqToTarget;
                    BestLocationSoFar = ProjectedPoint.Location;
                }
            }
        }
    }

    // --- NEW: After all attempts, return the best location we found ---
    if (bFoundAtLeastOneSafeLocation)
    {
        OutLocation = BestLocationSoFar;
        // For debugging, draw a green sphere at the chosen best location
        //DrawDebugSphere(GetWorld(), OutLocation, 25.f, 12, FColor::Green, false, 5.0f);
        return true;
    }

    // If we couldn't find any safe locations after all attempts, return false.
    UE_LOG(LogTemp, Warning, TEXT("FindSafeLocationNearTarget failed to find any safe spot after %d attempts."), MaxAttempts);
    return false;
}
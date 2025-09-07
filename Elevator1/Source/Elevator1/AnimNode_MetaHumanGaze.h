// Copyright (C) 2024, Some C++ Maste. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_MetaHumanGaze.generated.h"

/**
 * Custom Animation Node to control MetaHuman head and eye gaze based on a target location.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "MetaHuman Gaze")) // This DisplayName is the fix!
struct ELEVATOR1_API FAnimNode_MetaHumanGaze : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	// The world-space location to look at. This will be an input pin on the node.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target", meta = (PinShownByDefault))
	FVector LookAtLocation;

	// Alpha for how much the head should turn (0=eyes only, 1=full head turn). This is an input pin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control", meta = (PinShownByDefault))
	float HeadLookAtAlpha;

	// The bone for the head.
	UPROPERTY(EditAnywhere, Category = "Bones")
	FBoneReference HeadBone;

	// The bone for the left eye.
	UPROPERTY(EditAnywhere, Category = "Bones")
	FBoneReference LeftEyeBone;

	// The bone for the right eye.
	UPROPERTY(EditAnywhere, Category = "Bones")
	FBoneReference RightEyeBone;

	// Maximum angle the head can turn left or right.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	float HeadYawLimit;

public:
	FAnimNode_MetaHumanGaze();

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};
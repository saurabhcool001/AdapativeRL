// Copyright (C) 2024, Some C++ Maste. All Rights Reserved.

#include "AnimNode_MetaHumanGaze.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetMathLibrary.h"

FAnimNode_MetaHumanGaze::FAnimNode_MetaHumanGaze()
{
	LookAtLocation = FVector::ZeroVector;
	HeadLookAtAlpha = 0.0f;
	HeadYawLimit = 45.0f;
	
	// Set default bone names
	HeadBone.BoneName = FName("head");
	LeftEyeBone.BoneName = FName("FACIAL_L_Eye");
	RightEyeBone.BoneName = FName("FACIAL_R_Eye");
}

void FAnimNode_MetaHumanGaze::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	Super::Initialize_AnyThread(Context);
}

void FAnimNode_MetaHumanGaze::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// --- Head Rotation ---
	if (HeadLookAtAlpha > 0.0f)
	{
		// Get the Component Space transform of the head bone
		FTransform HeadBoneCSTransform = Output.Pose.GetComponentSpaceTransform(HeadBone.CachedCompactPoseIndex);
		
		// Calculate the target rotation for the head to look at the location
		FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(HeadBoneCSTransform.GetLocation(), LookAtLocation);
		
		// Blend the current rotation with the target rotation using the alpha
		FRotator NewHeadRotation = FMath::RInterpTo(HeadBoneCSTransform.GetRotation().Rotator(), TargetRotation, 1.0f, HeadLookAtAlpha);
		
		// Apply the new rotation
		HeadBoneCSTransform.SetRotation(NewHeadRotation.Quaternion());

		// Add the new transform to the output
		OutBoneTransforms.Add(FBoneTransform(HeadBone.CachedCompactPoseIndex, HeadBoneCSTransform));
	}

	// --- Eye Rotations (Always Alpha 1.0) ---
	// Left Eye
	FTransform LeftEyeCSTransform = Output.Pose.GetComponentSpaceTransform(LeftEyeBone.CachedCompactPoseIndex);
	FRotator LeftEyeTargetRotation = UKismetMathLibrary::FindLookAtRotation(LeftEyeCSTransform.GetLocation(), LookAtLocation);
	LeftEyeCSTransform.SetRotation(LeftEyeTargetRotation.Quaternion());
	OutBoneTransforms.Add(FBoneTransform(LeftEyeBone.CachedCompactPoseIndex, LeftEyeCSTransform));

	// Right Eye
	FTransform RightEyeCSTransform = Output.Pose.GetComponentSpaceTransform(RightEyeBone.CachedCompactPoseIndex);
	FRotator RightEyeTargetRotation = UKismetMathLibrary::FindLookAtRotation(RightEyeCSTransform.GetLocation(), LookAtLocation);
	RightEyeCSTransform.SetRotation(RightEyeTargetRotation.Quaternion());
	OutBoneTransforms.Add(FBoneTransform(RightEyeBone.CachedCompactPoseIndex, RightEyeCSTransform));
}

bool FAnimNode_MetaHumanGaze::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// Check if all our bones are valid
	return HeadBone.IsValidToEvaluate(RequiredBones) &&
		   LeftEyeBone.IsValidToEvaluate(RequiredBones) &&
		   RightEyeBone.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_MetaHumanGaze::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	HeadBone.Initialize(RequiredBones);
	LeftEyeBone.Initialize(RequiredBones);
	RightEyeBone.Initialize(RequiredBones);
}

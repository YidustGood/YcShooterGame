// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcWeaponVisualData.h"
#include "YcShooterGameplayTags.h"

UYcWeaponVisualData::UYcWeaponVisualData()
{
}

const FYcWeaponActionVisual* UYcWeaponVisualData::GetActionVisual(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid())
	{
		return nullptr;
	}

	// 使用预定义的 Tag 进行精确匹配
	using namespace YcShooterGameplayTags;

	// 查找扩展动作
	if (const FYcWeaponActionVisual* Action = Actions.Find(ActionTag))
	{
		return Action;
	}

	return nullptr;
}

TSoftObjectPtr<UAnimSequence> UYcWeaponVisualData::GetPoseAnim(FGameplayTag PoseTag) const
{
	if (!PoseTag.IsValid())
	{
		return nullptr;
	}

	// 使用预定义的 Tag 进行精确匹配
	using namespace YcShooterGameplayTags;

	// 查找扩展姿态
	if (const TSoftObjectPtr<UAnimSequence>* ExtendedPose = PoseAnims.Find(PoseTag))
	{
		return *ExtendedPose;
	}

	return nullptr;
}

bool UYcWeaponVisualData::K2_GetActionVisual(FGameplayTag ActionTag, FYcWeaponActionVisual& OutActionVisual) const
{
	if (const FYcWeaponActionVisual* Found = GetActionVisual(ActionTag))
	{
		OutActionVisual = *Found;
		return true;
	}
	return false;
}

TSoftObjectPtr<UAnimSequence> UYcWeaponVisualData::K2_GetPoseAnim(FGameplayTag PoseTag) const
{
	return GetPoseAnim(PoseTag);
}

bool UYcWeaponVisualData::HasAction(FGameplayTag ActionTag) const
{
	return ActionTag.IsValid() && Actions.Contains(ActionTag);
}

TArray<FGameplayTag> UYcWeaponVisualData::GetAllActionTags() const
{
	TArray<FGameplayTag> Tags;
	Actions.GetKeys(Tags);
	return Tags;
}

TArray<FGameplayTag> UYcWeaponVisualData::GetAllPoseTags() const
{
	TArray<FGameplayTag> Tags;
	PoseAnims.GetKeys(Tags);
	return Tags;
}

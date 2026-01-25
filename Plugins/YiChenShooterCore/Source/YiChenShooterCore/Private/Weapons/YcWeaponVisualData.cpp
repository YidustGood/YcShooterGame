// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcWeaponVisualData.h"
#include "YcShooterGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Yc_Asset_Item_WeaponVisualData, "Yc.Asset.Item.WeaponVisualData");

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

TSoftObjectPtr<UNiagaraSystem> UYcWeaponVisualData::GetVFXEffect(FGameplayTag VFXTag) const
{
	if (!VFXTag.IsValid())
	{
		return nullptr;
	}

	// 查找特效
	if (const TSoftObjectPtr<UNiagaraSystem>* Effect = VFXEffects.Find(VFXTag))
	{
		return *Effect;
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

TSoftObjectPtr<UNiagaraSystem> UYcWeaponVisualData::K2_GetVFXEffect(FGameplayTag VFXTag) const
{
	return GetVFXEffect(VFXTag);
}

bool UYcWeaponVisualData::HasVFXEffect(FGameplayTag VFXTag) const
{
	return VFXTag.IsValid() && VFXEffects.Contains(VFXTag);
}

TArray<FGameplayTag> UYcWeaponVisualData::GetAllVFXTags() const
{
	TArray<FGameplayTag> Tags;
	VFXEffects.GetKeys(Tags);
	return Tags;
}

TSoftObjectPtr<USoundBase> UYcWeaponVisualData::GetSoundEffect(FGameplayTag SoundTag) const
{
	if (!SoundTag.IsValid())
	{
		return nullptr;
	}

	// 查找音效
	if (const TSoftObjectPtr<USoundBase>* Sound = SoundEffects.Find(SoundTag))
	{
		return *Sound;
	}

	return nullptr;
}

TSoftObjectPtr<USoundBase> UYcWeaponVisualData::K2_GetSoundEffect(FGameplayTag SoundTag) const
{
	return GetSoundEffect(SoundTag);
}

bool UYcWeaponVisualData::HasSoundEffect(FGameplayTag SoundTag) const
{
	return SoundTag.IsValid() && SoundEffects.Contains(SoundTag);
}

TArray<FGameplayTag> UYcWeaponVisualData::GetAllSoundTags() const
{
	TArray<FGameplayTag> Tags;
	SoundEffects.GetKeys(Tags);
	return Tags;
}

// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcWeaponInstance.h"

#include "YcGameplayEffectContext.h"
#include "Physics/YcPhysicalMaterialWithTags.h"
#include "Weapons/YcWeaponLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponInstance)

UYcWeaponInstance::UYcWeaponInstance(const FObjectInitializer& ObjectInitializer)
{
}

void UYcWeaponInstance::OnEquipmentInstanceCreated(const FYcEquipmentDefinition& Definition)
{
	Super::OnEquipmentInstanceCreated(Definition);
	
	// 获取到武器视觉资产并保存, 方便后续使用
	WeaponVisualData = UYcWeaponLibrary::GetWeaponVisualData(this);
}

void UYcWeaponInstance::OnEquipped()
{
	Super::OnEquipped();
	
	TimeLastEquipped = GetWorld()->GetTimeSeconds();
	
	// @TODO: 应用输入设备属性（手柄震动等）
}

void UYcWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();
	
	// @TODO: 移除输入设备属性
}

void UYcWeaponInstance::UpdateFiringTime()
{
	// 获取当前世界时间并记录为最后开火时间
	const UWorld* World = GetWorld();
	check(World);
	TimeLastFired = World->GetTimeSeconds();
}

float UYcWeaponInstance::GetTimeSinceLastInteractedWith() const
{
	// 计算上一次装备到上一次射击的时间间隔
	return FMath::Max(TimeLastEquipped, TimeLastFired) ;
}

float UYcWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags) const
{
	return 1.0f;
}

float UYcWeaponInstance::GetPhysicalMaterialMultiplier(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags) const
{
	FYcWeaponDamageProfileView ProfileView;
	if (!PhysicalMaterial || !BuildCurrentDamageProfileView(nullptr, ProfileView))
	{
		return 1.0f;
	}

	const UYcPhysicalMaterialWithTags* TaggedMaterial = Cast<UYcPhysicalMaterialWithTags>(PhysicalMaterial);
	if (!TaggedMaterial || TaggedMaterial->Tags.IsEmpty())
	{
		return 1.0f;
	}

	for (const auto& Pair : ProfileView.HitZoneDamageMultipliers)
	{
		if (TaggedMaterial->Tags.HasTag(Pair.Key))
		{
			return Pair.Value;
		}
	}

	return 1.0f;
}

float UYcWeaponInstance::GetPhysicalMaterialMultiplier(const FYcGameplayEffectContext& EffectContext, const UPhysicalMaterial* PhysicalMaterial,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	FYcWeaponDamageProfileView ProfileView;
	if (!PhysicalMaterial || !BuildCurrentDamageProfileView(&EffectContext, ProfileView))
	{
		return 1.0f;
	}

	const UYcPhysicalMaterialWithTags* TaggedMaterial = Cast<UYcPhysicalMaterialWithTags>(PhysicalMaterial);
	if (!TaggedMaterial || TaggedMaterial->Tags.IsEmpty())
	{
		return 1.0f;
	}

	for (const auto& Pair : ProfileView.HitZoneDamageMultipliers)
	{
		if (TaggedMaterial->Tags.HasTag(Pair.Key))
		{
			return Pair.Value;
		}
	}

	return 1.0f;
}

FGameplayTag UYcWeaponInstance::GetHitZoneFromPhysicalMaterial(const UPhysicalMaterial* PhysicalMaterial) const
{
	FYcWeaponDamageProfileView ProfileView;
	if (!PhysicalMaterial || !BuildCurrentDamageProfileView(nullptr, ProfileView))
	{
		return FGameplayTag();
	}

	const UYcPhysicalMaterialWithTags* TaggedMaterial = Cast<UYcPhysicalMaterialWithTags>(PhysicalMaterial);
	if (!TaggedMaterial || TaggedMaterial->Tags.IsEmpty())
	{
		return FGameplayTag();
	}

	for (const auto& Pair : ProfileView.HitZoneDamageMultipliers)
	{
		if (TaggedMaterial->Tags.HasTag(Pair.Key))
		{
			return Pair.Key;
		}
	}

	return FGameplayTag();
}

FGameplayTag UYcWeaponInstance::GetHitZoneFromPhysicalMaterial(const FYcGameplayEffectContext& EffectContext, const UPhysicalMaterial* PhysicalMaterial) const
{
	FYcWeaponDamageProfileView ProfileView;
	if (!PhysicalMaterial || !BuildCurrentDamageProfileView(&EffectContext, ProfileView))
	{
		return FGameplayTag();
	}

	const UYcPhysicalMaterialWithTags* TaggedMaterial = Cast<UYcPhysicalMaterialWithTags>(PhysicalMaterial);
	if (!TaggedMaterial || TaggedMaterial->Tags.IsEmpty())
	{
		return FGameplayTag();
	}

	for (const auto& Pair : ProfileView.HitZoneDamageMultipliers)
	{
		if (TaggedMaterial->Tags.HasTag(Pair.Key))
		{
			return Pair.Key;
		}
	}

	return FGameplayTag();
}

bool UYcWeaponInstance::BuildCurrentDamageProfileView(const FYcGameplayEffectContext* EffectContext, FYcWeaponDamageProfileView& OutView) const
{
	return BuildDefaultDamageProfileView(OutView);
}

bool UYcWeaponInstance::BuildDefaultDamageProfileView(FYcWeaponDamageProfileView& OutView) const
{
	OutView = FYcWeaponDamageProfileView();
	return false;
}

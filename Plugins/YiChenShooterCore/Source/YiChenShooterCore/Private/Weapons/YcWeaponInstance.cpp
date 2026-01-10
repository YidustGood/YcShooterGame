// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponInstance)

UYcWeaponInstance::UYcWeaponInstance(const FObjectInitializer& ObjectInitializer)
{
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

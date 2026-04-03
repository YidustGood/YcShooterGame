// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_SetDamageType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_SetDamageType)

UYcDamageComponent_SetDamageType::UYcDamageComponent_SetDamageType()
{
	Priority = 5; // 最高优先级，确保在其他组件之前设置
	DebugName = TEXT("SetDamageType");
}

void UYcDamageComponent_SetDamageType::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	// 转换为伤害参数
	FYcDamageSummaryParams& DamageParams = static_cast<FYcDamageSummaryParams&>(Params);
	
	// 如果已有类型且不覆盖，跳过
	if (DamageParams.DamageTypeTag.IsValid() && !bOverrideExisting)
	{
		return;
	}

	// 设置默认类型
	if (DefaultDamageTypeTag.IsValid())
	{
		DamageParams.DamageTypeTag = DefaultDamageTypeTag;
		LogDebug(FString::Printf(TEXT("Set DamageType: %s"), *DefaultDamageTypeTag.ToString()));
	}
}

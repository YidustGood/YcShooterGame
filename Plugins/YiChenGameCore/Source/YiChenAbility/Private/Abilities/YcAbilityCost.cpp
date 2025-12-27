// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Abilities/YcAbilityCost.h"
#include "YiChenGameCore.h"  // 包含Core模块以使用 YICHEN_GAMECORE_SUPPORT_ANGELSCRIPT 宏

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilityCost)

UYcAbilityCost::UYcAbilityCost()
{
	
}

bool UYcAbilityCost::CheckCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
                               const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	bool bCallK2CheckCost = false;
	
#if YICHEN_GAMECORE_SUPPORT_ANGELSCRIPT
	// AngelScript支持模式：同时检查 bIsScriptClass（AngelScript脚本类值为true）和引擎约束的_C名称
	bCallK2CheckCost = (GetClass() && GetClass()->bIsScriptClass) || GetClass()->GetName().EndsWith(TEXT("_C"));
#else
	// 标准模式：检查引擎约束的蓝图类型名称  _C结尾, 因为DefaultToInstanced创建的实例对象用 IsA(UBlueprintGeneratedClass::StaticClass())无法正确判断
	bCallK2CheckCost = GetClass()->GetName().EndsWith(TEXT("_C"));
#endif
	
	// 确保只有脚本类或者蓝图类的时候才调用K2_CheckCost()
	if (bCallK2CheckCost)
	{
		return K2_CheckCost(Ability, Handle, *ActorInfo);
	}
	else
	{
		// C++实现的成本类，默认允许（子类可以重写CheckCost来实现自定义逻辑）
		return true;
	}
}

void UYcAbilityCost::ApplyCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	K2_ApplyCost(Ability, Handle, *ActorInfo, ActivationInfo);
}
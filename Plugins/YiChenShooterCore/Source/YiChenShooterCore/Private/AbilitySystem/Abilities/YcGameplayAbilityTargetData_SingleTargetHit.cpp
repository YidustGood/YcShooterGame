// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "AbilitySystem/Abilities/YcGameplayAbilityTargetData_SingleTargetHit.h"

#include "YcGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbilityTargetData_SingleTargetHit)

void FYcGameplayAbilityTargetData_SingleTargetHit::AddTargetDataToContext(FGameplayEffectContextHandle& Context, bool bIncludeActorArray) const
{
	// 首先调用基类实现
	// 基类会处理以下数据的传递：
	// - HitResult（命中结果，包含位置、法线、骨骼名、物理材质等）
	// - Actor 数组（如果 bIncludeActorArray 为 true）
	FGameplayAbilityTargetData_SingleTargetHit::AddTargetDataToContext(Context, bIncludeActorArray);

	// 然后添加项目特定的数据
	// 从通用的 FGameplayEffectContext 句柄中提取我们的自定义类型 FYcGameplayEffectContext
	// 如果类型匹配，则将 CartridgeID 传递到上下文中
	if (FYcGameplayEffectContext* TypedContext = FYcGameplayEffectContext::ExtractEffectContext(Context))
	{
		// 将弹匣ID传递到效果上下文
		// 后续的伤害计算、效果应用等逻辑可以通过 Context 访问这个值
		TypedContext->CartridgeID = CartridgeID;
	}
}

bool FYcGameplayAbilityTargetData_SingleTargetHit::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	// 首先序列化基类数据
	// 这会处理 HitResult 和 bHitReplaced 的序列化
	FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(Ar, Map, bOutSuccess);

	// 然后序列化项目特定数据
	// Ar << 操作符会根据 Ar 的模式自动选择读取或写入
	// - 如果 Ar.IsSaving()：将 CartridgeID 写入存档
	// - 如果 Ar.IsLoading()：从存档读取 CartridgeID
	Ar << CartridgeID;

	return true;
}
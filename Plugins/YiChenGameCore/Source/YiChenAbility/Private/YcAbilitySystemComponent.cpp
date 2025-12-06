// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "GameplayCueManager.h"
#include "YcGlobalAbilitySystem.h"
#include "Abilities/YcGameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilitySystemComponent)

UYcAbilitySystemComponent::UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UYcAbilitySystemComponent* UYcAbilitySystemComponent::GetAbilitySystemComponentFromActor(const AActor* Actor,
	bool LookForComponent)
{
	return Cast<UYcAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, LookForComponent));
}

/**
 * 根据技能类查找技能规格句柄
 * 遍历已激活的技能列表，找到第一个匹配的技能规格
 * 使用ABILITYLIST_SCOPE_LOCK()保护列表访问，防止并发修改
 * @param AbilityClass 要查找的技能类
 * @param OptionalSourceObject 可选的源对象过滤器
 * @return 找到的技能规格句柄，如果未找到则返回无效句柄
 */
FGameplayAbilitySpecHandle UYcAbilitySystemComponent::FindAbilitySpecHandleForClass(
	const TSubclassOf<UGameplayAbility> AbilityClass, UObject* OptionalSourceObject)
{
	ABILITYLIST_SCOPE_LOCK(); // 保护列表访问，防止并发修改
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		TSubclassOf<UGameplayAbility> SpecAbilityClass = Spec.Ability->GetClass();
		if (SpecAbilityClass != AbilityClass) continue;
		
		if (!OptionalSourceObject || (OptionalSourceObject && Spec.SourceObject == OptionalSourceObject))
		{
			return Spec.Handle;
		}
	}

	return FGameplayAbilitySpecHandle();
}

/**
 * 尝试取消指定类型的技能
 * 查找所有正在激活的指定类型的技能，并取消第一个找到的
 * @param InAbilityToCancel 要取消的技能类
 * @return 如果成功取消至少一个技能则返回true，否则返回false
 */
bool UYcAbilitySystemComponent::TryCancelAbilityByClass(const TSubclassOf<UGameplayAbility> InAbilityToCancel)
{
	bool bSuccess = false;

	const UGameplayAbility* const InAbilityCDO = InAbilityToCancel.GetDefaultObject();

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.IsActive() && Spec.Ability == InAbilityCDO)
		{
			CancelAbilityHandle(Spec.Handle);
			bSuccess = true;
			break;
		}
	}

	return bSuccess;
}

/**
 * 获取指定GameplayTag的计数（蓝图接口）
 * 直接调用父类GetTagCount方法
 * @param TagToCheck 要查询的GameplayTag
 * @return 该标签的堆叠计数
 */
int32 UYcAbilitySystemComponent::K2_GetTagCount(const FGameplayTag TagToCheck) const
{
	return GetTagCount(TagToCheck);
}

/**
 * 添加一个Loose GameplayTag（蓝图接口）
 * 直接调用父类AddLooseGameplayTag方法
 * @param GameplayTag 要添加的GameplayTag
 * @param Count 添加的数量
 */
void UYcAbilitySystemComponent::K2_AddLooseGameplayTag(const FGameplayTag& GameplayTag, const int32 Count)
{
	AddLooseGameplayTag(GameplayTag, Count);
}

/**
 * 批量添加Loose GameplayTags（蓝图接口）
 * 直接调用父类AddLooseGameplayTags方法
 * @param GameplayTags 要添加的GameplayTag容器
 * @param Count 每个标签添加的数量
 */
void UYcAbilitySystemComponent::K2_AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, const int32 Count)
{
	AddLooseGameplayTags(GameplayTags, Count);
}

/**
 * 移除一个Loose GameplayTag（蓝图接口）
 * 直接调用父类RemoveLooseGameplayTag方法
 * @param GameplayTag 要移除的GameplayTag
 * @param Count 移除的数量
 */
void UYcAbilitySystemComponent::K2_RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, const int32 Count)
{
	RemoveLooseGameplayTag(GameplayTag, Count);
}

/**
 * 批量移除Loose GameplayTags（蓝图接口）
 * 直接调用父类RemoveLooseGameplayTags方法
 * @param GameplayTags 要移除的GameplayTag容器
 * @param Count 每个标签移除的数量
 */
void UYcAbilitySystemComponent::K2_RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, const int32 Count)
{
	RemoveLooseGameplayTags(GameplayTags, Count);
}

/**
 * 在本地执行一次性的GameplayCue效果
 * 通过GameplayCueManager直接处理GameplayCue事件，不经过网络同步
 * 仅在调用此函数的客户端上执行
 * @param GameplayCueTag 要执行的GameplayCue标签
 * @param GameplayCueParameters GameplayCue的参数
 */
void UYcAbilitySystemComponent::ExecuteGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Executed, GameplayCueParameters);
}

/**
 * 在本地添加一个持续的GameplayCue效果
 * 触发OnActive和WhileActive事件，模拟GameplayCue的持续状态
 * 仅在调用此函数的客户端上执行，不经过网络同步
 * @param GameplayCueTag 要添加的GameplayCue标签
 * @param GameplayCueParameters GameplayCue的参数
 */
void UYcAbilitySystemComponent::AddGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::OnActive, GameplayCueParameters);
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::WhileActive, GameplayCueParameters);
}

/**
 * 在本地移除一个GameplayCue效果
 * 触发Removed事件，清理GameplayCue的持续状态
 * 仅在调用此函数的客户端上执行，不经过网络同步
 * @param GameplayCueTag 要移除的GameplayCue标签
 * @param GameplayCueParameters GameplayCue的参数
 */
void UYcAbilitySystemComponent::RemoveGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Removed, GameplayCueParameters);
}

/**
 * 尝试激活技能并批处理所有RPC
 * 使用FScopedServerAbilityRPCBatcher将同一帧内的多个RPC合并为一个
 * 这样可以显著减少网络流量，特别是对于快速连续的技能操作
 * 
 * RPC批处理效果：
 *   - 单发技能：ActivateAbility + SendTargetData + EndAbility = 1个RPC
 *   - 连续技能：首发合并，后续单独，最后1个RPC结束
 * 
 * @param InAbilityHandle 要激活的技能规格句柄
 * @param EndAbilityImmediately 是否立即结束技能。为true时激活后立即调用ExternalEndAbility()
 * @return 如果技能成功激活则返回true，否则返回false
 */
bool UYcAbilitySystemComponent::BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle,
	bool EndAbilityImmediately)
{
	bool AbilityActivated = false;
	if (InAbilityHandle.IsValid())
	{
		FScopedServerAbilityRPCBatcher GSAbilityRPCBatcher(this, InAbilityHandle);
		AbilityActivated = TryActivateAbility(InAbilityHandle, true);

		if (EndAbilityImmediately)
		{
			if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(InAbilityHandle))
			{
				UYcGameplayAbility* Ability = Cast<UYcGameplayAbility>(AbilitySpec->GetPrimaryInstance());
				Ability->ExternalEndAbility();
			}
		}

		return AbilityActivated;
	}

	return AbilityActivated;
}

/**
 * 对自身应用GameplayEffect，支持客户端预测
 * 如果ASC拥有有效的预测键，则使用预测模式应用效果
 * 预测模式允许客户端提前应用效果，然后由服务器验证
 * 如果预测键无效，则以普通模式应用效果
 * 
 * @param GameplayEffectClass 要应用的GameplayEffect类
 * @param Level 效果等级，影响效果的强度
 * @param EffectContext 效果上下文。如果无效则会自动创建
 * @return 应用的GameplayEffect句柄，用于后续移除或修改效果。如果输入无效则返回无效句柄
 */
FActiveGameplayEffectHandle UYcAbilitySystemComponent::K2_ApplyGameplayEffectToSelfWithPrediction(
	TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level, FGameplayEffectContextHandle EffectContext)
{
	if (GameplayEffectClass)
	{
		if (!EffectContext.IsValid())
		{
			EffectContext = MakeEffectContext();
		}

		const UGameplayEffect* GameplayEffect = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();

		if (CanPredict())
		{
			return ApplyGameplayEffectToSelf(GameplayEffect, Level, EffectContext, ScopedPredictionKey);
		}

		return ApplyGameplayEffectToSelf(GameplayEffect, Level, EffectContext);
	}
	
	return FActiveGameplayEffectHandle();
}

/**
 * 对目标应用GameplayEffect，支持客户端预测
 * 如果ASC拥有有效的预测键，则使用预测模式应用效果
 * 预测模式允许客户端提前应用效果，然后由服务器验证
 * 如果预测键无效，则以普通模式应用效果
 * 
 * 参数验证：
 *   - 如果Target为nullptr，记录警告日志并返回无效句柄
 *   - 如果GameplayEffectClass为nullptr，记录错误日志并返回无效句柄
 * 
 * @param GameplayEffectClass 要应用的GameplayEffect类
 * @param Target 目标ASC。如果为nullptr则返回无效句柄
 * @param Level 效果等级，影响效果的强度
 * @param Context 效果上下文，包含源对象、目标对象等信息
 * @return 应用的GameplayEffect句柄，用于后续移除或修改效果。如果输入无效则返回无效句柄
 */
FActiveGameplayEffectHandle UYcAbilitySystemComponent::K2_ApplyGameplayEffectToTargetWithPrediction(
	TSubclassOf<UGameplayEffect> GameplayEffectClass, UAbilitySystemComponent* Target, float Level,
	FGameplayEffectContextHandle Context)
{
	if (Target == nullptr)
	{
		ABILITY_LOG(Log, TEXT("UAbilitySystemComponent::BP_ApplyGameplayEffectToTargetWithPrediction called with null Target. %s. Context: %s"), *GetFullName(), *Context.ToString());
		return FActiveGameplayEffectHandle();
	}

	if (GameplayEffectClass == nullptr)
	{
		ABILITY_LOG(Error, TEXT("UAbilitySystemComponent::BP_ApplyGameplayEffectToTargetWithPrediction called with null GameplayEffectClass. %s. Context: %s"), *GetFullName(), *Context.ToString());
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* GameplayEffect = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();

	if (CanPredict())
	{
		return ApplyGameplayEffectToTarget(GameplayEffect, Target, Level, Context, ScopedPredictionKey);
	}

	return ApplyGameplayEffectToTarget(GameplayEffect, Target, Level, Context);
}

/**
 * 获取当前预测键的状态信息
 * 返回预测键的字符串表示和是否有效的状态
 * 用于调试和监控客户端预测的状态
 * 
 * @return 包含预测键信息和有效性的字符串，格式为："[PredictionKey] is valid for more prediction: [true/false]"
 */
FString UYcAbilitySystemComponent::GetCurrentPredictionKeyStatus()
{
	return ScopedPredictionKey.ToString() + " is valid for more prediction: " + (ScopedPredictionKey.IsValidForMorePrediction() ? TEXT("true") : TEXT("false"));
}

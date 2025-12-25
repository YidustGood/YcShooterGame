// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "GameplayCueManager.h"
#include "YiChenAbility.h"
#include "Abilities/YcGameplayAbility.h"
#include "NativeGameplayTags.h"
#include "YcAbilityTagRelationshipMapping.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilitySystemComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

UYcAbilitySystemComponent::UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UYcAbilitySystemComponent* UYcAbilitySystemComponent::GetAbilitySystemComponentFromActor(const AActor* Actor,
	bool LookForComponent)
{
	return Cast<UYcAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, LookForComponent));
}

void UYcAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);
	
	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);
	
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	
	if (bHasNewPawnAvatar)
	{
		// 通知所有可用技能新的Pawn Avatar已被设置
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			UYcGameplayAbility* AbilityCDO = CastChecked<UYcGameplayAbility>(AbilitySpec.Ability);

			if (AbilityCDO->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced)
			{
				TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
				for (UGameplayAbility* AbilityInstance : Instances)
				{
					UYcGameplayAbility* YcAbilityInstance = Cast<UYcGameplayAbility>(AbilityInstance);
					if (YcAbilityInstance)
					{
						// 对于回放，技能实例可能缺失
						YcAbilityInstance->OnPawnAvatarSet();
					}
				}
			}
			else
			{
				AbilityCDO->OnPawnAvatarSet();
			}
		}
		
		TryActivateAbilitiesOnSpawn();
		
		// @TODO: 后续可添加Avatar变化时对于动画相关内容的处理
	}
}

void UYcAbilitySystemComponent::SetTagRelationshipMapping(UYcAbilityTagRelationshipMapping* NewMapping)
{
	TagRelationshipMapping = NewMapping;
}

void UYcAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags,
	FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const
{
	// 根据TagRelationshipMapping查询技能激活所需的额外标签
	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
	}
}

void UYcAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		const UYcGameplayAbility* AbilityCDO = CastChecked<UYcGameplayAbility>(AbilitySpec.Ability);
		AbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
	}
}

void UYcAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags,
	UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags,
	bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	FGameplayTagContainer ModifiedBlockTags = BlockTags;
	FGameplayTagContainer ModifiedCancelTags = CancelTags;

	if (TagRelationshipMapping)
	{
		// 使用映射表扩展技能标签，获取应该阻止和取消的标签
		TagRelationshipMapping->GetAbilityTagsToBlockAndCancel(AbilityTags, &ModifiedBlockTags, &ModifiedCancelTags);
	}

	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, ModifiedBlockTags, bExecuteCancelTags, ModifiedCancelTags);

	// @TODO: 后续可添加特殊逻辑，如阻止输入或禁用移动
}

void UYcAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags,
	UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	// @TODO: 后续可添加特殊逻辑，如阻止输入或禁用移动
}

void UYcAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	
	// 遍历当前可激活的技能列表, 如果技能标签列表中有与输入标签匹配的就将其句柄添加到被按下列表中, 以便后续触发能力
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
		{
			InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			if (!InputPressedToReleasedSpecHandles.Contains(AbilitySpec.Handle))
			{
				InputPressedSpecHandles.Add(AbilitySpec.Handle);
			}
		}
	}
}

void UYcAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	
	// 检查可激活技能列表, 如果有与当前已取消输入的标签匹配的, 就从列表中移除
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
		{
			InputPressedToReleasedSpecHandles.Remove(AbilitySpec.Handle);
			InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
			InputHeldSpecHandles.Remove(AbilitySpec.Handle);
		}
	}
}

void UYcAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// 如果当前GAS拥有TAG_Gameplay_AbilityInputBlocked标签,那么将停止处理AbilityInput并清理所有AbilityInput状态
	if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
	{
		ClearAbilityInput();
		return;
	}
	
	// 本帧收集到的要激活的ability列表, 使用局部静态变量是为了避免每次调用都去申请空间, 因为该函数是tick调用所以可以优化性能
	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();
	
	// @TODO: 考虑在这些循环中使用FScopedServerAbilityRPCBatcher进行RPC批处理优化
	
	// 处理持续输入的技能（WhileInputActive策略）
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec) continue;
		if (!AbilitySpec->Ability || AbilitySpec->IsActive()) continue;
		// 未激活就加入本帧待激活列表
		const UYcGameplayAbility* AbilityCDO = CastChecked<UYcGameplayAbility>(AbilitySpec->Ability);
		if (AbilityCDO->GetActivationPolicy() == EYcAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}
	
	// 处理单次按下触发的技能（OnInputTriggered策略）
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec) continue;
		if (!AbilitySpec->Ability) continue;
		
		InputPressedToReleasedSpecHandles.Add(AbilitySpec->Handle);	// 标记这个触发输入已经被处理了,进入等待释放状态,避免反复处理
		AbilitySpec->InputPressed = true;
		// 点击按钮后激活能力,在能力没有结束前再次被点击就会通知WaitInputPress,以供能力响应激活期间的InputPress事件
		if (AbilitySpec->IsActive())
		{
			// 技能已激活，传递输入事件
			AbilitySpecInputPressed(*AbilitySpec);
			continue;
		}
		// 未激活就加入本帧待激活列表
		const UYcGameplayAbility* AbilityCDO = CastChecked<UYcGameplayAbility>(AbilitySpec->Ability);
		if (AbilityCDO->GetActivationPolicy() == EYcAbilityActivationPolicy::OnInputTriggered)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}
	
	// 统一激活所有待激活的技能（包括按下和持续输入触发的）
	// 统一处理可以避免持续输入在激活技能后又因为按下事件而再次发送输入事件
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
#if WITH_EDITOR // 编辑器下调试用
		const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle);
		if(AbilitySpec && AbilitySpec->Ability)
		{
			const FString AbilitySpecDebugMsg = FString::Printf(TEXT("尝试激活技能:%s"), *AbilitySpec->Ability->GetName());
			UKismetSystemLibrary::PrintString(this, AbilitySpecDebugMsg, false, true, FLinearColor::Green, 0.5f);
		}
#endif
		TryActivateAbility(AbilitySpecHandle);
	}
	
	// 处理所有本帧释放输入的技能
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec || !AbilitySpec->Ability || !AbilitySpec->IsActive()) continue;
		// 技能已激活，传递输入释放事件
		AbilitySpecInputReleased(*AbilitySpec);
	}
	
	// 清除本帧缓存的技能句柄
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UYcAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UYcAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);
	// 不支持UGameplayAbility::bReplicateInputDirectly
	// 使用复制事件代替，以确保WaitInputPress任务能正常工作
	if (!Spec.IsActive()) return;
	
	Spec.InputPressed = true;
	// 如果技能已激活则发送复制事件
	TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
	// @TODO: 需要验证直接获取Last()是否安全，考虑添加边界检查
	const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last() -> GetCurrentActivationInfoRef();
	// 触发InputPressed事件。此处不复制，监听者可以选择将InputPressed事件复制到服务器
	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
}

void UYcAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// 不支持UGameplayAbility::bReplicateInputDirectly
	// 使用复制事件代替，以确保WaitInputRelease任务能正常工作
	if (!Spec.IsActive()) return;
	Spec.InputPressed = false;
	// 如果技能已激活则发送复制事件
	TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
	// @TODO: 需要验证直接获取Last()是否安全，考虑添加边界检查
	const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last() -> GetCurrentActivationInfoRef();
	// 触发InputReleased事件。此处不复制，监听者可以选择将InputReleased事件复制到服务器
	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
}

void UYcAbilitySystemComponent::CancelInputActivatedAbilities(const bool bReplicateCancelAbility)
{
	auto ShouldCancelFunc = [this](const UYcGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)
	{
		// 获取技能激活策略，如果属于输入激活类型就满足取消激活条件
		const EYcAbilityActivationPolicy ActivationPolicy = Ability->GetActivationPolicy();
		return ((ActivationPolicy == EYcAbilityActivationPolicy::OnInputTriggered) || (ActivationPolicy == EYcAbilityActivationPolicy::WhileInputActive));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

bool UYcAbilitySystemComponent::IsActivationGroupBlocked(EYcAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
	case EYcAbilityActivationGroup::Independent:
		// 独立能力分组永远不会被阻塞
		bBlocked = false;
		break;

	case EYcAbilityActivationGroup::Exclusive_Replaceable:
	case EYcAbilityActivationGroup::Exclusive_Blocking:
		// 排他性能力仅当没有阻塞性排他能力活跃时，才能被激活
		bBlocked = (ActivationGroupCounts[static_cast<uint8>(EYcAbilityActivationGroup::Exclusive_Blocking)] > 0);
		break;

	default:
		// 如果传入无效的枚举值，触发断言并提示错误
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), static_cast<uint8>(Group));
		break;
	}

	return bBlocked;
}

void UYcAbilitySystemComponent::AddAbilityToActivationGroup(EYcAbilityActivationGroup Group,
	UYcGameplayAbility* YcAbility)
{
	check(YcAbility);
	check(ActivationGroupCounts[static_cast<uint8>(Group)] < INT32_MAX);

	ActivationGroupCounts[static_cast<uint8>(Group)]++;

	constexpr bool bReplicateCancelAbility = false;

	switch (Group)
	{
	case EYcAbilityActivationGroup::Independent:
		// 独立能力分组永远不会被阻塞
		break;
		
	case EYcAbilityActivationGroup::Exclusive_Replaceable:
	case EYcAbilityActivationGroup::Exclusive_Blocking:
		CancelActivationGroupAbilities(EYcAbilityActivationGroup::Exclusive_Replaceable, YcAbility, bReplicateCancelAbility);
		break;
		
	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), static_cast<uint8>(Group));
		break;
	}

	const int32 ExclusiveCount = 
		ActivationGroupCounts[static_cast<uint8>(EYcAbilityActivationGroup::Exclusive_Replaceable)] + 
		ActivationGroupCounts[static_cast<uint8>(EYcAbilityActivationGroup::Exclusive_Blocking)];
	
	if (!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogYcAbilitySystem, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void UYcAbilitySystemComponent::RemoveAbilityFromActivationGroup(EYcAbilityActivationGroup Group,
	const UYcGameplayAbility* YcAbility)
{
	check(YcAbility);
	check(ActivationGroupCounts[static_cast<uint8>(Group)] > 0);

	ActivationGroupCounts[static_cast<uint8>(Group)]--;
}

void UYcAbilitySystemComponent::CancelActivationGroupAbilities(EYcAbilityActivationGroup Group,
	UYcGameplayAbility* IgnoreAbility, bool bReplicateCancelAbility)
{
	// 创建条件取消函数
	auto ShouldCancelFunc = [this, Group, IgnoreAbility](const UYcGameplayAbility* YcAbility, FGameplayAbilitySpecHandle Handle)
	{
		// Group匹配并且不是需要忽略的技能则满足取消条件
		return ((YcAbility->GetActivationGroup() == Group) && (YcAbility != IgnoreAbility));
	};
	
	// 取消所有满足条件的技能
	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

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

int32 UYcAbilitySystemComponent::K2_GetTagCount(const FGameplayTag TagToCheck) const
{
	return GetTagCount(TagToCheck);
}

void UYcAbilitySystemComponent::K2_AddLooseGameplayTag(const FGameplayTag& GameplayTag, const int32 Count)
{
	AddLooseGameplayTag(GameplayTag, Count);
}

void UYcAbilitySystemComponent::K2_AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, const int32 Count)
{
	AddLooseGameplayTags(GameplayTags, Count);
}

void UYcAbilitySystemComponent::K2_RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, const int32 Count)
{
	RemoveLooseGameplayTag(GameplayTag, Count);
}

void UYcAbilitySystemComponent::K2_RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, const int32 Count)
{
	RemoveLooseGameplayTags(GameplayTags, Count);
}

void UYcAbilitySystemComponent::ExecuteGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Executed, GameplayCueParameters);
}

void UYcAbilitySystemComponent::AddGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::OnActive, GameplayCueParameters);
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::WhileActive, GameplayCueParameters);
}

void UYcAbilitySystemComponent::RemoveGameplayCueLocal(const FGameplayTag GameplayCueTag, const FGameplayCueParameters& GameplayCueParameters)
{
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(GetOwner(), GameplayCueTag, EGameplayCueEvent::Type::Removed, GameplayCueParameters);
}

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

FString UYcAbilitySystemComponent::GetCurrentPredictionKeyStatus()
{
	return ScopedPredictionKey.ToString() + " is valid for more prediction: " + (ScopedPredictionKey.IsValidForMorePrediction() ? TEXT("true") : TEXT("false"));
}

void UYcAbilitySystemComponent::CancelAbilitiesByFunc(const TShouldCancelAbilityFunc& ShouldCancelFunc,
													  const bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive()) continue;

		UYcGameplayAbility* AbilityCDO = CastChecked<UYcGameplayAbility>(AbilitySpec.Ability);

		if (AbilityCDO->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced)
		{
			// 取消所有生成的实例，而不是CDO
			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* AbilityInstance : Instances)
			{
				UYcGameplayAbility* YcAbilityInstance = CastChecked<UYcGameplayAbility>(AbilityInstance);

				if (ShouldCancelFunc(YcAbilityInstance, AbilitySpec.Handle))
				{
					if (YcAbilityInstance->CanBeCanceled())
					{
						YcAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), YcAbilityInstance->GetCurrentActivationInfo(), bReplicateCancelAbility);
					}
					else
					{
						UE_LOG(LogYcAbilitySystem, Error, TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."), *YcAbilityInstance->GetName());
					}
				}
			}
		}
		else
		{
			// 取消NonInstanced类型的技能CDO
			if (ShouldCancelFunc(AbilityCDO, AbilitySpec.Handle))
			{
				// NonInstanced类型的技能总是可以被取消的
				check(AbilityCDO->CanBeCanceled());
				AbilityCDO->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), FGameplayAbilityActivationInfo(), bReplicateCancelAbility);
			}
		}
	}
}
// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "GameplayCueManager.h"
#include "YiChenAbility.h"
#include "Abilities/YcGameplayAbility.h"
#include "NativeGameplayTags.h"
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
		// Notify all abilities that a new pawn avatar has been set
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
						// Ability instances may be missing for replays
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
		
		// @TODO Animations
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
	
	//@TODO: See if we can use FScopedServerAbilityRPCBatcher ScopedRPCBatcher in some of these loops
	
	//
	// Process all abilities that activate when the input is held./ 处理持续输入的技能
	//
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
	
	//
	// Process all abilities that had their input pressed this frame./ 处理单次按下触发的技能
	//
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
			// Ability is active so pass along the input event.
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
	
	//
	// Try to activate all the abilities that are from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send a input event to the ability because of the press.
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate) // 遍历激活所有待激活的技能
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
	
	// Process all abilities that had their input released this frame. / 处理所有释放了的Ability
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles) // 处理技能按钮释放
	{
		FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle);
		if (!AbilitySpec || !AbilitySpec->Ability || !AbilitySpec->IsActive()) continue;
		// Ability is active so pass along the input event.
		AbilitySpecInputReleased(*AbilitySpec);
	}
	
	// Clear the cached ability handles.
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
	// @TODO 这里直接获取last对吗？
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
	// @TODO 这里直接获取last对吗？
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
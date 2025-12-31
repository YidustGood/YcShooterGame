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
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilitySystemComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

static TAutoConsoleVariable<bool> CVarGasFixClientSideMontageBlendOutTimeSpecAnimInstance(TEXT("AbilitySystem.Fix.ClientSideMontageBlendOutTimeSpecAnimInstance"), true, TEXT("Enable a fix to replicate the Montage BlendOutTime for (recently) stopped Montages"));

static TAutoConsoleVariable<float> CVarReplayMontageErrorThreshold(
	TEXT("Yc.Replay.MontageErrorThreshold"),
	0.5f,
	TEXT("Tolerance level for when montage playback position correction occurs in replays")
);

UYcAbilitySystemComponent::UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void UYcAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UYcAbilitySystemComponent, RepAnimMontageInfoForMeshes);
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

void UYcAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle,
	FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
	TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
	if (ReplicatedData.IsValid())
	{
		OutTargetDataHandle = ReplicatedData->TargetData;
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
	if (ActivationGroupCounts[static_cast<uint8>(Group)] <= 0) return;

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

void UYcAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);
	
	if(UYcGameplayAbility* YcAbility = CastChecked<UYcGameplayAbility>(Ability))
	{
		AddAbilityToActivationGroup(YcAbility->GetActivationGroup(), YcAbility);
	}
}

void UYcAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
	const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);
	
	// 检查是否需要通过ClientRPC通知客户端
	if (APawn* Avatar = Cast<APawn>(GetAvatarActor()))
	{
		// 如果Avatar不是本地控制且技能支持网络，则通过ClientRPC通知客户端处理失败
		if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking())
		{
			ClientNotifyAbilityFailed(Ability, FailureReason);
			return;
		}
	}

	// 本地直接处理失败逻辑（本地控制的客户端或服务器）
	HandleAbilityFailed(Ability, FailureReason);
}

void UYcAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
	bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
	
	const UYcGameplayAbility* YcAbility = CastChecked<UYcGameplayAbility>(Ability);
	RemoveAbilityFromActivationGroup(YcAbility->GetActivationGroup(), YcAbility);
}

void UYcAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(const UGameplayAbility* Ability,
                                                                         const FGameplayTagContainer& FailureReason)
{
	UE_LOG(LogYcAbilitySystem, Warning, TEXT("UYcAbilitySystemComponent::ClientNotifyAbilityFailed: Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());

	// 客户端收到服务器通知后，调用技能的失败处理函数
	// 这会触发失败消息广播（如果配置了映射表）和蓝图事件
	if (const UYcGameplayAbility* YcAbility = Cast<const UYcGameplayAbility>(Ability))
	{
		YcAbility->OnAbilityFailedToActivate(FailureReason);
	}
}

void UYcAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability,
	const FGameplayTagContainer& FailureReason)
{
	UE_LOG(LogYcAbilitySystem, Warning, TEXT("UYcAbilitySystemComponent::HandleAbilityFailed: Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());

	// 调用技能的失败处理函数
	// 这会触发失败消息广播（如果技能配置了FailureTagToUserFacingMessages映射表，会通过GameplayMessageSubsystem广播用户友好的失败消息）
	// 同时也会触发技能的蓝图事件K2_OnAbilityFailedToActivate，允许蓝图实现自定义的失败处理逻辑
	if (const UYcGameplayAbility* YcAbility = Cast<const UYcGameplayAbility>(Ability))
	{
		YcAbility->OnAbilityFailedToActivate(FailureReason);
	}
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

float UYcAbilitySystemComponent::PlayMontageForMesh(UGameplayAbility* InAnimatingAbility, USkeletalMeshComponent* InMesh, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* NewAnimMontage, float InPlayRate, FName StartSectionName, bool bReplicateMontage)
{
	UYcGameplayAbility* InAbility = Cast<UYcGameplayAbility>(InAnimatingAbility);

	float Duration = -1.f;

	// 验证Mesh有效性：必须存在且属于AvatarActor
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && NewAnimMontage)
	{
		// 步骤1：本地播放蒙太奇
		Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate);
		if (Duration > 0.f)
		{
			// 步骤2：更新本地蒙太奇追踪信息
			FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

			// 检查是否有其他技能正在播放动画
			// 如果有，该技能应该已经收到了'interrupted'回调
			// 我们期望它在收到回调后自行结束
			if (AnimMontageInfo.LocalMontageInfo.AnimatingAbility.IsValid() && AnimMontageInfo.LocalMontageInfo.AnimatingAbility != InAnimatingAbility)
			{
				// 之前正在播放动画的技能应该已经收到了'interrupted'回调
				// 可以考虑将此作为全局策略并'取消'该技能
				// 
				// 目前，我们期望它在收到回调后自行结束
			}

			// RootMotion日志记录
			if (NewAnimMontage->HasRootMotion() && AnimInstance->GetOwningActor())
			{
				UE_LOG(LogRootMotion, Log, TEXT("UAbilitySystemComponent::PlayMontage %s, Role: %s")
					, *GetNameSafe(NewAnimMontage)
					, *UEnum::GetValueAsString(TEXT("Engine.ENetRole"), AnimInstance->GetOwningActor()->GetLocalRole())
					);
			}

			// 更新本地蒙太奇信息
			AnimMontageInfo.LocalMontageInfo.AnimMontage = NewAnimMontage;
			AnimMontageInfo.LocalMontageInfo.AnimatingAbility = InAnimatingAbility;
			// 切换PlayInstanceId，用于检测是否需要重新播放（即使是同一个蒙太奇）
			AnimMontageInfo.LocalMontageInfo.PlayInstanceId = !AnimMontageInfo.LocalMontageInfo.PlayInstanceId;
			
			// 通知技能当前正在播放的蒙太奇
			if (InAbility)
			{
				InAbility->SetCurrentMontageForMesh(InMesh, NewAnimMontage);
			}
			
			// 如果指定了起始Section，跳转到该Section
			if (StartSectionName != NAME_None)
			{
				AnimInstance->Montage_JumpToSection(StartSectionName, NewAnimMontage);
			}

			// 步骤3：处理网络复制
			if (IsOwnerActorAuthoritative())
			{
				// ===== 服务器逻辑 =====
				if (bReplicateMontage)
				{
					// 获取或创建复制蒙太奇信息
					// 这些是静态参数，只在蒙太奇开始播放时设置，之后不会改变
					FGameplayAbilityRepAnimMontageForMesh& AbilityRepMontageInfo = GetGameplayAbilityRepAnimMontageForMesh(InMesh);
					AbilityRepMontageInfo.RepMontageInfo.Animation = NewAnimMontage;
					// 切换PlayInstanceId，模拟端通过检测此值变化来判断是否需要重新播放
					AbilityRepMontageInfo.RepMontageInfo.PlayInstanceId = !bool(AbilityRepMontageInfo.RepMontageInfo.PlayInstanceId);

					// 更新蒙太奇生命周期内会变化的参数（Position、PlayRate、IsStopped等）
					AnimMontage_UpdateReplicatedDataForMesh(InMesh);

					// 强制网络更新，确保复制属性立即同步到客户端
					if (AbilityActorInfo->AvatarActor != nullptr)
					{
						AbilityActorInfo->AvatarActor->ForceNetUpdate();
					}
				}
			}
			else
			{
				// ===== 客户端预测逻辑 =====
				// 获取当前预测键，如果服务器拒绝此预测，需要回滚
				FPredictionKey PredictionKey = GetPredictionKeyForNewAction();
				if (PredictionKey.IsValidKey())
				{
					// 注册拒绝回调：如果服务器拒绝此预测，停止预测播放的蒙太奇
					PredictionKey.NewRejectedDelegate().BindUObject(this, &UYcAbilitySystemComponent::OnPredictiveMontageRejectedForMesh, InMesh, NewAnimMontage);
				}
			}
		}
	}

	return Duration;
}

float UYcAbilitySystemComponent::PlayMontageSimulatedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* NewAnimMontage, float InPlayRate, FName StartSectionName)
{
	float Duration = -1.f;
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && NewAnimMontage)
	{
		// 直接播放蒙太奇，不处理网络复制
		Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate);
		if (Duration > 0.f)
		{
			// 仅更新本地追踪信息
			FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
			AnimMontageInfo.LocalMontageInfo.AnimMontage = NewAnimMontage;
		}
	}

	return Duration;
}

void UYcAbilitySystemComponent::CurrentMontageStopForMesh(USkeletalMeshComponent* InMesh, float OverrideBlendOutTime)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
	UAnimMontage* MontageToStop = AnimMontageInfo.LocalMontageInfo.AnimMontage;
	
	// 检查是否需要停止：AnimInstance有效、蒙太奇存在且未停止
	bool bShouldStopMontage = AnimInstance && MontageToStop && !AnimInstance->Montage_GetIsStopped(MontageToStop);

	if (bShouldStopMontage)
	{
		// 确定混出时间：使用覆盖值或蒙太奇默认值
		const float BlendOutTime = (OverrideBlendOutTime >= 0.0f ? OverrideBlendOutTime : MontageToStop->BlendOut.GetBlendTime());

		// 停止蒙太奇
		AnimInstance->Montage_Stop(BlendOutTime, MontageToStop);

		// 服务器：更新复制属性，同步停止状态到模拟客户端
		if (IsOwnerActorAuthoritative())
		{
			AnimMontage_UpdateReplicatedDataForMesh(InMesh);
		}
	}
}

void UYcAbilitySystemComponent::StopAllCurrentMontages(float OverrideBlendOutTime)
{
	for (FGameplayAbilityLocalAnimMontageForMesh& GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		CurrentMontageStopForMesh(GameplayAbilityLocalAnimMontageForMesh.Mesh, OverrideBlendOutTime);
	}
}

void UYcAbilitySystemComponent::StopMontageIfCurrentForMesh(USkeletalMeshComponent* InMesh, const UAnimMontage& Montage, float OverrideBlendOutTime)
{
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
	if (&Montage == AnimMontageInfo.LocalMontageInfo.AnimMontage)
	{
		CurrentMontageStopForMesh(InMesh, OverrideBlendOutTime);
	}
}

void UYcAbilitySystemComponent::ClearAnimatingAbilityForAllMeshes(UGameplayAbility* Ability)
{
	UYcGameplayAbility* GSAbility = Cast<UYcGameplayAbility>(Ability);
	for (FGameplayAbilityLocalAnimMontageForMesh& GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		if (GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility == Ability)
		{
			GSAbility->SetCurrentMontageForMesh(GameplayAbilityLocalAnimMontageForMesh.Mesh, nullptr);
			GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility = nullptr;
		}
	}
}

void UYcAbilitySystemComponent::CurrentMontageJumpToSectionForMesh(USkeletalMeshComponent* InMesh, FName SectionName)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
	if ((SectionName != NAME_None) && AnimInstance && AnimMontageInfo.LocalMontageInfo.AnimMontage)
	{
		// 本地立即跳转
		AnimInstance->Montage_JumpToSection(SectionName, AnimMontageInfo.LocalMontageInfo.AnimMontage);
		
		if (IsOwnerActorAuthoritative())
		{
			// 服务器：更新复制属性，同步到模拟客户端
			AnimMontage_UpdateReplicatedDataForMesh(InMesh);
		}
		else
		{
			// 客户端：发送ServerRPC通知服务器
			ServerCurrentMontageJumpToSectionNameForMesh(InMesh, AnimMontageInfo.LocalMontageInfo.AnimMontage, SectionName);
		}
	}
}

void UYcAbilitySystemComponent::CurrentMontageSetNextSectionNameForMesh(USkeletalMeshComponent* InMesh, FName FromSectionName, FName ToSectionName)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
	if (AnimMontageInfo.LocalMontageInfo.AnimMontage && AnimInstance)
	{
		// 本地设置下一个Section
		AnimInstance->Montage_SetNextSection(FromSectionName, ToSectionName, AnimMontageInfo.LocalMontageInfo.AnimMontage);

		if (IsOwnerActorAuthoritative())
		{
			// 服务器：更新复制属性，同步到模拟客户端
			AnimMontage_UpdateReplicatedDataForMesh(InMesh);
		}
		else
		{
			// 客户端：发送ServerRPC，包含当前位置用于服务器校正
			float CurrentPosition = AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage);
			ServerCurrentMontageSetNextSectionNameForMesh(InMesh, AnimMontageInfo.LocalMontageInfo.AnimMontage, CurrentPosition, FromSectionName, ToSectionName);
		}
	}
}

void UYcAbilitySystemComponent::CurrentMontageSetPlayRateForMesh(USkeletalMeshComponent* InMesh, float InPlayRate)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);
	if (AnimMontageInfo.LocalMontageInfo.AnimMontage && AnimInstance)
	{
		// 本地设置播放速率
		AnimInstance->Montage_SetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage, InPlayRate);

		if (IsOwnerActorAuthoritative())
		{
			// 服务器：更新复制属性，同步到模拟客户端
			AnimMontage_UpdateReplicatedDataForMesh(InMesh);
		}
		else
		{
			// 客户端：发送ServerRPC
			ServerCurrentMontageSetPlayRateForMesh(InMesh, AnimMontageInfo.LocalMontageInfo.AnimMontage, InPlayRate);
		}
	}
}

bool UYcAbilitySystemComponent::IsAnimatingAbilityForAnyMesh(UGameplayAbility* InAbility) const
{
	for (FGameplayAbilityLocalAnimMontageForMesh GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		if (GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility == InAbility)
		{
			return true;
		}
	}

	return false;
}

UGameplayAbility* UYcAbilitySystemComponent::GetAnimatingAbilityFromAnyMesh()
{
	// 同一时间只能有一个技能在所有骨骼网格上播放动画
	for (FGameplayAbilityLocalAnimMontageForMesh& GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		if (GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility.IsValid())
		{
			return GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimatingAbility.Get();
		}
	}

	return nullptr;
}

TArray<UAnimMontage*> UYcAbilitySystemComponent::GetCurrentMontages() const
{
	TArray<UAnimMontage*> Montages;

	for (FGameplayAbilityLocalAnimMontageForMesh GameplayAbilityLocalAnimMontageForMesh : LocalAnimMontageInfoForMeshes)
	{
		UAnimInstance* AnimInstance = IsValid(GameplayAbilityLocalAnimMontageForMesh.Mesh) 
			&& GameplayAbilityLocalAnimMontageForMesh.Mesh->GetOwner() == AbilityActorInfo->AvatarActor ? GameplayAbilityLocalAnimMontageForMesh.Mesh->GetAnimInstance() : nullptr;

		if (GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimMontage && AnimInstance 
			&& AnimInstance->Montage_IsActive(GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimMontage))
		{
			Montages.Add(GameplayAbilityLocalAnimMontageForMesh.LocalMontageInfo.AnimMontage);
		}
	}

	return Montages;
}

UAnimMontage* UYcAbilitySystemComponent::GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

	if (AnimMontageInfo.LocalMontageInfo.AnimMontage && AnimInstance
		&& AnimInstance->Montage_IsActive(AnimMontageInfo.LocalMontageInfo.AnimMontage))
	{
		return AnimMontageInfo.LocalMontageInfo.AnimMontage;
	}

	return nullptr;
}

int32 UYcAbilitySystemComponent::GetCurrentMontageSectionIDForMesh(USkeletalMeshComponent* InMesh)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	UAnimMontage* CurrentAnimMontage = GetCurrentMontageForMesh(InMesh);

	if (CurrentAnimMontage && AnimInstance)
	{
		float MontagePosition = AnimInstance->Montage_GetPosition(CurrentAnimMontage);
		return CurrentAnimMontage->GetSectionIndexFromPosition(MontagePosition);
	}

	return INDEX_NONE;
}

FName UYcAbilitySystemComponent::GetCurrentMontageSectionNameForMesh(USkeletalMeshComponent* InMesh)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	UAnimMontage* CurrentAnimMontage = GetCurrentMontageForMesh(InMesh);

	if (CurrentAnimMontage && AnimInstance)
	{
		float MontagePosition = AnimInstance->Montage_GetPosition(CurrentAnimMontage);
		int32 CurrentSectionID = CurrentAnimMontage->GetSectionIndexFromPosition(MontagePosition);

		return CurrentAnimMontage->GetSectionName(CurrentSectionID);
	}

	return NAME_None;
}

float UYcAbilitySystemComponent::GetCurrentMontageSectionLengthForMesh(USkeletalMeshComponent* InMesh)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	UAnimMontage* CurrentAnimMontage = GetCurrentMontageForMesh(InMesh);

	if (CurrentAnimMontage && AnimInstance)
	{
		int32 CurrentSectionID = GetCurrentMontageSectionIDForMesh(InMesh);
		if (CurrentSectionID != INDEX_NONE)
		{
			TArray<FCompositeSection>& CompositeSections = CurrentAnimMontage->CompositeSections;

			// 如果后面还有Section，计算两个Section起始时间的差值
			if (CurrentSectionID < (CompositeSections.Num() - 1))
			{
				return (CompositeSections[CurrentSectionID + 1].GetTime() - CompositeSections[CurrentSectionID].GetTime());
			}
			// 如果是最后一个Section，计算到蒙太奇结束的时间
			else
			{
				return (CurrentAnimMontage->GetPlayLength() - CompositeSections[CurrentSectionID].GetTime());
			}
		}

		// 如果没有Section，返回整个蒙太奇的时长
		return CurrentAnimMontage->GetPlayLength();
	}

	return 0.f;
}

float UYcAbilitySystemComponent::GetCurrentMontageSectionTimeLeftForMesh(USkeletalMeshComponent* InMesh)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	UAnimMontage* CurrentAnimMontage = GetCurrentMontageForMesh(InMesh);

	if (CurrentAnimMontage && AnimInstance && AnimInstance->Montage_IsActive(CurrentAnimMontage))
	{
		const float CurrentPosition = AnimInstance->Montage_GetPosition(CurrentAnimMontage);
		return CurrentAnimMontage->GetSectionTimeLeftFromPos(CurrentPosition);
	}

	return -1.f;
}

FGameplayAbilityLocalAnimMontageForMesh& UYcAbilitySystemComponent::GetLocalAnimMontageInfoForMesh(USkeletalMeshComponent* InMesh)
{
	// 查找已存在的
	for (FGameplayAbilityLocalAnimMontageForMesh& MontageInfo : LocalAnimMontageInfoForMeshes)
	{
		if (MontageInfo.Mesh == InMesh)
		{
			return MontageInfo;
		}
	}

	// 不存在则创建新的
	FGameplayAbilityLocalAnimMontageForMesh MontageInfo = FGameplayAbilityLocalAnimMontageForMesh(InMesh);
	LocalAnimMontageInfoForMeshes.Add(MontageInfo);
	return LocalAnimMontageInfoForMeshes.Last();
}

FGameplayAbilityRepAnimMontageForMesh& UYcAbilitySystemComponent::GetGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh)
{
	// 查找已存在的
	for (FGameplayAbilityRepAnimMontageForMesh& RepMontageInfo : RepAnimMontageInfoForMeshes)
	{
		if (RepMontageInfo.Mesh == InMesh)
		{
			return RepMontageInfo;
		}
	}

	// 不存在则创建新的
	FGameplayAbilityRepAnimMontageForMesh RepMontageInfo = FGameplayAbilityRepAnimMontageForMesh(InMesh);
	RepAnimMontageInfoForMeshes.Add(RepMontageInfo);
	return RepAnimMontageInfoForMeshes.Last();
}

void UYcAbilitySystemComponent::OnPredictiveMontageRejectedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* PredictiveMontage)
{
	// 预测被拒绝时的淡出时间
	static const float MONTAGE_PREDICTION_REJECT_FADETIME = 0.25f;

	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	if (AnimInstance && PredictiveMontage)
	{
		// 如果预测的蒙太奇仍在播放，停止它
		if (AnimInstance->Montage_IsPlaying(PredictiveMontage))
		{
			AnimInstance->Montage_Stop(MONTAGE_PREDICTION_REJECT_FADETIME, PredictiveMontage);
		}
	}
}

void UYcAbilitySystemComponent::AnimMontage_UpdateReplicatedDataForMesh(USkeletalMeshComponent* InMesh)
{
	check(IsOwnerActorAuthoritative());

	AnimMontage_UpdateReplicatedDataForMesh(GetGameplayAbilityRepAnimMontageForMesh(InMesh));
}

void UYcAbilitySystemComponent::AnimMontage_UpdateReplicatedDataForMesh(FGameplayAbilityRepAnimMontageForMesh& OutRepAnimMontageInfo)
{
	UAnimInstance* AnimInstance = IsValid(OutRepAnimMontageInfo.Mesh) && OutRepAnimMontageInfo.Mesh->GetOwner() 
		== AbilityActorInfo->AvatarActor ? OutRepAnimMontageInfo.Mesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(OutRepAnimMontageInfo.Mesh);

	if (AnimInstance && AnimMontageInfo.LocalMontageInfo.AnimMontage)
	{
		// 复制蒙太奇资源引用
		OutRepAnimMontageInfo.RepMontageInfo.Animation = AnimMontageInfo.LocalMontageInfo.AnimMontage;

		// 获取停止状态
		bool bIsStopped = AnimInstance->Montage_GetIsStopped(AnimMontageInfo.LocalMontageInfo.AnimMontage);

		// 如果蒙太奇正在播放，更新动态参数
		if (!bIsStopped)
		{
			OutRepAnimMontageInfo.RepMontageInfo.PlayRate = AnimInstance->Montage_GetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage);
			OutRepAnimMontageInfo.RepMontageInfo.Position = AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage);
			OutRepAnimMontageInfo.RepMontageInfo.BlendTime = AnimInstance->Montage_GetBlendTime(AnimMontageInfo.LocalMontageInfo.AnimMontage);
		}

		// 如果停止状态发生变化，强制网络更新
		if (OutRepAnimMontageInfo.RepMontageInfo.IsStopped != bIsStopped)
		{
			OutRepAnimMontageInfo.RepMontageInfo.IsStopped = bIsStopped;

			// 动画开始或停止时，立即更新客户端
			if (AbilityActorInfo->AvatarActor != nullptr)
			{
				AbilityActorInfo->AvatarActor->ForceNetUpdate();
			}

			UpdateShouldTick();
		}

		// 复制NextSectionID（实际复制NextSectionID+1，以便能表示INDEX_NONE）
		int32 CurrentSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(OutRepAnimMontageInfo.RepMontageInfo.Position);
		if (CurrentSectionID != INDEX_NONE)
		{
			int32 NextSectionID = AnimInstance->Montage_GetNextSectionID(AnimMontageInfo.LocalMontageInfo.AnimMontage, CurrentSectionID);
			if (NextSectionID >= (256 - 1))
			{
				ABILITY_LOG(Error, TEXT("AnimMontage_UpdateReplicatedData. NextSectionID = %d.  RepAnimMontageInfo.Position: %.2f, CurrentSectionID: %d. LocalAnimMontageInfo.AnimMontage %s"),
					NextSectionID, OutRepAnimMontageInfo.RepMontageInfo.Position, CurrentSectionID, *GetNameSafe(AnimMontageInfo.LocalMontageInfo.AnimMontage));
				ensure(NextSectionID < (256 - 1));
			}
			OutRepAnimMontageInfo.RepMontageInfo.NextSectionID = uint8(NextSectionID + 1);
		}
		else
		{
			OutRepAnimMontageInfo.RepMontageInfo.NextSectionID = 0;
		}
	}
}

void UYcAbilitySystemComponent::AnimMontage_UpdateForcedPlayFlagsForMesh(FGameplayAbilityRepAnimMontageForMesh& OutRepAnimMontageInfo)
{
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(OutRepAnimMontageInfo.Mesh);

	OutRepAnimMontageInfo.RepMontageInfo.PlayInstanceId = AnimMontageInfo.LocalMontageInfo.PlayInstanceId;
}

void UYcAbilitySystemComponent::OnRep_ReplicatedAnimMontageForMesh()
{
	// 遍历所有复制的蒙太奇信息
	for (FGameplayAbilityRepAnimMontageForMesh& NewRepMontageInfoForMesh : RepAnimMontageInfoForMeshes)
	{
		// 获取对应Mesh的本地蒙太奇信息
		FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(NewRepMontageInfoForMesh.Mesh);

		UWorld* World = GetWorld();

		// 如果标记跳过PlayRate，使用默认值1.0
		if (NewRepMontageInfoForMesh.RepMontageInfo.bSkipPlayRate)
		{
			NewRepMontageInfoForMesh.RepMontageInfo.PlayRate = 1.f;
		}

		const bool bIsPlayingReplay = World && World->IsPlayingReplay();

		// 位置校正阈值：回放模式使用更大的容差，正常游戏使用0.1秒
		const float MONTAGE_REP_POS_ERR_THRESH = bIsPlayingReplay ? CVarReplayMontageErrorThreshold.GetValueOnGameThread() : 0.1f;

		// 获取AnimInstance，验证Mesh有效性
		UAnimInstance* AnimInstance = IsValid(NewRepMontageInfoForMesh.Mesh) && NewRepMontageInfoForMesh.Mesh->GetOwner()
			== AbilityActorInfo->AvatarActor ? NewRepMontageInfoForMesh.Mesh->GetAnimInstance() : nullptr;
		if (AnimInstance == nullptr || !IsReadyForReplicatedMontageForMesh())
		{
			// 还不能处理，标记待处理
			bPendingMontageRep = true;
			return;
		}
		bPendingMontageRep = false;

		// 只在非本地控制的客户端（模拟代理）上处理
		if (!AbilityActorInfo->IsLocallyControlled())
		{
			// 调试日志（通过控制台变量net.Montage.Debug启用）
			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("net.Montage.Debug"));
			bool DebugMontage = (CVar && CVar->GetValueOnGameThread() == 1);
			if (DebugMontage)
			{
				ABILITY_LOG(Warning, TEXT("\n\nOnRep_ReplicatedAnimMontage, %s"), *GetNameSafe(this));
				ABILITY_LOG(Warning, TEXT("\tAnimMontage: %s\n\tPlayRate: %f\n\tPosition: %f\n\tBlendTime: %f\n\tNextSectionID: %d\n\tIsStopped: %d\n\tForcePlayBit: %d"),
					*GetNameSafe(NewRepMontageInfoForMesh.RepMontageInfo.GetAnimMontage()),
					NewRepMontageInfoForMesh.RepMontageInfo.PlayRate,
					NewRepMontageInfoForMesh.RepMontageInfo.Position,
					NewRepMontageInfoForMesh.RepMontageInfo.BlendTime,
					NewRepMontageInfoForMesh.RepMontageInfo.NextSectionID,
					NewRepMontageInfoForMesh.RepMontageInfo.IsStopped,
					NewRepMontageInfoForMesh.RepMontageInfo.PlayInstanceId);
				ABILITY_LOG(Warning, TEXT("\tLocalAnimMontageInfo.AnimMontage: %s\n\tPosition: %f"),
					*GetNameSafe(AnimMontageInfo.LocalMontageInfo.AnimMontage), AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage));
			}

			if (NewRepMontageInfoForMesh.RepMontageInfo.GetAnimMontage())
			{
				// ===== 检查是否需要播放新蒙太奇 =====
				const bool ReplicatedPlayBit = bool(NewRepMontageInfoForMesh.RepMontageInfo.PlayInstanceId);
				// 蒙太奇变化或PlayInstanceId变化时，需要重新播放
				if ((AnimMontageInfo.LocalMontageInfo.AnimMontage != NewRepMontageInfoForMesh.RepMontageInfo.GetAnimMontage()) || (bool(AnimMontageInfo.LocalMontageInfo.PlayInstanceId) != ReplicatedPlayBit))
				{
					AnimMontageInfo.LocalMontageInfo.PlayInstanceId = ReplicatedPlayBit;
					// 调用模拟端播放函数（不更新复制属性）
					PlayMontageSimulatedForMesh(NewRepMontageInfoForMesh.Mesh, NewRepMontageInfoForMesh.RepMontageInfo.GetAnimMontage(), NewRepMontageInfoForMesh.RepMontageInfo.PlayRate);
				}

				// 播放失败检查
				if (AnimMontageInfo.LocalMontageInfo.AnimMontage == nullptr)
				{
					ABILITY_LOG(Warning, TEXT("OnRep_ReplicatedAnimMontage: PlayMontageSimulated failed. Name: %s, AnimMontage: %s"), *GetNameSafe(this), *GetNameSafe(NewRepMontageInfoForMesh.RepMontageInfo.GetAnimMontage()));
					return;
				}

				// ===== 同步播放速率 =====
				if (AnimInstance->Montage_GetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage) != NewRepMontageInfoForMesh.RepMontageInfo.PlayRate)
				{
					AnimInstance->Montage_SetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage, NewRepMontageInfoForMesh.RepMontageInfo.PlayRate);
				}

				// ===== 处理停止状态 =====
				const bool bIsStopped = AnimInstance->Montage_GetIsStopped(AnimMontageInfo.LocalMontageInfo.AnimMontage);
				const bool bReplicatedIsStopped = bool(NewRepMontageInfoForMesh.RepMontageInfo.IsStopped);

				// 优先处理停止，避免Section切换导致混合弹出
				if (bReplicatedIsStopped)
				{
					if (!bIsStopped)
					{
						// 服务器已停止，本地也停止
						CurrentMontageStopForMesh(NewRepMontageInfoForMesh.Mesh, NewRepMontageInfoForMesh.RepMontageInfo.BlendTime);
					}
				}
				else if (!NewRepMontageInfoForMesh.RepMontageInfo.SkipPositionCorrection)
				{
					// ===== 位置和Section校正 =====
					const int32 RepSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(NewRepMontageInfoForMesh.RepMontageInfo.Position);
					const int32 RepNextSectionID = int32(NewRepMontageInfoForMesh.RepMontageInfo.NextSectionID) - 1;

					// 同步NextSectionID
					if (RepSectionID != INDEX_NONE)
					{
						const int32 NextSectionID = AnimInstance->Montage_GetNextSectionID(AnimMontageInfo.LocalMontageInfo.AnimMontage, RepSectionID);

						// 如果NextSectionID不同，设置正确的NextSection
						if (NextSectionID != RepNextSectionID)
						{
							AnimInstance->Montage_SetNextSection(AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionName(RepSectionID), AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionName(RepNextSectionID), AnimMontageInfo.LocalMontageInfo.AnimMontage);
						}

						// 检查客户端是否在错误的Section
						const int32 CurrentSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage));
						if ((CurrentSectionID != RepSectionID) && (CurrentSectionID != RepNextSectionID))
						{
							// 客户端在错误的Section，传送到正确Section的开始位置
							const float SectionStartTime = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetAnimCompositeSection(RepSectionID).GetTime();
							AnimInstance->Montage_SetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage, SectionStartTime);
						}
					}

					// ===== 位置校正 =====
					const float CurrentPosition = AnimInstance->Montage_GetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage);
					const int32 CurrentSectionID = AnimMontageInfo.LocalMontageInfo.AnimMontage->GetSectionIndexFromPosition(CurrentPosition);
					const float DeltaPosition = NewRepMontageInfoForMesh.RepMontageInfo.Position - CurrentPosition;

					// 只在同一Section内检查阈值（不同Section的位置差计算更复杂）
					if ((CurrentSectionID == RepSectionID) && (FMath::Abs(DeltaPosition) > MONTAGE_REP_POS_ERR_THRESH) && (NewRepMontageInfoForMesh.RepMontageInfo.IsStopped == 0))
					{
						// 快进到服务器位置并触发通知
						if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(NewRepMontageInfoForMesh.RepMontageInfo.GetAnimMontage()))
						{
							// 如果是向后跳转，跳过触发通知（已经触发过了）
							const float DeltaTime = !FMath::IsNearlyZero(NewRepMontageInfoForMesh.RepMontageInfo.PlayRate) ? (DeltaPosition / NewRepMontageInfoForMesh.RepMontageInfo.PlayRate) : 0.f;
							if (DeltaTime >= 0.f)
							{
								MontageInstance->UpdateWeight(DeltaTime);
								MontageInstance->HandleEvents(CurrentPosition, NewRepMontageInfoForMesh.RepMontageInfo.Position, nullptr);
								AnimInstance->TriggerAnimNotifies(DeltaTime);
							}
						}
						// 跳转到服务器位置
						AnimInstance->Montage_SetPosition(AnimMontageInfo.LocalMontageInfo.AnimMontage, NewRepMontageInfoForMesh.RepMontageInfo.Position);
					}
				}
			}
		}
	}
}

bool UYcAbilitySystemComponent::IsReadyForReplicatedMontageForMesh()
{
	/** 子类可以覆盖此函数添加额外检查（如"皮肤是否已应用"） */
	return true;
}

void UYcAbilitySystemComponent::ServerCurrentMontageSetNextSectionNameForMesh_Implementation(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float ClientPosition, FName SectionName, FName NextSectionName)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

	if (AnimInstance)
	{
		UAnimMontage* CurrentAnimMontage = AnimMontageInfo.LocalMontageInfo.AnimMontage;
		// 验证客户端蒙太奇与服务器一致
		if (ClientAnimMontage == CurrentAnimMontage)
		{
			// 设置NextSection
			AnimInstance->Montage_SetNextSection(SectionName, NextSectionName, CurrentAnimMontage);

			// 位置校正：检查服务器是否在正确的Section
			float CurrentPosition = AnimInstance->Montage_GetPosition(CurrentAnimMontage);
			int32 CurrentSectionID = CurrentAnimMontage->GetSectionIndexFromPosition(CurrentPosition);
			FName CurrentSectionName = CurrentAnimMontage->GetSectionName(CurrentSectionID);

			int32 ClientSectionID = CurrentAnimMontage->GetSectionIndexFromPosition(ClientPosition);
			FName ClientCurrentSectionName = CurrentAnimMontage->GetSectionName(ClientSectionID);
			
			// 如果服务器Section与客户端不一致，跳转到客户端位置
			if ((CurrentSectionName != ClientCurrentSectionName) || (CurrentSectionName != SectionName))
			{
				// 服务器在错误的Section，跳转到客户端位置
				AnimInstance->Montage_SetPosition(CurrentAnimMontage, ClientPosition);
			}

			// 更新复制属性，同步到模拟代理
			if (IsOwnerActorAuthoritative())
			{
				AnimMontage_UpdateReplicatedDataForMesh(InMesh);
			}
		}
	}
}

bool UYcAbilitySystemComponent::ServerCurrentMontageSetNextSectionNameForMesh_Validate(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float ClientPosition, FName SectionName, FName NextSectionName)
{
	return true;
}

void UYcAbilitySystemComponent::ServerCurrentMontageJumpToSectionNameForMesh_Implementation(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, FName SectionName)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

	if (AnimInstance)
	{
		UAnimMontage* CurrentAnimMontage = AnimMontageInfo.LocalMontageInfo.AnimMontage;
		// 验证客户端蒙太奇与服务器一致
		if (ClientAnimMontage == CurrentAnimMontage)
		{
			// 执行Section跳转
			AnimInstance->Montage_JumpToSection(SectionName, CurrentAnimMontage);

			// 更新复制属性，同步到模拟代理
			if (IsOwnerActorAuthoritative())
			{
				AnimMontage_UpdateReplicatedDataForMesh(InMesh);
			}
		}
	}
}

bool UYcAbilitySystemComponent::ServerCurrentMontageJumpToSectionNameForMesh_Validate(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, FName SectionName)
{
	return true;
}

void UYcAbilitySystemComponent::ServerCurrentMontageSetPlayRateForMesh_Implementation(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float InPlayRate)
{
	UAnimInstance* AnimInstance = IsValid(InMesh) && InMesh->GetOwner() == AbilityActorInfo->AvatarActor ? InMesh->GetAnimInstance() : nullptr;
	FGameplayAbilityLocalAnimMontageForMesh& AnimMontageInfo = GetLocalAnimMontageInfoForMesh(InMesh);

	if (AnimInstance)
	{
		UAnimMontage* CurrentAnimMontage = AnimMontageInfo.LocalMontageInfo.AnimMontage;
		// 验证客户端蒙太奇与服务器一致
		if (ClientAnimMontage == CurrentAnimMontage)
		{
			// 设置播放速率
			AnimInstance->Montage_SetPlayRate(AnimMontageInfo.LocalMontageInfo.AnimMontage, InPlayRate);

			// 更新复制属性，同步到模拟代理
			if (IsOwnerActorAuthoritative())
			{
				AnimMontage_UpdateReplicatedDataForMesh(InMesh);
			}
		}
	}
}

bool UYcAbilitySystemComponent::ServerCurrentMontageSetPlayRateForMesh_Validate(USkeletalMeshComponent* InMesh, UAnimMontage* ClientAnimMontage, float InPlayRate)
{
	return true;
}
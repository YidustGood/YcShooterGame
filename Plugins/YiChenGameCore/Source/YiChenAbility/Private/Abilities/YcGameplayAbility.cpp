// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Abilities/YcGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "YcAbilitySourceInterface.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayEffectContext.h"
#include "YcGameplayTags.h"
#include "YiChenAbility.h"
#include "Abilities/YcAbilityCost.h"
#include "Abilities/YcAbilitySimpleFailureMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility)

/** 确保Ability已实例化的宏，如果未实例化则输出错误日志并直接返回函数 */
#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue)																				\
{																																						\
if (!ensure(IsInstantiated()))																														\
{																																					\
ABILITY_LOG(Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName());	\
return ReturnValue;																																\
}																																					\
}

UYcGameplayAbility::UYcGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EYcAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EYcAbilityActivationGroup::Independent;
}

void UYcGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	K2_OnAbilityAdded();// 转发调用蓝图实现的函数

	TryActivateAbilityOnSpawn(ActorInfo, Spec); // 技能添加后调用尝试激活, 内部会进行条件判断
}

void UYcGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	K2_OnAbilityRemoved();// 转发调用蓝图实现的函数

	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UYcGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent,
                                                           const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
                                                           FGameplayTagContainer* OptionalRelevantTags) const
{
	// 通过ASC处理死亡排除和能力标签扩展的专用版本

	bool bBlocked = false;
	bool bMissing = false;
	
	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	const FGameplayTag& BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
	const FGameplayTag& MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;
	
	// 检查技能的标签是否被阻止
	if (AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags()))
	{
		bBlocked = true;
	}
	
	const UYcAbilitySystemComponent* ASC = Cast<UYcAbilitySystemComponent>(&AbilitySystemComponent);
	static FGameplayTagContainer AllRequiredTags;
	static FGameplayTagContainer AllBlockedTags;
	
	AllRequiredTags = ActivationRequiredTags;
	AllBlockedTags = ActivationBlockedTags;
	
	// 通过ASC扩展技能标签，添加额外的必需/阻止标签
	if (ASC)
	{
		ASC->GetAdditionalActivationTagRequirements(AbilityTags, AllRequiredTags, AllBlockedTags);
	}
	
	// 检查技能的必需/阻止标签
	if (AllBlockedTags.Num() || AllRequiredTags.Num())
	{
		static FGameplayTagContainer AbilitySystemComponentTags;

		AbilitySystemComponentTags.Reset();
		AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);

		if (AbilitySystemComponentTags.HasAny(AllBlockedTags))
		{
		if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(YcGameplayTags::Status_Death))
		{
			// 如果玩家已死亡且因阻止标签而被拒绝，添加反馈标签
			OptionalRelevantTags->AddTag(YcGameplayTags::Ability_ActivateFail_IsDead);
		}

			bBlocked = true;
		}

		if (!AbilitySystemComponentTags.HasAll(AllRequiredTags))
		{
			bMissing = true;
		}
	}

	if (SourceTags != nullptr)
	{
		// 检查技能释放者(Owner)的SourceTags中是否存在阻挡Tag或是否拥有全部依赖的Tag，以此来设定状态
		if (SourceBlockedTags.Num() || SourceRequiredTags.Num())
		{
			if (SourceTags->HasAny(SourceBlockedTags))
			{
				bBlocked = true;
			}

			if (!SourceTags->HasAll(SourceRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	// 检查技能作用目标（Avatar）的标签
	if (TargetTags != nullptr)
	{
		if (TargetBlockedTags.Num() || TargetRequiredTags.Num())
		{
			if (TargetTags->HasAny(TargetBlockedTags))
			{
				bBlocked = true;
			}

			if (!TargetTags->HasAll(TargetRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	if (bBlocked)
	{
		if (OptionalRelevantTags && BlockedTag.IsValid())
		{
			OptionalRelevantTags->AddTag(BlockedTag);
		}
		return false;
	}
	if (bMissing)
	{
		if (OptionalRelevantTags && MissingTag.IsValid())
		{
			OptionalRelevantTags->AddTag(MissingTag);
		}
		return false;
	}

	return true;
}

FGameplayEffectContextHandle UYcGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

	FYcGameplayEffectContext* EffectContext = FYcGameplayEffectContext::ExtractEffectContext(ContextHandle);
	check(EffectContext);

	check(ActorInfo);

	AActor* EffectCauser = nullptr;
	const IYcAbilitySourceInterface* AbilitySource = nullptr;
	float SourceLevel = 0.0f;
	GetAbilitySource(Handle, ActorInfo, /*out*/ SourceLevel, /*out*/ AbilitySource, /*out*/ EffectCauser);

	const UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

	AActor* Instigator = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;

	EffectContext->SetAbilitySource(AbilitySource, SourceLevel);
	EffectContext->AddInstigator(Instigator, EffectCauser);
	EffectContext->AddSourceObject(SourceObject);

	return ContextHandle;
}

bool UYcGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}

	// 检查我们是否能够承担额外的所有费用(Cost)
	for (const TObjectPtr<UYcAbilityCost>& AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			if (!AdditionalCost->CheckCost(this, Handle, ActorInfo, /*inout*/ OptionalRelevantTags))
			{
				return false;
			}
		}
	}

	return true;
}

void UYcGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
	
	// 用于判断该能力是否确实击中了目标的lambda函数（因为某些费用仅在成功攻击时才会消耗）
	auto DetermineIfAbilityHitTarget = [&]()
	{
		if (!ActorInfo->IsNetAuthority()) return false;
		UYcAbilitySystemComponent* ASC = Cast<UYcAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
		if (ASC == nullptr) return false;
		
		FGameplayAbilityTargetDataHandle TargetData;
		ASC->GetAbilityTargetData(Handle, ActivationInfo, TargetData);
		for (int32 TargetDataIdx = 0; TargetDataIdx < TargetData.Data.Num(); ++TargetDataIdx)
		{
			if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetData, TargetDataIdx))
			{
				return true;
			}
		}
		
		return false;
	};
	
	// 处理支付所有的额外费用
	bool bAbilityHitTarget = false;
	bool bHasDeterminedIfAbilityHitTarget = false;
	for (const TObjectPtr<UYcAbilityCost>& AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost == nullptr) continue;
		
		if (AdditionalCost->ShouldOnlyApplyCostOnHit())
		{
			if (!bHasDeterminedIfAbilityHitTarget)
			{
				bAbilityHitTarget = DetermineIfAbilityHitTarget();
				bHasDeterminedIfAbilityHitTarget = true;
			}

			if (!bAbilityHitTarget)
			{
				continue;
			}
		}

		AdditionalCost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);
	}
}

void UYcGameplayAbility::GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                          float& OutSourceLevel, const IYcAbilitySourceInterface*& OutAbilitySource, AActor*& OutEffectCauser) const
{
	OutSourceLevel = 0.0f;
	OutAbilitySource = nullptr;
	OutEffectCauser = nullptr;

	OutEffectCauser = ActorInfo->AvatarActor.Get();

	// 如果技能是由实现了IYcAbilitySourceInterface的对象授予的，则使用它作为源
	UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

	OutAbilitySource = Cast<IYcAbilitySourceInterface>(SourceObject);
}

UYcAbilitySystemComponent* UYcGameplayAbility::GetYcAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UYcAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AController* UYcGameplayAbility::GetControllerFromActorInfo() const
{
	if (!CurrentActorInfo) return nullptr;
	
	if (AController* PC = CurrentActorInfo->PlayerController.Get())
	{
		return PC;
	}

	// 在所有者链中寻找玩家控制器或Pawn
	AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
	while (TestActor)
	{
		if (AController* C = Cast<AController>(TestActor))
		{
			return C;
		}

		if (const APawn* Pawn = Cast<APawn>(TestActor))
		{
			return Pawn->GetController();
		}
		
		TestActor = TestActor->GetOwner();
	}

	return nullptr;
}

bool UYcGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) return false;
	
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	
	// @TODO 可能在设置标签关系后删除
	UYcAbilitySystemComponent* YcASC = CastChecked<UYcAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	if (YcASC->IsActivationGroupBlocked(ActivationGroup))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(YcGameplayTags::Ability_ActivateFail_ActivationGroup);
		}
		return false;
	}
	
	return true;
}

void UYcGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled)
{
	// 如果该能力属于“可被替换”类型，则它不能阻止被取消。
	if (!bCanBeCanceled && (ActivationGroup == EYcAbilityActivationGroup::Exclusive_Replaceable))
	{
		UE_LOG(LogYcAbilitySystem, Error, TEXT("SetCanBeCanceled: Ability [%s] can not block canceling because its activation group is replaceable."), *GetName());
		return;
	}
	Super::SetCanBeCanceled(bCanBeCanceled);
}

void UYcGameplayAbility::OnPawnAvatarSet()
{
	// 触发蓝图事件
	K2_OnPawnAvatarSet();
}

void UYcGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilitySpec& Spec) const
{
	const bool bIsPredicting = (Spec.ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting);

	// 如果激活策略不是OnSpawn则不激活
	if (!ActorInfo || Spec.IsActive() || bIsPredicting || ActivationPolicy != EYcAbilityActivationPolicy::OnSpawn) return;
	
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// 如果Avatar被TearOff或即将销毁，则不激活
	if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
	{
		const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
		const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

		const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
		const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

		if (bClientShouldActivate || bServerShouldActivate)
		{
			ASC->TryActivateAbility(Spec.Handle);
		}
	}
}

void UYcGameplayAbility::ExternalEndAbility()
{
	check(CurrentActorInfo);

	// 外部结束技能，确保网络同步
	bool bReplicateEndAbility = true;
	bool bWasCancelled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UYcGameplayAbility::CanChangeActivationGroup(EYcAbilityActivationGroup NewGroup) const
{
	if (!IsInstantiated() || !IsActive()) return false;
	
	if (ActivationGroup == NewGroup) return true;

	UYcAbilitySystemComponent* YcASC = GetYcAbilitySystemComponentFromActorInfo();
	check(YcASC);
	
	if ((ActivationGroup != EYcAbilityActivationGroup::Exclusive_Blocking) && YcASC->IsActivationGroupBlocked(NewGroup))
	{
		// 如果目标分组当前被阻塞，此能力将无法切换分组（除非它自己就是造成阻塞的那个能力）。
		return false;
	}

	if ((NewGroup == EYcAbilityActivationGroup::Exclusive_Replaceable) && !CanBeCanceled())
	{
		// 如果此能力本身不可被取消，则它不能切换到“可被替换的排他性”分组。
		return false;
	}

	return true;
}

bool UYcGameplayAbility::ChangeActivationGroup(EYcAbilityActivationGroup NewGroup)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ChangeActivationGroup, false);

	if (!CanChangeActivationGroup(NewGroup)) return false;

	if (ActivationGroup != NewGroup)
	{
		UYcAbilitySystemComponent* YcASC = GetYcAbilitySystemComponentFromActorInfo();
		check(YcASC);

		YcASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
		YcASC->AddAbilityToActivationGroup(NewGroup, this);

		ActivationGroup = NewGroup;
	}

	return true;
}

void UYcGameplayAbility::NativeOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const
{
	// 遍历失败原因标签，查找映射表中是否有对应的用户友好消息
	bool bSimpleFailureFound = false;
	for (FGameplayTag Reason : FailedReason)
	{
		// 只处理第一个找到的匹配标签，避免重复广播消息
		if (!bSimpleFailureFound)
		{
			// 在映射表中查找该失败标签对应的用户友好消息
			if (const FText* pUserFacingMessage = FailureTagToUserFacingMessages.Find(Reason))
			{
				// 构建失败消息结构
				FYcAbilitySimpleFailureMessage Message;
				Message.PlayerController = GetActorInfo().PlayerController.Get();
				Message.FailureTags = FailedReason;
				Message.UserFacingReason = *pUserFacingMessage;
				
				// 通过GameplayMessageSubsystem广播技能激活失败消息，供UI系统订阅并显示用户提示
				UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
				MessageSystem.BroadcastMessage(TAG_ABILITY_SIMPLE_FAILURE_MESSAGE, Message);
				bSimpleFailureFound = true;
			}
		}
	}
}

/**
 * 获取指定骨骼网格上当前正在播放的蒙太奇
 * 从CurrentAbilityMeshMontages数组中查找对应Mesh的蒙太奇
 * @param InMesh 目标骨骼网格
 * @return 当前正在播放的蒙太奇，如果没有则返回nullptr
 */
UAnimMontage* UYcGameplayAbility::GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh)
{
	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbilityMeshMontage(InMesh, AbilityMeshMontage))
	{
		return AbilityMeshMontage.Montage;
	}

	return nullptr;
}

/**
 * 设置指定骨骼网格上当前正在播放的蒙太奇
 * 由蒙太奇Task（如AbilityTask_PlayMontageForMeshAndWaitForEvent）调用
 * 用于追踪技能正在播放的蒙太奇，允许将蒙太奇事件与技能事件关联起来
 * @param InMesh 目标骨骼网格
 * @param InCurrentMontage 当前蒙太奇，nullptr表示清除
 */
void UYcGameplayAbility::SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* InCurrentMontage)
{
	ensure(IsInstantiated());

	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbilityMeshMontage(InMesh, AbilityMeshMontage))
	{
		// 已存在则更新蒙太奇引用
		AbilityMeshMontage.Montage = InCurrentMontage;
	}
	else
	{
		// 不存在则添加新的Mesh-Montage映射
		CurrentAbilityMeshMontages.Add(FAbilityMeshMontage(InMesh, InCurrentMontage));
	}
}

/**
 * 查找指定骨骼网格对应的蒙太奇信息
 * 遍历CurrentAbilityMeshMontages数组查找匹配的Mesh
 * @param InMesh 目标骨骼网格
 * @param InAbilityMontage 输出：找到的蒙太奇信息
 * @return 如果找到返回true，否则返回false
 */
bool UYcGameplayAbility::FindAbilityMeshMontage(USkeletalMeshComponent* InMesh, FAbilityMeshMontage& InAbilityMeshMontage)
{
	for (FAbilityMeshMontage& MeshMontage : CurrentAbilityMeshMontages)
	{
		if (MeshMontage.Mesh == InMesh)
		{
			InAbilityMeshMontage = MeshMontage;
			return true;
		}
	}

	return false;
}

/**
 * 立即跳转到指定骨骼网格上蒙太奇的指定Section
 * 只有当此技能是当前正在播放动画的技能时才会执行跳转
 * 会通过ASC处理网络同步（服务器更新复制属性，客户端发送ServerRPC）
 * @param InMesh 目标骨骼网格
 * @param SectionName 目标Section名称
 */
void UYcGameplayAbility::MontageJumpToSectionForMesh(USkeletalMeshComponent* InMesh, FName SectionName)
{
	check(CurrentActorInfo);

	UYcAbilitySystemComponent* const AbilitySystemComponent = Cast<UYcAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	// 只有当此技能是当前正在播放动画的技能时才执行跳转
	if (AbilitySystemComponent->IsAnimatingAbilityForAnyMesh(this))
	{
		AbilitySystemComponent->CurrentMontageJumpToSectionForMesh(InMesh, SectionName);
	}
}

/**
 * 设置指定骨骼网格上蒙太奇的下一个Section
 * 用于实现蒙太奇的分支播放，如连击系统
 * 只有当此技能是当前正在播放动画的技能时才会执行设置
 * 会通过ASC处理网络同步（服务器更新复制属性，客户端发送ServerRPC）
 * @param InMesh 目标骨骼网格
 * @param FromSectionName 当前Section名称
 * @param ToSectionName 下一个Section名称
 */
void UYcGameplayAbility::MontageSetNextSectionNameForMesh(USkeletalMeshComponent* InMesh, FName FromSectionName, FName ToSectionName)
{
	check(CurrentActorInfo);

	UYcAbilitySystemComponent* const AbilitySystemComponent = Cast<UYcAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo_Checked());
	// 只有当此技能是当前正在播放动画的技能时才执行设置
	if (AbilitySystemComponent->IsAnimatingAbilityForAnyMesh(this))
	{
		AbilitySystemComponent->CurrentMontageSetNextSectionNameForMesh(InMesh, FromSectionName, ToSectionName);
	}
}

/**
 * 停止指定骨骼网格上当前播放的蒙太奇
 * 只有当此技能是当前正在播放动画的技能时才会停止
 * 会通过ASC处理网络同步（服务器更新复制属性，同步停止状态到模拟客户端）
 * @param InMesh 目标骨骼网格
 * @param OverrideBlendOutTime 混出时间覆盖，>=0时覆盖蒙太奇的默认BlendOutTime
 */
void UYcGameplayAbility::MontageStopForMesh(USkeletalMeshComponent* InMesh, float OverrideBlendOutTime)
{
	check(CurrentActorInfo);

	UYcAbilitySystemComponent* const AbilitySystemComponent = Cast<UYcAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
	if (AbilitySystemComponent != nullptr)
	{
		// 只有当此技能是当前正在播放动画的技能时才停止蒙太奇
		if (AbilitySystemComponent->IsAnimatingAbilityForAnyMesh(this))
		{
			AbilitySystemComponent->CurrentMontageStopForMesh(InMesh, OverrideBlendOutTime);
		}
	}
}

/**
 * 停止所有骨骼网格上当前播放的蒙太奇
 * 只有当此技能是当前正在播放动画的技能时才会停止
 * 会通过ASC处理网络同步（服务器更新复制属性，同步停止状态到模拟客户端）
 * @param OverrideBlendOutTime 混出时间覆盖，>=0时覆盖蒙太奇的默认BlendOutTime
 */
void UYcGameplayAbility::MontageStopForAllMeshes(float OverrideBlendOutTime)
{
	check(CurrentActorInfo);

	UYcAbilitySystemComponent* const AbilitySystemComponent = Cast<UYcAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
	if (AbilitySystemComponent != nullptr)
	{
		// 只有当此技能是当前正在播放动画的技能时才停止所有蒙太奇
		if (AbilitySystemComponent->IsAnimatingAbilityForAnyMesh(this))
		{
			AbilitySystemComponent->StopAllCurrentMontages(OverrideBlendOutTime);
		}
	}
}
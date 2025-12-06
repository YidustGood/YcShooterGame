// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Abilities/YcGameplayAbility.h"

#include "AbilitySystemGlobals.h"
#include "YcAbilitySourceInterface.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayEffectContext.h"
#include "YcGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility)

UYcGameplayAbility::UYcGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
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
	// Specialized version to handle death exclusion and AbilityTags expansion via ASC
	// 通过ASC处理死亡排除和能力标签扩展的专用版本

	bool bBlocked = false;
	bool bMissing = false;
	
	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	const FGameplayTag& BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
	const FGameplayTag& MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;
	
	// Check if any of this ability's tags are currently blocked
	if (AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags()))
	{
		bBlocked = true;
	}
	
	const UYcAbilitySystemComponent* ASC = Cast<UYcAbilitySystemComponent>(&AbilitySystemComponent);
	static FGameplayTagContainer AllRequiredTags;
	static FGameplayTagContainer AllBlockedTags;
	
	AllRequiredTags = ActivationRequiredTags;
	AllBlockedTags = ActivationBlockedTags;
	
	// Expand our ability tags to add additional required/blocked tags
	if (ASC)
	{
		ASC->GetAdditionalActivationTagRequirements(AbilityTags, AllRequiredTags, AllBlockedTags);
	}
	
	// Check to see the required/blocked tags for this ability
	if (AllBlockedTags.Num() || AllRequiredTags.Num())
	{
		static FGameplayTagContainer AbilitySystemComponentTags;

		AbilitySystemComponentTags.Reset();
		AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);

		if (AbilitySystemComponentTags.HasAny(AllBlockedTags))
		{
			if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(YcGameplayTags::Status_Death))
			{
				// If player is dead and was rejected due to blocking tags, give that feedback
				//如果玩家已经死亡，并且由于阻塞标签而被拒绝，给出反馈
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

	// 技能作用目标(Avatr?)
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

void UYcGameplayAbility::GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                          float& OutSourceLevel, const IYcAbilitySourceInterface*& OutAbilitySource, AActor*& OutEffectCauser) const
{
	OutSourceLevel = 0.0f;
	OutAbilitySource = nullptr;
	OutEffectCauser = nullptr;

	OutEffectCauser = ActorInfo->AvatarActor.Get();

	// If we were added by something that's an ability info source, use it
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
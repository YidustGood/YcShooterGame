// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcShooterHealthComponent.h"
#include "NativeGameplayTags.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "YiChenShooterCore.h"
#include "AbilitySystem/Attributes/YcShooterHealthSet.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayCommon/YcGameVerbMessage.h"
#include "Net/UnrealNetwork.h"
#include "System/YcAssetManager.h"
#include "System/YcGameData.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcShooterHealthComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Elimination_Message, "Yc.Elimination.Message");

UYcShooterHealthComponent::UYcShooterHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	AbilitySystemComponent = nullptr;
	DeathState = EYcDeathState::NotDead;
	
	HealthSet = nullptr;
}

void UYcShooterHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UYcShooterHealthComponent, DeathState);
}

void UYcShooterHealthComponent::DoInitializeWithAbilitySystem(UYcAbilitySystemComponent* ASC)
{
	Super::DoInitializeWithAbilitySystem(ASC);
	
	const AActor* Owner = GetOwner();
	check(Owner);
	
	// 创建健康组件所需的AttributeSet
	HealthSet = AddAttributeSet<UYcShooterHealthSet>();
	
	// 验证属性集创建成功
	if (!HealthSet)
	{
		UE_LOG(LogYcShooterCore, Error, TEXT("UYcShooterHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."), *GetNameSafe(Owner));
		return;
	}
	
	// 注意：如果需要获取已存在的属性集，可以使用GetAttributeSet<T>()函数
	// 例如：HealthSet = GetAttributeSet<UYcShooterHealthSet>();

	// 注册以监听属性更改
	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	// TEMP: 将属性重置为默认值。最终这将由电子表格驱动
	AbilitySystemComponent->SetNumericAttributeBase(UYcShooterHealthSet::GetHealthAttribute(), HealthSet->GetMaxHealth());

	ClearGameplayTags();

	// 进行数值初始化广播，以便UI显示初始化值
	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
}

void UYcShooterHealthComponent::DoUninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	// 移除委托
	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	// 清空属性引用
	HealthSet = nullptr;
	
	Super::DoUninitializeFromAbilitySystem();
}

float UYcShooterHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.0f);
}

float UYcShooterHealthComponent::GetMaxHealth() const
{
	return (HealthSet ? HealthSet->GetMaxHealth() : 0.0f);
}

float UYcShooterHealthComponent::GetHealthNormalized() const
{
	if(!HealthSet) return 0.0f;
	
	const float Health = HealthSet->GetHealth();
	const float MaxHealth = HealthSet->GetMaxHealth();

	return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
}

void UYcShooterHealthComponent::K2_DamageSelfDestruct(bool bFellOutOfWorld)
{
	DamageSelfDestruct(bFellOutOfWorld);
}

void UYcShooterHealthComponent::StartDeath()
{
	if (DeathState != EYcDeathState::NotDead) return;

	DeathState = EYcDeathState::DeathStarted;

	if (AbilitySystemComponent)
	{
		// 为ASC添加死亡中的标签
		AbilitySystemComponent->SetLooseGameplayTagCount(YcGameplayTags::Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	// 广播开始死亡
	OnDeathStarted.Broadcast(Owner);

	// 让Owner立刻进行网络更新, 因为开始死亡是一个重要的事情
	Owner->ForceNetUpdate();
}

void UYcShooterHealthComponent::FinishDeath()
{
	if (DeathState != EYcDeathState::DeathStarted) return;
	
	DeathState = EYcDeathState::DeathFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(YcGameplayTags::Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
}


void UYcShooterHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	// 要处于未死亡状态且ASC组件有效才能进行对自我的摧毁性伤害
	if ((DeathState != EYcDeathState::NotDead) || AbilitySystemComponent == nullptr) return;
	
	// 获取伤害GE类
	const TSubclassOf<UGameplayEffect> DamageGE = UYcAssetManager::GetSubclass(UYcGameData::Get().DamageGameplayEffect_SetByCaller);
	if (!DamageGE)
	{
		UE_LOG(LogYcShooterCore, Error, TEXT("UYcShooterHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to find gameplay effect [%s]."), *GetNameSafe(GetOwner()), *UYcGameData::Get().DamageGameplayEffect_SetByCaller.GetAssetName());
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DamageGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	if (!Spec)
	{
		UE_LOG(LogYcShooterCore, Error, TEXT("UYcShooterHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to make outgoing spec for [%s]."), *GetNameSafe(GetOwner()), *GetNameSafe(DamageGE));
		return;
	}

	/** 动态添加并非源自原始 GE 定义的资产标签；该标签既会被添加到动态资产标签中，也会被注入到捕获的源规格标签中 */
	Spec->AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct);

	if (bFellOutOfWorld)
	{
		Spec->AddDynamicAssetTag(TAG_Gameplay_FellOutOfWorld);
	}

	// 摧毁性伤害的数值就是当前的生命值
	const float DamageAmount = GetMaxHealth();

	Spec->SetSetByCallerMagnitude(TAG_Gameplay_Attribute_SetByCaller_Damage, DamageAmount);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
}

void UYcShooterHealthComponent::ClearGameplayTags()
{
	if (!AbilitySystemComponent) return;
	
	AbilitySystemComponent->SetLooseGameplayTagCount(YcGameplayTags::Status_Death_Dying, 0);
	AbilitySystemComponent->SetLooseGameplayTagCount(YcGameplayTags::Status_Death_Dead, 0);
}

void UYcShooterHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UYcShooterHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UYcShooterHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	// 这段代码只能在服务端执行
#if WITH_SERVER_CODE
	if (AbilitySystemComponent == nullptr || DamageEffectSpec == nullptr) return;
	
	// @TODO: 考虑到底是在这里调用StartDeath()和FinishDeath()还是在死亡技能里面调用, 两者有何区别?
	
	// 向ASC的AvatarActor发送"GameplayEvent.Death"事件，我们的死亡技能配置了"GameplayEvent.Death"触发器，故可触发死亡技能，然后在死亡技能里编写具体的死亡业务逻辑
	{
		FGameplayEventData DeathPayload;
		DeathPayload.EventTag = YcGameplayTags::GameplayEvent_Death;
		DeathPayload.Instigator = DamageInstigator;
		DeathPayload.Target = AbilitySystemComponent->GetAvatarActor();
		DeathPayload.OptionalObject = DamageEffectSpec->Def;
		DeathPayload.ContextHandle = DamageEffectSpec->GetEffectContext();
		DeathPayload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
		DeathPayload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
		DeathPayload.EventMagnitude = DamageMagnitude;
		
		FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
		AbilitySystemComponent->HandleGameplayEvent(DeathPayload.EventTag, &DeathPayload);
	}

	// 发送标准化的动作消息，供其他系统观察
	// 发送玩家被消灭的消息，以便其他系统可以观察到这个事件，例如负责击杀信息的UI通过监听TAG_Yc_Elimination_Message通道的消息响应玩家死亡事件
	{
		FYcGameVerbMessage Message;
		Message.Verb = TAG_Yc_Elimination_Message;
		// 发起伤害者（ASC挂载在PlayerState的话就是PlayerState，通常是如此）
		Message.Instigator = DamageInstigator;	
		Message.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
		// 被伤害者（ASC挂载在PlayerState的话就是PlayerState，通常是如此）
		Message.Target = UYcGameVerbMessageHelpers::GetPlayerStateFromObject(AbilitySystemComponent->GetAvatarActor());
		Message.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
		//@TODO: 填充上下文标签，以及任何非技能系统的源/发起者标签
		//@TODO: 确定是否为敌对团队击杀、自伤、团队击杀等

		UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
		MessageSystem.BroadcastMessage(Message.Verb, Message);
	}

	//@TODO: assist messages (could compute from damage dealt elsewhere)?
#endif // #if WITH_SERVER_CODE
}

void UYcShooterHealthComponent::OnRep_DeathState(EYcDeathState OldDeathState)
{
	// 客户端的StartDeath和FinishDeath都是通过DeathState的属性同步触发
	const EYcDeathState NewDeathState = DeathState;

	// 暂时恢复死亡状态，因为我们依赖StartDeath和FinishDeath来改变它
	DeathState = OldDeathState;

	if (OldDeathState > NewDeathState)
	{
		// 服务器试图让我们返回，但我们已经预测超过了服务器状态
		UE_LOG(LogYcShooterCore, Warning, TEXT("UYcShooterHealthComponent: Predicted past server death state [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		return;
	}

	if (OldDeathState == EYcDeathState::NotDead)
	{
		// NotDead -> DeathStarted
		if (NewDeathState == EYcDeathState::DeathStarted)
		{
			StartDeath();
		}
		// NotDead -> DeathFinished
		else if (NewDeathState == EYcDeathState::DeathFinished)
		{
			StartDeath();
			FinishDeath();
		}
		else
		{
			UE_LOG(LogYcShooterCore, Error, TEXT("UYcShooterHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}
	else if (OldDeathState == EYcDeathState::DeathStarted)
	{
		// DeathStarted -> DeathFinished
		if (NewDeathState == EYcDeathState::DeathFinished)
		{
			FinishDeath();
		}
		else
		{
			UE_LOG(LogYcShooterCore, Error, TEXT("UYcShooterHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}

	// 检查状态处理一致性
	ensureMsgf((DeathState == NewDeathState), TEXT("UYcShooterHealthComponent: Death transition failed [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
}
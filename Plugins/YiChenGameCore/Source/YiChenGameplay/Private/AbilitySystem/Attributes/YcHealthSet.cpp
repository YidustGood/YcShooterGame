// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AbilitySystem/Attributes/YcHealthSet.h"

#include "GameplayEffectExtension.h"
#include "NativeGameplayTags.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayCommon/YcGameVerbMessage.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHealthSet)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Damage, "Gameplay.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageImmunity, "Gameplay.DamageImmunity");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageSelfDestruct, "Gameplay.Damage.SelfDestruct");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_FellOutOfWorld, "Gameplay.Damage.FellOutOfWorld");
UE_DEFINE_GAMEPLAY_TAG(TAG_Yc_Damage_Message, "Yc.Damage.Message");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Attribute_SetByCaller_Heal, "Gameplay.Attribute.SetByCaller.Heal");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Attribute_SetByCaller_Damage, "Gameplay.Attribute.SetByCaller.Damage");

UYcHealthSet::UYcHealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
{
	bOutOfHealth = false;
	MaxHealthBeforeAttributeChange = 0.0f;
	HealthBeforeAttributeChange = 0.0f;
}

void UYcHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UYcHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYcHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UYcHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcHealthSet, Health, OldValue);

	// 调用变化回调，但没有instigator信息（客户端上可能缺失）
	// 未来可以改为显式RPC
	// 客户端上的这些事件不应该修改属性

	const float CurrentHealth = GetHealth();
	const float EstimatedMagnitude = CurrentHealth - OldValue.GetCurrentValue();
	
	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);

	// 如果之前未归零但现在归零了，触发归零事件
	if (!bOutOfHealth && CurrentHealth <= 0.0f)
	{
		OnOutOfHealth.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentHealth);
	}

	bOutOfHealth = (CurrentHealth <= 0.0f);
}

void UYcHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcHealthSet, MaxHealth, OldValue);

	// 调用变化回调，但没有instigator信息（客户端上可能缺失）
	// 未来可以改为显式RPC
	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxHealth() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxHealth());
}

bool UYcHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}
	
	// 处理普通伤害的修改
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct);

			// 检查伤害免疫（自毁伤害除外）
			if (Data.Target.HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity) && !bIsDamageFromSelfDestruct)
			{
				// 不扣除任何健康值
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}

#if !UE_BUILD_SHIPPING
			// 检查GodMode作弊（自毁伤害除外），无限生命值在下面检查
			if (Data.Target.HasMatchingGameplayTag(YcGameplayTags::Cheat_GodMode) && !bIsDamageFromSelfDestruct)
			{
				// 不扣除任何健康值
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}
#endif // #if !UE_BUILD_SHIPPING
		}
	}

	// 保存当前健康值，用于后续计算变化量
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();

	return true;
}

void UYcHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(TAG_Gameplay_DamageSelfDestruct);
	float MinimumHealth = 0.0f;

#if !UE_BUILD_SHIPPING
	// GodMode和无限生命值阻止死亡（自毁伤害除外）
	if (!bIsDamageFromSelfDestruct &&
		(Data.Target.HasMatchingGameplayTag(YcGameplayTags::Cheat_GodMode) || Data.Target.HasMatchingGameplayTag(YcGameplayTags::Cheat_UnlimitedHealth) ))
	{
		MinimumHealth = 1.0f;
	}
#endif // #if !UE_BUILD_SHIPPING

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	// 获取效果发起者（默认指向攻击者的ASC组件持有对象，如果ASC挂载到PlayerState则为PlayerState对象）
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	// 获取效果原因者（默认指向攻击者的ASC组件的avatar actor，如果ASC由PlayerState持有，则这个指向玩家控制的角色对象）
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// 发送标准化的动作消息，供其他系统观察
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			FYcGameVerbMessage Message;
			Message.Verb = TAG_Yc_Damage_Message;
			Message.Instigator = Data.EffectSpec.GetEffectContext().GetEffectCauser();
			Message.InstigatorTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
			Message.Target = GetOwningActor();
			Message.TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();
			//@TODO: 填充上下文标签，以及任何非技能系统的源/发起者标签
			//@TODO: 确定是否为敌对团队击杀、自伤、团队击杀等
			Message.Magnitude = Data.EvaluatedData.Magnitude;

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}

		// 将Damage转换为-Health并限制范围：GE_Damage增加Damage值，然后用Health吸收Damage，然后将Damage归零，完成伤害扣血
		SetHealth(FMath::Clamp(GetHealth() - GetDamage(), MinimumHealth, GetMaxHealth()));
		SetDamage(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		// 将Healing转换为+Health并限制范围
		SetHealth(FMath::Clamp(GetHealth() + GetHealing(), MinimumHealth, GetMaxHealth()));
		SetHealing(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 限制范围并进入健康值归零处理逻辑
		SetHealth(FMath::Clamp(GetHealth(), MinimumHealth, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// TODO: 是否需要限制当前健康值？

		// 通知最大健康值的任何变化
		OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeAttributeChange, GetMaxHealth());
	}

	// 如果健康值实际发生了变化，触发回调
	if (GetHealth() != HealthBeforeAttributeChange)
	{
		OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}

	// 如果健康值归零且之前未归零，触发归零事件
	if ((GetHealth() <= 0.0f) && !bOutOfHealth)
	{
		OnOutOfHealth.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
	}

	// 再次检查健康值，以防上面的回调事件改变了它
	bOutOfHealth = (GetHealth() <= 0.0f);
}

void UYcHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UYcHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UYcHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		// 确保当前健康值不超过新的最大健康值
		if (GetHealth() > NewValue)
		{
			UYcAbilitySystemComponent* ASC = GetYcAbilitySystemComponent();
			check(ASC);

			ASC->ApplyModToAttribute(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
		}
	}

	// 如果之前归零但现在健康值大于0，更新状态
	if (bOutOfHealth && (GetHealth() > 0.0f))
	{
		bOutOfHealth = false;
	}

	// HealthSet中的属性变化情况打印（调试用）
	// const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue(Cast<AActor>(GetOuter())->GetLocalRole());
	// const FString Msg = FString::Printf(TEXT("HealthSet [%s] Post Change : NewValue(%f) - OldValue(%f) [%s]"), *Attribute.GetName(), NewValue, OldValue, *LocalRoleName);
	// UE_LOG(LogYcShooterCore, Log, *Msg);
}

void UYcHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		// 不允许健康值为负数或超过最大健康值
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// 不允许最大健康值低于1
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
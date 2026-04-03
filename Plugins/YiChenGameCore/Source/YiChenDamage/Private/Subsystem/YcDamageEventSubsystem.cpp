// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Subsystem/YcDamageEventSubsystem.h"

#include "Executions/YcDamageSummaryParams.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "YcDamageGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageEventSubsystem)

void UYcDamageEventSubsystem::BroadcastDamageEvent(const FYcDamageEventData& EventData, const FGameplayTagContainer& SourceTags)
{
	// 根据事件类型分发到不同的委托
	if (EventData.bWasDodged)
	{
		OnDamageDodged.Broadcast(EventData, SourceTags);
	}
	else if (EventData.bWasImmune)
	{
		OnDamageImmune.Broadcast(EventData, SourceTags);
	}
	else
	{
		// 正常伤害事件
		OnDamageApplied.Broadcast(EventData, SourceTags);

		// 暴击事件
		if (EventData.bWasCritical)
		{
			OnDamageCritical.Broadcast(EventData, SourceTags);
		}
	}
}

FYcDamageEventData UYcDamageEventSubsystem::CreateEventDataFromParams(const FYcDamageSummaryParams& Params, AActor* Instigator, AActor* Target)
{
	FYcDamageEventData EventData;
	
	EventData.Instigator = Instigator;
	EventData.Target = Target;
	EventData.FinalDamage = Params.GetFinalDamage();
	EventData.BaseDamage = Params.GetBaseDamage();
	EventData.DamageType = Params.DamageTypeTag;
	EventData.EventTags = Params.TemporaryTags;
	EventData.bWasDodged = Params.bCancelExecution && Params.TemporaryTags.HasTag(YcDamageGameplayTags::Damage_Event_Dodged);
	EventData.bWasImmune = Params.bCancelExecution && Params.TemporaryTags.HasTag(YcDamageGameplayTags::Damage_Event_Immunity);
	EventData.bWasCritical = Params.TemporaryTags.HasTag(YcDamageGameplayTags::Damage_Event_Critical);
	
	// 命中区域可以从 SetByCaller 或 Context 中获取
	// 这里暂时留空，后续扩展

	return EventData;
}

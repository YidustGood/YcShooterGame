// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_Armor.generated.h"

/**
 * 护甲减伤组件
 * 处理护甲吸收伤害的逻辑, 破甲时会向目标ASC组件的avatar发送破甲事件(SendGameplayEventToActor)
 * 
 * 多 AttributeSet 架构：
 * - 每个护甲部位对应一个独立的 UYcArmorSet 实例
 * - 通过 ArmorTag 标识护甲部位（如 Armor.Head、Armor.Body）
 * - HitZone 映射到对应的 ArmorTag 来查找护甲
 * 
 * 执行优先级: 100 (在伤害应用之前)
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Armor Mitigator"))
class YICHENCOMBATCORE_API UYcDamageComponent_Armor : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_Armor();

protected:
	/** 是否启用护甲减伤 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableArmorMitigation = true;

	/** 不受护甲影响的伤害类型标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTagContainer IgnoreArmorDamageTypes;

	/** 是否在护甲耗尽时发送事件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bSendArmorBrokenEvent = true;

	/** 护甲破碎事件标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bSendArmorBrokenEvent"))
	FGameplayTag ArmorBrokenEventTag;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;

protected:
	/** 执行护甲吸收（基于 HitZone 映射到 ArmorTag） */
	void ExecuteArmorAbsorption(FYcDamageSummaryParams& Params, const class UYcArmorSet* ArmorSet) const;

	/** 将 HitZone 转换为 ArmorTag */
	FGameplayTag ConvertHitZoneToArmorTag(const FGameplayTag& HitZone) const;

private:
	/** 获取目标护甲属性集（基于 HitZone 或默认） */
	class UYcArmorSet* GetTargetArmorSet(class UYcAbilitySystemComponent* YcASC, const FGameplayTag& HitZone) const;
};

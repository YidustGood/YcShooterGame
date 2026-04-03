// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_DistanceAttenuation.generated.h"

class IYcAbilitySourceInterface;
class UCurveFloat;

/**
 * 距离衰减组件
 * 根据攻击距离计算伤害衰减
 * 
 * 执行优先级: 30
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Distance Attenuation"))
class YICHENDAMAGE_API UYcDamageComponent_DistanceAttenuation : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_DistanceAttenuation();

protected:
	/** 是否启用距离衰减 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableDistanceAttenuation = true;

	/** 最小衰减距离（此距离内无衰减） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bEnableDistanceAttenuation"))
	float MinAttenuationDistance = 0.0f;

	/** 最大衰减距离（此距离外伤害为0） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bEnableDistanceAttenuation"))
	float MaxAttenuationDistance = 10000.0f;

	/** 衰减曲线（可选） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bEnableDistanceAttenuation"))
	UCurveFloat* AttenuationCurve = nullptr;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;

private:
	/** 计算距离衰减系数 */
	float CalculateAttenuation(float Distance) const;
};

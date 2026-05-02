// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "Executions/YcDamageRuntimePayloads.h"
#include "YcDamageComponent_BackstabMultiplier.generated.h"

/**
 * 背刺伤害倍率组件
 * 从 AbilitySource 读取背刺参数，并基于攻击者与目标朝向关系追加伤害倍率。
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Backstab Multiplier"))
class YICHENDAMAGE_API UYcDamageComponent_BackstabMultiplier : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_BackstabMultiplier();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bProjectToHorizontalPlane = true;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

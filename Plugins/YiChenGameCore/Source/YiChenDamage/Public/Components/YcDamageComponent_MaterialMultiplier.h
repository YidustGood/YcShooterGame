// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_MaterialMultiplier.generated.h"

/**
 * 物理材质伤害倍率组件
 * 根据命中位置的物理材质计算伤害倍率（可增可减）
 * 例如：爆头2.0倍、四肢0.8倍、护甲0.5倍等
 * 
 * 执行优先级: 35
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Material Multiplier"))
class YICHENDAMAGE_API UYcDamageComponent_MaterialMultiplier : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_MaterialMultiplier();

protected:
	/** 是否启用物理材质倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableMaterialMultiplier = true;

	/** 默认倍率值（当没有物理材质时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float DefaultMultiplier = 1.0f;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_Dodge.generated.h"

/**
 * 闪避处理组件
 * 随机判定闪避，取消伤害执行
 * 
 * 执行优先级: 15 (在伤害计算之前)
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Dodge Handler"))
class YICHENDAMAGE_API UYcDamageComponent_Dodge : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_Dodge();

protected:
	/** 是否启用闪避 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableDodge = true;

	/** 闪避率属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayAttribute DodgeRateAttribute;

	/** 默认闪避率（当属性未找到时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float DefaultDodgeRate = 0.0f;

	/** 闪避标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTag DodgeTag;

	/** 是否使用固定随机种子（用于测试） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bUseFixedSeed = false;

	/** 固定随机种子值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bUseFixedSeed"))
	int32 FixedSeed = 0;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;

private:
	/** 随机数生成器 */
	mutable FRandomStream RandomStream;
};

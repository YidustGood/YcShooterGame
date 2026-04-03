// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_Critical.generated.h"

/**
 * 暴击处理组件
 * 随机判定暴击，修改伤害系数
 * 
 * 执行优先级: 50
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Critical Handler"))
class YICHENDAMAGE_API UYcDamageComponent_Critical : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_Critical();

protected:
	/** 是否启用暴击 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableCritical = true;

	/** 暴击率属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayAttribute CriticalRateAttribute;

	/** 暴击伤害倍率属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayAttribute CriticalDamageAttribute;

	/** 默认暴击率（当属性未找到时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float DefaultCriticalRate = 0.0f;

	/** 默认暴击伤害倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float DefaultCriticalDamage = 1.5f;

	/** 暴击标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTag CriticalTag;

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

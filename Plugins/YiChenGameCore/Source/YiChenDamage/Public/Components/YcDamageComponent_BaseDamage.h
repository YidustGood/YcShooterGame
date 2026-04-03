// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_BaseDamage.generated.h"

/**
 * 基础伤害收集组件
 * 从 SetByCaller 或配置值获取基础伤害并设置到 Params
 * 
 * 执行优先级: 10 (最先执行)
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Base Damage Collector"))
class YICHENDAMAGE_API UYcDamageComponent_BaseDamage : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_BaseDamage();

protected:
	/** 是否使用 SetByCaller 覆盖基础伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bUseSetByCaller = false;

	/** SetByCaller 标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bUseSetByCaller"))
	FGameplayTag SetByCallerTag;

	/** 固定基础伤害值（当不使用 SetByCaller 时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "!bUseSetByCaller"))
	float FixedBaseDamage = 0.0f;

	/** 基础伤害倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float BaseDamageMultiplier = 1.0f;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

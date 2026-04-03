// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_Influence.generated.h"

/**
 * 加成收集组件
 * 根据 DamageTypeTag 从 InfluenceSubsystem 收集属性加成
 * 
 * 执行优先级: 25 (在基础伤害收集之后)
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Influence Collector"))
class YICHENDAMAGE_API UYcDamageComponent_Influence : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_Influence();

protected:
	/** 是否启用加成收集 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableInfluenceCollection = true;

	/** 默认伤害类型标签（当 Params 中没有设置时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTag DefaultDamageTypeTag;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

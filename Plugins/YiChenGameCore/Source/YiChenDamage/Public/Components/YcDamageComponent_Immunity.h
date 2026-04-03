// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_Immunity.generated.h"

/**
 * 免疫处理组件
 * 检查目标是否免疫伤害，取消伤害执行
 * 
 * 执行优先级: 10 (最先执行)
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Immunity Handler"))
class YICHENDAMAGE_API UYcDamageComponent_Immunity : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_Immunity();

protected:
	/** 是否启用免疫检查 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableImmunityCheck = true;

	/** 免疫标签（目标拥有此标签时免疫伤害） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTagContainer ImmunityTags;

	/** 免疫事件标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTag ImmunityEventTag;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

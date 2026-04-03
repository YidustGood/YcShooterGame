// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_SetDamageType.generated.h"

/**
 * 伤害类型设置组件
 * 
 * 功能：
 * - 设置伤害类型标签（如果 Context 中没有提供）
 * - 作为默认伤害类型的回退
 * 
 * 执行优先级: 5 (最高优先级，确保在其他组件之前设置)
 * 
 * 使用场景：
 * - 为没有指定伤害类型的 GE 提供默认类型
 * - 武器伤害默认为物理伤害
 * - 技能伤害默认为对应元素类型
 */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Set Damage Type"))
class YICHENDAMAGE_API UYcDamageComponent_SetDamageType : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_SetDamageType();

	// -------------------------------------------------------------------
	// 配置
	// -------------------------------------------------------------------

	/** 默认伤害类型标签（当 Context 中没有设置时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FGameplayTag DefaultDamageTypeTag;

	/** 是否覆盖 Context 中已设置的类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bOverrideExisting = false;

	// -------------------------------------------------------------------
	// UYcDamageExecutionComponent 接口
	// -------------------------------------------------------------------

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "YcAttributeExecutionComponent.h"
#include "YcAttributeExecution.generated.h"

/**
 * 通用属性执行计算基类
 * 基于 Component 架构的属性修改执行类
 * 
 * 设计思想：
 * - 作为协调者，定义执行顺序，不包含具体逻辑
 * - 通过 Components 数组配置计算流程
 * - 支持蓝图配置和 C++ 扩展
 * 
 * 使用方式：
 * 1. 创建 GE，选择此 Execution
 * 2. 在 Components 数组中添加所需的组件
 * 3. 配置每个组件的参数
 */
UCLASS(Abstract, Blueprintable)
class YICHENABILITY_API UYcAttributeExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UYcAttributeExecution();

	//~ UObject 接口
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	//~ UObject 接口结束

protected:
	// -------------------------------------------------------------------
	// 组件配置
	// -------------------------------------------------------------------

	/** 是否按 Priority 排序组件（默认 true）。关闭时按编辑器配置顺序执行 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	bool bSortByPriority = true;

	/** 执行组件列表（按 Priority 排序执行，或按编辑器配置顺序执行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Components")
	mutable TArray<TObjectPtr<UYcAttributeExecutionComponent>> Components;

	/** 组件是否已排序（缓存标志） */
	UPROPERTY(Transient)
	mutable bool bComponentsSorted = false;

	// -------------------------------------------------------------------
	// 调试配置
	// -------------------------------------------------------------------

	/** 是否启用调试日志 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableDebugLog = false;

	/** 是否在 PIE 中自动启用调试 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bAutoEnableDebugInPIE = true;

	// -------------------------------------------------------------------
	// ExecutionCalculation 接口
	// -------------------------------------------------------------------

	/**
	 * 执行计算
	 * 1. 初始化 Params
	 * 2. 按 Priority 遍历 Components
	 * 3. 计算最终值
	 * 4. 输出到 OutExecutionOutput
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	// -------------------------------------------------------------------
	// 内部函数（供子类重写）
	// -------------------------------------------------------------------

	/**
	 * 初始化参数
	 * 子类可重写以添加特定领域的初始化逻辑
	 */
	virtual void InitializeParams(FYcAttributeSummaryParams& Params, const FGameplayEffectCustomExecutionParameters& ExecutionParams) const;

	/**
	 * 获取标签容器
	 */
	void GetTagContainers(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayTagContainer& OutSourceTags, FGameplayTagContainer& OutTargetTags) const;

	/**
	 * 排序组件（按 Priority），带缓存优化
	 */
	void SortComponents() const;

	/**
	 * 标记组件需要重新排序
	 */
	void MarkComponentsDirty() const { bComponentsSorted = false; }

	/**
	 * 从参数中获取 World
	 * 依次尝试：TargetASC -> SourceASC -> GetWorld()
	 * @param Params 属性计算参数
	 * @return World 指针，可能为 nullptr
	 */
	UWorld* GetWorldFromParams(const FYcAttributeSummaryParams& Params) const;

	/**
	 * 输出调试信息
	 */
	virtual void LogDebugInfo(const FYcAttributeSummaryParams& Params) const;
};

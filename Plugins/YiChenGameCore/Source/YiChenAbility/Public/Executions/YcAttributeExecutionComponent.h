// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "YcAttributeSummaryParams.h"
#include "YcAttributeExecutionComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEffectCustomExecutionParameters;
struct FGameplayEffectCustomExecutionOutput;

/**
 * 属性执行组件基类
 * 所有属性计算组件的抽象基类，提供统一的接口和过滤机制
 * 
 * 设计思想：
 * - 单一职责：每个 Component 只负责一个计算环节
 * - Tag 过滤：通过 SourceTags/TargetTags 控制组件是否执行
 * - 蓝图友好：使用 BlueprintNativeEvent 支持蓝图重写
 * 
 * 使用方式：
 * 1. 继承此类创建自定义组件
 * 2. 重写 Execute_Implementation 实现具体逻辑
 * 3. 使用 SourceTags/TargetTags 设置过滤条件
 * 4. 使用 Priority 控制执行顺序
 */
UCLASS(BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew, Abstract)
class YICHENABILITY_API UYcAttributeExecutionComponent : public UObject
{
	GENERATED_BODY()

public:
	UYcAttributeExecutionComponent();

	// -------------------------------------------------------------------
	// 过滤机制
	// -------------------------------------------------------------------

	/** 源标签过滤条件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FGameplayTagRequirements SourceTags;

	/** 目标标签过滤条件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FGameplayTagRequirements TargetTags;

	/** 类型标签过滤（匹配任意一个即执行，子类可定义具体含义） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FGameplayTagContainer TypeTags;

	// -------------------------------------------------------------------
	// 配置项
	// -------------------------------------------------------------------

	/** 执行优先级（数值越小越先执行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 Priority = 100;

	/** 是否启用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnabled = true;

	/** 是否启用调试日志（默认关闭） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableDebugLog = false;

	/** 调试名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FString DebugName;
	
#if WITH_EDITORONLY_DATA
	/** 编辑器中的配置描述，便于开发阶段标注 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FText DevDescription;
#endif

	// -------------------------------------------------------------------
	// 执行接口
	// -------------------------------------------------------------------

	/**
	 * 检查组件是否应该执行
	 * 根据 Tag 过滤条件判断
	 */
	UFUNCTION(BlueprintCallable, Category = "YcAttribute")
	virtual bool ShouldExecute(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const;

	/**
	 * 蓝图可重写的执行条件检查
	 */
	UFUNCTION(BlueprintNativeEvent, DisplayName = "ShouldComponentExecute", Category = "YcAttribute")
	bool K2_ShouldExecute(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const;
	virtual bool K2_ShouldExecute_Implementation(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const;

	/**
	 * 执行计算逻辑
	 * 子类必须重写此函数实现具体逻辑
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "YcAttribute")
	void Execute(FYcAttributeSummaryParams& Params);
	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params);

	// -------------------------------------------------------------------
	// 辅助函数
	// -------------------------------------------------------------------

	/**
	 * 检查源标签是否匹配（热路径，内联）
	 */
	FORCEINLINE bool IsSourceTagsMatched(const FGameplayTagContainer& Tags) const
	{
		return SourceTags.RequirementsMet(Tags);
	}

	/**
	 * 检查目标标签是否匹配（热路径，内联）
	 */
	FORCEINLINE bool IsTargetTagsMatched(const FGameplayTagContainer& Tags) const
	{
		return TargetTags.RequirementsMet(Tags);
	}

	/**
	 * 检查类型标签是否匹配（热路径，内联）
	 */
	FORCEINLINE bool IsTypeTagsMatched(const FGameplayTag& InTypeTag) const
	{
		if (TypeTags.IsEmpty())
		{
			return true;
		}
		return TypeTags.HasTag(InTypeTag);
	}

	// -------------------------------------------------------------------
	// UObject 接口
	// -------------------------------------------------------------------

	virtual UWorld* GetWorld() const override;

#if WITH_EDITOR
	virtual FText GetDefaultNodeTitle() const;
#endif

protected:
	// -------------------------------------------------------------------
	// 内部辅助函数（供子类使用）
	// -------------------------------------------------------------------

	/**
	 * 获取源属性值
	 * @param Params 参数结构体
	 * @param Attribute 要获取的属性
	 * @param bFound 是否找到属性（输出参数）
	 * @return 属性值
	 */
	float GetSourceAttribute(const FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, bool* bFound = nullptr) const;

	/**
	 * 获取目标属性值
	 * @param Params 参数结构体
	 * @param Attribute 要获取的属性
	 * @param bFound 是否找到属性（输出参数）
	 * @return 属性值
	 */
	float GetTargetAttribute(const FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, bool* bFound = nullptr) const;

	/**
	 * 应用属性修改
	 * @param Params 参数结构体
	 * @param Attribute 要修改的属性
	 * @param Value 修改值
	 * @param Op 修改操作类型
	 */
	void ApplyModToAttribute(FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, float Value, EGameplayModOp::Type Op = EGameplayModOp::Additive) const;

	/**
	 * 添加输出修改器
	 * @param Params 参数结构体
	 * @param Attribute 要修改的属性
	 * @param Value 修改值
	 * @param Op 修改操作类型
	 */
	void AddOutputModifier(FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, float Value, EGameplayModOp::Type Op = EGameplayModOp::Additive) const;

	/**
	 * 获取 SetByCaller 值
	 * @param Params 参数结构体
	 * @param Tag SetByCaller 标签
	 * @param bFound 是否找到（输出参数）
	 * @return SetByCaller 值
	 */
	float GetSetByCallerValue(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag, bool* bFound = nullptr) const;

	/**
	 * 记录调试日志
	 */
	void LogDebug(const FString& Message) const;
};

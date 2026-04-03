// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Resistance/YcResistanceContainer.h"
#include "YcResistanceComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 抗性管理组件
 * 
 * 功能：
 * - 管理实体的所有伤害类型抗性
 * - 支持网络复制
 * - 提供蓝图友好的接口
 * 
 * 使用方式：
 * 1. 将组件添加到 Character 或 PlayerState
 * 2. 通过 SetResistance 设置抗性值
 * 3. 伤害系统通过 GetResistance 查询抗性
 * 
 * 示例：
 *   // 设置火焰抗性 30%
 *   ResistanceComponent->SetResistance(FGameplayTag::RequestGameplayTag("Resistance.Fire"), 0.3f);
 *   
 *   // 获取抗性值
 *   float FireRes = ResistanceComponent->GetResistance(FGameplayTag::RequestGameplayTag("Resistance.Fire"));
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class YICHENDAMAGE_API UYcResistanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcResistanceComponent();

	// -------------------------------------------------------------------
	// 抗性操作接口
	// -------------------------------------------------------------------

	/**
	 * 设置指定标签的抗性值
	 * @param Tag 抗性标签（如 Resistance.Fire）
	 * @param Value 抗性值（0.0 ~ 1.0）
	 */
	UFUNCTION(BlueprintCallable, Category = "Resistance")
	void SetResistance(FGameplayTag Tag, float Value);

	/**
	 * 增加指定标签的抗性值
	 * @param Tag 抗性标签
	 * @param Delta 增量值（可为负数）
	 */
	UFUNCTION(BlueprintCallable, Category = "Resistance")
	void AddResistance(FGameplayTag Tag, float Delta);

	/**
	 * 移除指定标签的抗性
	 * @param Tag 抗性标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Resistance")
	void RemoveResistance(FGameplayTag Tag);

	/**
	 * 获取指定标签的抗性值
	 * @param Tag 抗性标签
	 * @return 抗性值，不存在返回 0.0
	 */
	UFUNCTION(BlueprintPure, Category = "Resistance")
	float GetResistance(FGameplayTag Tag) const;

	/**
	 * 检查是否存在指定标签的抗性
	 */
	UFUNCTION(BlueprintPure, Category = "Resistance")
	bool HasResistance(FGameplayTag Tag) const;

	/**
	 * 获取所有抗性标签
	 */
	UFUNCTION(BlueprintPure, Category = "Resistance")
	TArray<FGameplayTag> GetAllResistanceTags() const;

	/**
	 * 清空所有抗性
	 */
	UFUNCTION(BlueprintCallable, Category = "Resistance")
	void ClearAllResistances();

	// -------------------------------------------------------------------
	// 直接访问容器（C++ 高效访问）
	// -------------------------------------------------------------------

	/** 获取抗性容器（只读） */
	const FYcResistanceContainer& GetResistanceContainer() const { return ResistanceContainer; }

	/** 获取抗性容器（可修改） */
	FYcResistanceContainer& GetMutableResistanceContainer() { return ResistanceContainer; }

	// -------------------------------------------------------------------
	// 配置
	// -------------------------------------------------------------------

	/** 最大抗性值（默认 0.8，避免完全免疫） */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MaxResistance = 0.8f;

	/** 最小抗性值（默认 0.0） */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	float MinResistance = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 抗性容器 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Resistance", meta = (AllowPrivateAccess = "true"))
	FYcResistanceContainer ResistanceContainer;
};

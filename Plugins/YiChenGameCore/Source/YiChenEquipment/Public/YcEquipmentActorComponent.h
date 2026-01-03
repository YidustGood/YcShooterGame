// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "YcEquipmentActorComponent.generated.h"

class UYcEquipmentInstance;

/** 当有效的装备对象通过网络同步到客户端后的委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipmentInstRep, UYcEquipmentInstance*, EquipmentInst);

/**
 * 装备Actor组件
 * 所有由装备实例生成的Actor都需要添加这个组件，用于：
 * - 反向查询所属的装备实例
 * - 存储装备配置的 Tags（解决 AActor::Tags 不复制的问题）
 * 
 * 网络同步：
 * - 组件本身通过 SetIsReplicated(true) 复制到客户端
 * - EquipmentTags 使用 COND_InitialOnly 只在初始化时同步一次
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class YICHENEQUIPMENT_API UYcEquipmentActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcEquipmentActorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 静态辅助函数：从任意Actor获取其所属的装备实例
	 * @param Actor 要查询的Actor
	 * @return 所属的装备实例，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment", meta = (DefaultToSelf = "Actor"))
	static UYcEquipmentInstance* GetEquipmentFromActor(const AActor* Actor);

	/** 获取所属的装备实例 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UYcEquipmentInstance* GetOwningEquipment() const { return EquipmentInst; }
	
	/** 检查是否包含指定 Tag */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasEquipmentTag(FName Tag) const { return EquipmentTags.Contains(Tag); }
	
	/** 获取所有装备 Tags */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	const TArray<FName>& GetEquipmentTags() const { return EquipmentTags; }
	
	/** 当有效的装备对象通过网络同步到客户端后的委托 */
	UPROPERTY(BlueprintAssignable)
	FEquipmentInstRep OnEquipmentRep;
	
protected:
	UFUNCTION()
	void OnRep_EquipmentInst();

private:
	friend class UYcEquipmentInstance;
	
	/** 
	 * 所属的装备实例
	 * 使用 COND_InitialOnly 只在初始化时同步一次
	 * 特别说明: 实际上通过FYcEquipmentList::PostReplicatedAdd里设置也可以, 这样就无需额外的网络复制, 但是会增加代码复杂度综合权衡下来没有必要
	 */
	UPROPERTY(ReplicatedUsing=OnRep_EquipmentInst, VisibleInstanceOnly)
	TObjectPtr<UYcEquipmentInstance> EquipmentInst;
	
	/** 
	 * 装备配置的 Tags
	 * 由于 AActor::Tags 不会网络复制，这里单独存储并复制
	 * 使用 COND_InitialOnly 只在初始化时同步一次（Tags 不会动态修改）
	 */
	UPROPERTY(Replicated, VisibleInstanceOnly)
	TArray<FName> EquipmentTags;
};
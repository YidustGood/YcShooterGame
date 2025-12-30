// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "YcEquipmentActorComponent.generated.h"

class UYcEquipmentInstance;

/**
 * 装备Actor组件
 * 附加到由装备实例生成的Actor上，用于反向查询所属的装备实例
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class YICHENEQUIPMENT_API UYcEquipmentActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcEquipmentActorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 静态辅助函数：从任意Actor获取其所属的装备实例
	 * @param Actor 要查询的Actor
	 * @return 所属的装备实例，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment", meta = (DefaultToSelf = "Actor"))
	static UYcEquipmentInstance* GetEquipmentFromActor(AActor* Actor);

	/** 获取所属的装备实例 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UYcEquipmentInstance* GetOwningEquipment() const { return OwningEquipment.Get(); }

private:
	friend class UYcEquipmentInstance;
	
	/** 所属的装备实例（弱引用，避免循环引用） */
	UPROPERTY()
	TWeakObjectPtr<UYcEquipmentInstance> OwningEquipment;
};
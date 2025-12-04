// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcEquipmentDefinition.h"
#include "YcReplicableObject.h"
#include "YcEquipmentInstance.generated.h"

class UYcInventoryItemInstance;

/**
 * 装备实例基类
 * 提供了生成/销毁Actor附加/卸下到玩家角色模型身上的能力,该对象持有并管理需要生成到玩家角色身上的Actors
 */
UCLASS()
class YICHENEQUIPMENT_API UYcEquipmentInstance : public UYcReplicableObject
{
	GENERATED_BODY()
public:
	UYcEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; };
	
protected:
	UFUNCTION()
	void OnRep_Instigator(const UYcInventoryItemInstance* LastInstigator);
	
private:
	// 该装备对应的库存物品实例对象(UYcInventoryItemInstance)
	UPROPERTY(ReplicatedUsing=OnRep_Instigator, VisibleAnywhere)
	TObjectPtr<UYcInventoryItemInstance> Instigator;
	
	// 该装备对象已经生成的Actors对象列表
	UPROPERTY(Replicated, VisibleInstanceOnly)
	TArray<TObjectPtr<AActor>> SpawnedActors;
	
	// 该装备生成的仅存于本地的Actors(例如多人联机射击游戏中的第一人称武器Actor)
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AActor>> OwnerClientSpawnedActors;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FYcEquipmentDefinition EquipmentDef;
	
public:
	FORCEINLINE const FYcEquipmentDefinition& GetEquipmentDef() const { return EquipmentDef; }
};
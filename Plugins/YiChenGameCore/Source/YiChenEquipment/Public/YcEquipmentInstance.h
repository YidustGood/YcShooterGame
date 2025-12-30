// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcEquipmentDefinition.h"
#include "YcReplicableObject.h"
#include "YcEquipmentInstance.generated.h"

class UYcInventoryItemInstance;
class UYcEquipmentManagerComponent;

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
	
	// 获取装备实例对应的的库存物品实例对象
	UFUNCTION(BlueprintPure, Category=Equipment)
	UYcInventoryItemInstance* GetAssociatedItem() const;
	
	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;
	
	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;
	
	UFUNCTION(BlueprintPure, Category=Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }
	
	/** 生成装备所需的Actors, 分为开启网络复制和未开启两种, 会有不同的生成策略 */
	UFUNCTION(BlueprintCallable)
	virtual void SpawnEquipmentActors(const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn);

	/** 销毁这个装备所生成的Actors */
	UFUNCTION(BlueprintCallable)
	virtual void DestroyEquipmentActors();
	
	/**
	 * 根据Tag查找生成的Actor对象
	 * @param Tag 要查找的Tag
	 * @param bReplicateActor 是否查找为网络复制的Actor
	 * @return 查找倒的Actor
	 */
	UFUNCTION(BlueprintCallable)
	AActor* FindSpawnedActorByTag(FName Tag, bool bReplicateActor);
	
	/** 蓝图版本获取装备定义, 内部实际上是通过装备所属的ItemInst获取到FInventoryFragment_Equippable->EquipmentDef */
	UFUNCTION(BlueprintCallable, DisplayName="GetEquipmentDef")
	bool K2_GetEquipmentDef(FYcEquipmentDefinition& OutEquipmentDef);
	
	/** 客户端和服务器同时触发：可以在这里处理武器实例装备和卸载装备后所需要做的事情 ，例如FPS游戏装备只存在于本地控制端的第一人称武器模型*/
	virtual void OnEquipped();
	virtual void OnUnequipped();
	
	/**
	 * 查询装备定义中的 Fragment
	 * 只能在 C++ 中使用，效率最高
	 * 
	 * @return 找到的 Fragment 指针，如果不存在则返回 nullptr
	 * 
	 * 使用示例：
	 * const FEquipmentFragment_WeaponStats* WeaponStats = 
	 *     Equipment->GetTypedFragment<FEquipmentFragment_WeaponStats>();
	 */
	template <typename FragmentType>
	const FragmentType* GetTypedFragment()
	{
		if (!GetEquipmentDef()) return nullptr;
		return EquipmentDef->GetTypedFragment<FragmentType>();
	}
	
	/**
	 * 蓝图版本：查询装备定义中的 Fragment
	 * 从装备定义的 Fragments 数组中查找指定类型的 Fragment
	 * 
	 * @param FragmentStructType 目标 Fragment 结构类型（必须是 FYcEquipmentFragment 或其子类）
	 * @return 查找到的 Fragment 结构体数据，如果不存在则返回空结构体
	 * 
	 * 注意：此函数会产生一次 Fragment 的深拷贝。如果频繁调用，建议缓存结果。
	 * 
	 * 使用示例（蓝图）：
	 * 1. 调用 Find Equipment Fragment
	 * 2. 输入 Fragment Struct Type: FEquipmentFragment_WeaponStats
	 * 3. 获取返回的 Fragment 数据
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Equipment")
	TInstancedStruct<FYcEquipmentFragment> FindEquipmentFragment(const UScriptStruct* FragmentStructType);
	
protected:
	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnUnequipped"))
	void K2_OnUnequipped();

	UFUNCTION(Client, reliable)
	void DestroyEquipmentActorsOnOwnerClient();
	
private:
	friend struct FYcEquipmentList;
	
	// 该装备对应的库存物品实例对象(UYcInventoryItemInstance)
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UYcInventoryItemInstance> Instigator;
	
	// 该装备对象已经生成的Actors对象列表
	UPROPERTY(Replicated, VisibleInstanceOnly)
	TArray<TObjectPtr<AActor>> SpawnedActors;
	
	// 该装备生成的仅存于本地的Actors(例如多人联机射击游戏中的第一人称武器Actor)
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AActor>> OwnerClientSpawnedActors;
	
	/** 装备定义指针, 在SetEquipmentDefRowHandle函数调用时自动通过EquipmentDefRowHandle进行设置, 该指针指向的就是DataTable中的EquipmentDef */
	const FYcEquipmentDefinition* EquipmentDef;
	
private:
	AActor* SpawnEquipActorInternal(const TSubclassOf<AActor>& ActorToSpawnClass,const FYcEquipmentActorToSpawn& SpawnInfo, USceneComponent* AttachTarget) const;
	
	/** 
	 * 设置装备实例对应的库存物品实例对象 
	 * 服务端在FYcEquipmentList::AddEntry()调用
	 * 客户端通过FYcEquipmentList中FastArray的3个辅助函数中调用
	 * 如此一来Instigator便无须开启网络复制了
	 */
	void SetInstigator(UYcInventoryItemInstance* InInstigator);
	
	/** 更新EquipmentDef, 在SetInstigator内调用, 从Instigator获取EquipmentDef并缓存指针 */
	void UpdateEquipmentDef();
	
public:
	/** 获取装备定义, 内部实际上是通过装备所属的ItemInst获取到FInventoryFragment_Equippable->EquipmentDef */
	const FYcEquipmentDefinition* GetEquipmentDef();
};
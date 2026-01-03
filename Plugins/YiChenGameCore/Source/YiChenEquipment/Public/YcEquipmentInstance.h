// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcEquipmentDefinition.h"
#include "YcReplicableObject.h"
#include "YcEquipmentInstance.generated.h"

class UYcInventoryItemInstance;
class UYcEquipmentManagerComponent;

/**
 * 装备实例基类
 * 
 * 提供了生成/销毁/隐藏/显示Actor附加到玩家角色模型身上的能力。
 * 该对象持有并管理需要生成到玩家角色身上的Actors（如武器模型、护甲模型等）。
 * 
 * 主要功能：
 * - Actor生命周期管理：生成、销毁、隐藏、显示
 * - Actor复用优化：支持卸下装备时保留Actor，再次装备时复用
 * - 网络同步：支持服务器复制的Actor和仅本地存在的Actor
 * - 休眠优化：卸下时可进入休眠状态降低性能消耗
 * 
 * 网络架构：
 * - SpawnedActors: 服务器生成并复制到客户端的Actor列表
 * - OwnerClientSpawnedActors: 仅在控制客户端本地生成的Actor列表（如第一人称武器）
 * 
 * @see UYcEquipmentManagerComponent 装备管理组件
 * @see FYcEquipmentDefinition 装备定义
 * @see FYcEquipmentActorToSpawn Actor生成配置
 */
UCLASS(BlueprintType)
class YICHENEQUIPMENT_API UYcEquipmentInstance : public UYcReplicableObject
{
	GENERATED_BODY()
	
public:
	UYcEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~ Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~ End UObject Interface

	//~=============================================================================
	// 基础查询接口
	
	/**
	 * 获取装备实例对应的库存物品实例对象
	 * @return 关联的库存物品实例，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UYcInventoryItemInstance* GetAssociatedItem() const;
	
	/**
	 * 获取装备所属的Pawn
	 * 会自动处理EquipmentManagerComponent挂载到Pawn/PlayerState/PlayerController的情况
	 * @return 所属的Pawn，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	APawn* GetPawn() const;
	
	/**
	 * 获取指定类型的Pawn
	 * @param PawnType 期望的Pawn类型
	 * @return 如果Outer是指定类型则返回，否则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment", meta = (DeterminesOutputType = PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;
	
	/**
	 * 获取已生成的服务器复制Actor列表
	 * @return 已生成的Actor数组（网络复制的）
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }
	
	/**
	 * 获取装备定义
	 * 内部通过装备所属的ItemInst获取到FInventoryFragment_Equippable->EquipmentDef
	 * @return 装备定义指针，如果不存在则返回nullptr
	 */
	const FYcEquipmentDefinition* GetEquipmentDef();
	
	/**
	 * 蓝图版本：获取装备定义
	 * @param OutEquipmentDef 输出的装备定义副本
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment", DisplayName = "GetEquipmentDef")
	bool K2_GetEquipmentDef(FYcEquipmentDefinition& OutEquipmentDef);

	//~=============================================================================
	// Actor生命周期管理
	
	/**
	 * 生成装备所需的Actors
	 * 
	 * 根据配置分为两种生成策略：
	 * - bReplicateActor=true: 在服务器生成，自动复制到客户端
	 * - bReplicateActor=false: 仅在控制客户端本地生成
	 * 
	 * 如果已经生成过Actors（bActorsSpawned=true），会自动调用Show逻辑复用已有Actor
	 * 
	 * @param ActorsToSpawn 要生成的Actor配置列表
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void SpawnEquipmentActors(const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn);

	/**
	 * 销毁装备所生成的所有Actors
	 * 会同时销毁服务器复制的Actor和本地Actor，并重置bActorsSpawned标记
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void DestroyEquipmentActors();
	
	/**
	 * 隐藏装备生成的Actors（不销毁，用于装备切换时的复用）
	 * 
	 * 根据FYcEquipmentActorToSpawn中的bDormantOnUnequip配置决定是否进入休眠状态：
	 * - 休眠状态会禁用Tick、碰撞，并设置网络休眠（仅复制Actor）
	 * - 保持附加状态，显示时无需重新Attach
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void HideEquipmentActors();
	
	/**
	 * 显示之前隐藏的装备Actors
	 * 如果之前进入了休眠状态，会自动唤醒（恢复Tick、碰撞、网络同步）
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void ShowEquipmentActors();
	
	/**
	 * 仅显示本地客户端的装备Actors（不通过RPC）
	 * 用于客户端在收到网络同步后直接恢复本地Actors
	 * @note 此函数供内部使用，通常不需要手动调用
	 */
	void ShowLocalEquipmentActors();
	
	/**
	 * 仅隐藏本地客户端的装备Actors（不通过RPC）
	 * 用于客户端在 PreReplicatedRemove 中隐藏本地Actors
	 * @note 此函数供内部使用，通常不需要手动调用
	 */
	void HideLocalEquipmentActors();
	
	/**
	 * 检查是否已经生成过装备Actors
	 * @return true表示已生成过（可能处于隐藏状态），false表示从未生成
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasSpawnedActors() const { return bActorsSpawned; }
	
	/**
	 * 根据Tag查找生成的Actor对象
	 * @param Tag 要查找的ActorTag
	 * @param bReplicateActor true查找服务器复制的Actor，false查找本地Actor
	 * @return 找到的Actor，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	AActor* FindSpawnedActorByTag(FName Tag, bool bReplicateActor);
	
	/**
	 * 获取装备状态标记
	 * @return true 表示当前处于装备状态，false 表示已卸下
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsEquipped() const;

	//~=============================================================================
	// 装备/卸载回调
	
	/**
	 * 装备时的回调（服务器和客户端都会触发）
	 * 
	 * 内部会自动处理：
	 * - 显示装备附加的 Actors（调用 ShowEquipmentActors）
	 * - 对于本地控制的 Pawn，还会显示本地 Actors（调用 ShowLocalEquipmentActors）
	 * - 调用蓝图实现的 K2_OnEquipped 事件
	 * 
	 * 使用 bEquipped 标记避免重复调用
	 * 
	 * 可在子类中重写以扩展装备后的逻辑，如播放装备动画
	 */
	virtual void OnEquipped();
	
	/**
	 * 卸载时的回调（服务器和客户端都会触发）
	 * 
	 * 内部会自动处理：
	 * - 调用蓝图实现的 K2_OnUnequipped 事件
	 * 
	 * 注意：隐藏装备 Actors 的逻辑需要在蓝图中调用 HideEquipmentActors，
	 * 因为可能需要先播放卸下装备的动画，动画结束后再隐藏
	 * 
	 * 使用 bEquipped 标记避免重复调用
	 * 
	 * 可在子类中重写以扩展卸载后的逻辑
	 */
	virtual void OnUnequipped();

	//~=============================================================================
	// Fragment查询接口
	
	/**
	 * C++版本：查询装备定义中的Fragment
	 * 
	 * @tparam FragmentType 目标Fragment类型
	 * @return 找到的Fragment指针，如果不存在则返回nullptr
	 * 
	 * @code
	 * const FEquipmentFragment_WeaponStats* WeaponStats = 
	 *     Equipment->GetTypedFragment<FEquipmentFragment_WeaponStats>();
	 * if (WeaponStats)
	 * {
	 *     float Damage = WeaponStats->BaseDamage;
	 * }
	 * @endcode
	 */
	template <typename FragmentType>
	const FragmentType* GetTypedFragment()
	{
		if (!GetEquipmentDef()) return nullptr;
		return EquipmentDef->GetTypedFragment<FragmentType>();
	}
	
	/**
	 * 蓝图版本：查询装备定义中的Fragment
	 * 
	 * @param FragmentStructType 目标Fragment结构类型（必须是FYcEquipmentFragment或其子类）
	 * @return 查找到的Fragment结构体数据，如果不存在则返回空结构体
	 * 
	 * @note 此函数会产生一次Fragment的深拷贝，如果频繁调用建议缓存结果
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Equipment")
	TInstancedStruct<FYcEquipmentFragment> FindEquipmentFragment(const UScriptStruct* FragmentStructType);
	
protected:
	//~=============================================================================
	// 蓝图可实现事件
	
	/** 蓝图版本：装备时的回调 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment", meta = (DisplayName = "OnEquipped"))
	void K2_OnEquipped();

	/** 蓝图版本：卸载时的回调 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment", meta = (DisplayName = "OnUnequipped"))
	void K2_OnUnequipped();

	//~=============================================================================
	// 客户端RPC
	
	/** 通知控制客户端销毁本地Actors */
	UFUNCTION(Client, Reliable)
	void DestroyEquipmentActorsOnOwnerClient();
	
	/** 通知控制客户端隐藏非网络复制的本地Actors */
	UFUNCTION(Client, Reliable)
	void HideEquipmentActorsOnOwnerClient();
	
	/** 通知控制客户端显示本地Actors */
	UFUNCTION(Client, Reliable)
	void ShowEquipmentActorsOnOwnerClient();
	
private:
	friend struct FYcEquipmentList;
	
	//~=============================================================================
	// 休眠管理（内部使用）
	
	/**
	 * 让单个Actor进入休眠/低消耗状态
	 * @param Actor 目标Actor
	 * @param bReplicated 是否为网络复制的Actor（影响是否设置网络休眠）
	 */
	void SetActorDormant(AActor* Actor, bool bReplicated);
	
	/**
	 * 让单个Actor从休眠状态唤醒
	 * @param Actor 目标Actor
	 * @param bReplicated 是否为网络复制的Actor（影响是否唤醒网络）
	 */
	void WakeActorFromDormant(AActor* Actor, bool bReplicated);
	
	//~=============================================================================
	// 内部数据
	
	/** 该装备对应的库存物品实例对象 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcInventoryItemInstance> Instigator;
	
	/** 
	 * 服务器生成并复制到客户端的Actor列表
	 * 对应FYcEquipmentActorToSpawn中bReplicateActor=true的配置
	 */
	UPROPERTY(Replicated, VisibleInstanceOnly)
	TArray<TObjectPtr<AActor>> SpawnedActors;
	
	/** 
	 * 仅在控制客户端本地生成的Actor列表
	 * 对应FYcEquipmentActorToSpawn中bReplicateActor=false的配置
	 * 典型用例：FPS游戏中的第一人称武器模型
	 */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AActor>> OwnerClientSpawnedActors;
	
	/** 
	 * 装备定义指针缓存
	 * 在SetInstigator时通过ItemInst->FInventoryFragment_Equippable获取并缓存
	 * 指向DataTable中的原始数据，无需手动释放
	 */
	const FYcEquipmentDefinition* EquipmentDef;
	
	/** 
	 * 标记是否已经生成过装备Actors
	 * 用于SpawnEquipmentActors中判断是否需要复用已有Actor
	 */
	uint8 bActorsSpawned : 1;
	
	/** 
	 * 标记当前是否处于装备状态
	 * 用于 OnEquipped/OnUnequipped 中避免重复调用
	 * true = 已装备，false = 已卸下
	 */
	uint8 bEquipped : 1;
	
	//~=============================================================================
	// 内部函数
	
	/**
	 * 内部函数：生成单个装备Actor
	 * @param ActorToSpawnClass 要生成的Actor类
	 * @param SpawnInfo 生成配置信息
	 * @param AttachTarget 附加目标组件
	 * @return 生成的Actor，失败返回nullptr
	 */
	AActor* SpawnEquipActorInternal(const TSubclassOf<AActor>& ActorToSpawnClass, const FYcEquipmentActorToSpawn& SpawnInfo, USceneComponent* AttachTarget);
	
	/**
	 * 设置装备实例对应的库存物品实例对象
	 * 
	 * 调用时机：
	 * - 服务端：在FYcEquipmentList::AddEntry()中调用
	 * - 客户端：通过FYcEquipmentList的FastArray回调函数调用
	 * 
	 * 这样设计使得Instigator无需开启网络复制
	 * 
	 * @param InInstigator 库存物品实例
	 */
	void SetInstigator(UYcInventoryItemInstance* InInstigator);
	
	/**
	 * 更新EquipmentDef缓存
	 * 在SetInstigator内调用，从Instigator获取EquipmentDef并缓存指针
	 */
	void UpdateEquipmentDef();
	
	/**
	 * 设置 Actor 的视觉可见性（不使用 SetActorHiddenInGame）
	 * 
	 * 原因：AActor::bHidden 会通过网络复制，服务器设置后会同步到客户端，
	 * 这会覆盖客户端的预测状态导致闪烁。
	 * 
	 * 解决方案：使用组件的 SetVisibility，这个不会自动复制。
	 */
	void SetActorVisualVisibility(const AActor* Actor, const bool bVisible);
};
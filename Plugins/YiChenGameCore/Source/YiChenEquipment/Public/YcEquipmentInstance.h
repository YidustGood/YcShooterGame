// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcEquipmentDefinition.h"
#include "YcReplicableObject.h"
#include "YcEquipmentInstance.generated.h"

class UYcInventoryItemInstance;
class UYcEquipmentManagerComponent;

/**
 * 装备状态枚举
 * 用于标识装备实例的当前状态
 */
UENUM(BlueprintType)
enum class EYcEquipmentState : uint8
{
	/** 未装备状态：装备实例已创建，Actor已生成但隐藏，GA未授予 */
	Unequipped,
	
	/** 已装备状态：装备正在使用中，Actor可见，GA已授予 */
	Equipped,
};

/** 
 * 装备事件委托
 * 用于通知外部系统装备状态变化，支持蓝图绑定
 * @param EquipmentInst 触发事件的装备实例
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipmentDelegate, UYcEquipmentInstance*, EquipmentInst);

/**
 * ============================================================================
 * 装备实例基类 (Equipment Instance)
 * ============================================================================
 * 
 * 提供了生成/销毁/隐藏/显示Actor附加到玩家角色模型身上的能力。
 * 该对象持有并管理需要生成到玩家角色身上的Actors（如武器模型、护甲模型等）。
 * 
 * ============================================================================
 * 核心设计：状态驱动
 * ============================================================================
 * 
 * 装备实例采用状态驱动设计，通过 EquipmentState 属性控制装备的装备/卸下：
 * 
 * - Unequipped（未装备）：装备已创建，Actor已生成但隐藏，GA未授予
 * - Equipped（已装备）：装备正在使用，Actor可见，GA已授予
 * 
 * 状态切换通过网络复制的 EquipmentState 属性实现，客户端通过 OnRep 回调
 * 自动处理状态变化，支持断线重连后正确恢复装备状态。
 * 
 * ============================================================================
 * 网络架构
 * ============================================================================
 * 
 * - EquipmentState: 服务器权威，通过 ReplicatedUsing 同步到客户端
 * - SpawnedActors: 服务器生成并复制到客户端的Actor列表
 * - OwnerClientSpawnedActors: 仅在控制客户端本地生成的Actor列表（如第一人称武器）
 * 
 * ============================================================================
 * 生命周期
 * ============================================================================
 * 
 * 1. 创建阶段（物品加入QuickBar时）：
 *    - 服务器创建 EquipmentInstance
 *    - 生成 Actors（隐藏状态）
 *    - State = Unequipped
 *    - 通过 EquipmentList 复制到客户端
 * 
 * 2. 装备阶段：
 *    - 服务器设置 State = Equipped
 *    - 授予 GA
 *    - 显示 Actors
 *    - 客户端通过 OnRep 同步状态
 * 
 * 3. 卸下阶段：
 *    - 服务器设置 State = Unequipped
 *    - 移除 GA
 *    - 隐藏 Actors
 *    - 客户端通过 OnRep 同步状态
 * 
 * 4. 销毁阶段（物品从QuickBar移除时）：
 *    - 销毁 Actors
 *    - 从 EquipmentList 移除
 *    - 销毁 EquipmentInstance
 * 
 * @see UYcEquipmentManagerComponent 装备管理组件
 * @see FYcEquipmentDefinition 装备定义
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

	// ========================================================================
	// 状态管理接口
	// ========================================================================
	
	/**
	 * 获取当前装备状态
	 * @return 当前的装备状态（Unequipped/Equipped）
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EYcEquipmentState GetEquipmentState() const { return EquipmentState; }
	
	/**
	 * 检查装备是否处于已装备状态
	 * @return true 表示当前处于已装备状态
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsEquipped() const { return EquipmentState == EYcEquipmentState::Equipped; }

	// ========================================================================
	// 基础查询接口
	// ========================================================================
	
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

	// ========================================================================
	// Actor生命周期管理
	// ========================================================================
	
	/**
	 * 生成装备所需的Actors（隐藏状态）
	 * 
	 * 根据配置分为两种生成策略：
	 * - bReplicateActor=true: 在服务器生成，自动复制到客户端
	 * - bReplicateActor=false: 仅在控制客户端本地生成
	 * 
	 * @param ActorsToSpawn 要生成的Actor配置列表
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void SpawnEquipmentActors(const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn);

	/**
	 * 销毁装备所生成的所有Actors
	 * 会同时销毁服务器复制的Actor和本地Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void DestroyEquipmentActors();
	
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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	AActor* FindSpawnedActorByTag(FName Tag, bool bReplicateActor = true);

	// ========================================================================
	// Actor可见性控制（蓝图可调用）
	// ========================================================================
	
	/**
	 * 显示装备Actors
	 * 
	 * 同时处理服务器复制的Actors和本地Actors的显示。
	 * 可在蓝图中调用，例如装备动画播放完成后调用此函数显示武器。
	 * 
	 * 内部会根据配置处理：
	 * - 从休眠状态唤醒（如果配置了bDormantOnUnequip）
	 * - 设置可见性为true
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ShowEquipmentActors();
	
	/**
	 * 隐藏装备Actors
	 * 
	 * 同时处理服务器复制的Actors和本地Actors的隐藏。
	 * 可在蓝图中调用，例如卸下动画播放完成后调用此函数隐藏武器。
	 * 
	 * 内部会根据配置处理：
	 * - 设置可见性为false
	 * - 进入休眠状态（如果配置了bDormantOnUnequip）
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void HideEquipmentActors();

	// ========================================================================
	// 事件委托
	// ========================================================================
	
	/** 
	 * 装备事件
	 * 在装备状态变为 Equipped 时广播
	 */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FEquipmentDelegate OnEquippedEvent;
	
	/** 
	 * 卸下事件
	 * 在装备状态变为 Unequipped 时广播
	 */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FEquipmentDelegate OnUnequippedEvent;
	
	/**
	 * 广播装备完成事件
	 * 通常在装备动画播放完成后由蓝图调用
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void BroadcastEquippedEvent();
	
	/**
	 * 广播卸下完成事件
	 * 通常在卸下动画播放完成后由蓝图调用
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void BroadcastUnequippedEvent();

	// ========================================================================
	// Fragment查询接口
	// ========================================================================
	
	/**
	 * C++版本：查询装备定义中的Fragment
	 * @tparam FragmentType 目标Fragment类型
	 * @return 找到的Fragment指针，如果不存在则返回nullptr
	 */
	template <typename FragmentType>
	const FragmentType* GetTypedFragment()
	{
		if (!GetEquipmentDef()) return nullptr;
		return EquipmentDef->GetTypedFragment<FragmentType>();
	}
	
	/**
	 * 蓝图版本：查询装备定义中的Fragment
	 * @param FragmentStructType 目标Fragment结构类型
	 * @return 查找到的Fragment结构体数据
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Equipment")
	TInstancedStruct<FYcEquipmentFragment> FindEquipmentFragment(const UScriptStruct* FragmentStructType);
	
protected:
	// ========================================================================
	// 状态变化回调（可在子类中重写）
	// ========================================================================
	
	/**
	 * 装备时的回调
	 * 在状态变为 Equipped 时调用，处理显示Actor等逻辑
	 * 可在子类中重写以扩展装备后的逻辑
	 */
	virtual void OnEquipped();
	
	/**
	 * 卸下时的回调
	 * 在状态变为 Unequipped 时调用，处理隐藏Actor等逻辑
	 * 可在子类中重写以扩展卸下后的逻辑
	 */
	virtual void OnUnequipped();
	
	/** 蓝图版本：装备时的回调 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment", meta = (DisplayName = "OnEquipped"))
	void K2_OnEquipped();

	/** 蓝图版本：卸下时的回调 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment", meta = (DisplayName = "OnUnequipped"))
	void K2_OnUnequipped();

	// ========================================================================
	// 网络复制回调
	// ========================================================================
	
	UFUNCTION()
	void OnRep_EquipmentState(EYcEquipmentState OldState);

	// ========================================================================
	// 客户端RPC
	// ========================================================================
	
	/** 通知控制客户端销毁本地Actors */
	UFUNCTION(Client, Reliable)
	void ClientDestroyLocalActors();
	
private:
	friend struct FYcEquipmentList;
	friend class UYcEquipmentManagerComponent;
	
	// ========================================================================
	// 内部Actor管理
	// ========================================================================
	
	/** 显示服务器复制的装备Actors（内部使用） */
	void ShowReplicatedActors();
	
	/** 隐藏服务器复制的装备Actors（内部使用） */
	void HideReplicatedActors();
	
	/** 显示本地客户端的装备Actors（内部使用） */
	void ShowLocalActors();
	
	/** 隐藏本地客户端的装备Actors（内部使用） */
	void HideLocalActors();
	
	/** 让单个Actor进入休眠状态 */
	void SetActorDormant(AActor* Actor, bool bReplicated);
	
	/** 让单个Actor从休眠状态唤醒 */
	void WakeActorFromDormant(AActor* Actor, bool bReplicated);
	
	/** 设置Actor的视觉可见性（不使用SetActorHiddenInGame避免网络复制问题） */
	void SetActorVisualVisibility(const AActor* Actor, bool bVisible);
	
	/** 内部函数：生成单个装备Actor */
	AActor* SpawnEquipActorInternal(const TSubclassOf<AActor>& ActorToSpawnClass, const FYcEquipmentActorToSpawn& SpawnInfo, USceneComponent* AttachTarget);
	
	// ========================================================================
	// 状态设置（仅服务器调用）
	// ========================================================================
	
public:
	/**
	 * 设置装备状态（仅服务器或主控客户端预测调用）
	 * @param NewState 新的装备状态
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetEquipmentState(EYcEquipmentState NewState);
	
protected:
	
	/**
	 * 设置装备实例对应的库存物品实例对象
	 * @param InInstigator 库存物品实例
	 */
	void SetInstigator(UYcInventoryItemInstance* InInstigator);
	
	/** 更新EquipmentDef缓存 */
	void UpdateEquipmentDef();
	
	// ========================================================================
	// 网络复制属性
	// ========================================================================
	
	/** 
	 * 装备状态 - 服务器权威，复制到所有客户端
	 * 通过 OnRep 回调处理状态变化
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquipmentState)
	EYcEquipmentState EquipmentState = EYcEquipmentState::Unequipped;
	
	/** 
	 * 服务器生成并复制到客户端的Actor列表
	 * 对应FYcEquipmentActorToSpawn中bReplicateActor=true的配置
	 */
	UPROPERTY(Replicated, VisibleInstanceOnly)
	TArray<TObjectPtr<AActor>> SpawnedActors;
	
	// ========================================================================
	// 本地状态（不参与网络复制）
	// ========================================================================
	
	/** 该装备对应的库存物品实例对象 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcInventoryItemInstance> Instigator;
	
	/** 
	 * 仅在控制客户端本地生成的Actor列表
	 * 对应FYcEquipmentActorToSpawn中bReplicateActor=false的配置
	 */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AActor>> OwnerClientSpawnedActors;
	
	/** 装备定义指针缓存 */
	const FYcEquipmentDefinition* EquipmentDef = nullptr;
	
	/** 标记是否已经生成过装备Actors */
	uint8 bActorsSpawned : 1;
	
	// ========================================================================
	// Actor自动显示/隐藏控制
	// ========================================================================
	
	/**
	 * 装备时是否自动显示Actors
	 * 
	 * - true（默认）：OnEquipped 时自动调用 ShowEquipmentActors()
	 * - false：OnEquipped 时不自动显示，需要在 K2_OnEquipped 蓝图中手动调用
	 *          目前暂未想到什么场景, 与bAutoHideEquipmentActors成对, 便于后续扩展使用
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Actor Visibility")
	uint8 bAutoShowEquipmentActors : 1;
	
	/**
	 * 卸下时是否自动隐藏Actors
	 * 
	 * - true（默认）：OnUnequipped 时自动调用 HideEquipmentActors()
	 * - false：OnUnequipped 时不自动隐藏，需要在 K2_OnUnequipped 蓝图中手动调用
	 *          适用于需要先播放卸下动画，动画完成后再隐藏武器的场景
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Actor Visibility")
	uint8 bAutoHideEquipmentActors : 1;
};
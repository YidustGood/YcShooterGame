// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DataRegistryId.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcInventoryManagerComponent.h"
#include "YcInventoryOperationTypes.h"
#include "Fragments/ItemFragment_ContextMenu.h"
#include "Fragments/ItemFragment_GridItem.h"
#include "Fragments/ItemFragment_GridRegions.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GridInventoryManagerComponent.generated.h"

class UYcInventoryItemInstance;
class UYcInventoryOperationRouterComponent;
class UYcEquipmentSlotComponent;
struct FGameplayMessageListenerHandle;
struct FYcInventoryItemChangeMessage;
struct FYcEquipmentSlotChangedMessage;

/**
 * 单格运行时状态（占用、归属物品、相对偏移）。 * Runtime state of a single grid slot (occupation, owning item, relative offset).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOccupied = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FYcItemInstanceId OccupyingItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemRelativeX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemRelativeY = 0;

	UPROPERTY()
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	void Reset();
};

/**
 * 物品在网格中的落位缓存（左上角、尺寸、区域口袋）。 * Cached placement info for one item (top-left tile, size, region/pocket).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemGridInfo
{
	GENERATED_BODY()

	FItemGridInfo() = default;
	FItemGridInfo(const FIntPoint InTilePos, const FIntPoint InItemSize);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint TilePos = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint ItemSize = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;
};

/**
 * 区域口袋运行时状态（用于布局与自动放置优先级）。 * Runtime state for a region pocket (layout and auto-placement priority).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryRegionRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Columns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Rows = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint LayoutOffset = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;
};

/** 区域口袋槽位存储。/ Slot storage for one region pocket. */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryRegionSlotsStorage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridInventorySlot> Slots;
};

/** 区域口袋形状存储。/ Shape-cell storage for one region pocket. */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryRegionShapeStorage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> ShapeCells;
};

/** 区域标签约束缓存。/ Cached tag-constraint for a region. */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryRegionTagConstraintStorage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGridRegionTagConstraint Constraint;
};

/** 卸装前重排的单步移动。/ One relocation move in pre-unequip planning. */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FUnequipRelocateMove
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Tile = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRotated = false;
};

/** 卸装重排时的口袋模拟状态。/ Simulated pocket state for unequip planning. */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FUnequipRegionPocketSimState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Columns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Rows = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridInventorySlot> Slots;
};

/** 右键动作请求消息载体。/ Message payload for context-action request. */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridItemContextActionRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Player = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UYcInventoryItemInstance> ItemInst = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag ActionTag;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag ExecutorTag;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTag EventTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryGridChanged);

/**
 * 网格库存核心组件（C++版）。 * C++ grid-inventory core component.
 *
 * 主要职责。 * - 网格占用与落位管理（含旋转）
 * - 多区域多口袋与形状、标签约。 * - 容器搜索会话（揭示、进度、并发重建）
 * - 交换操作服务端校?执行与回。 * - 卸装前区域重排计划与应用
 *
 * Responsibilities:
 * - Grid occupancy and placement (with rotation)
 * - Multi-region/pocket with shape and tag constraints
 * - Container search session (reveal/progress/rebuild)
 * - Server-side swap validation/execution with rollback
 * - Pre-unequip relocation planning and apply
 */
UCLASS(ClassGroup=(YiChenInventory), Blueprintable, meta=(BlueprintSpawnableComponent))
class YCGRIDINVENTORYRUNTIME_API UGridInventoryManagerComponent : public UYcInventoryManagerComponent
{
	GENERATED_BODY()

public:
	UGridInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool CanAcceptItemForReturn_Implementation(UYcInventoryItemInstance* ItemInstance, FString& OutReason) override;
	virtual bool CanApplyInventoryRelocation_Implementation(const FYcInventoryRelocationRequest& Request, FString& OutReason) override;
	virtual bool ApplyInventoryRelocation_Implementation(const FYcInventoryRelocationRequest& Request, FString& OutReason) override;

	/** 基础区域定义（静态配置）。/ Base region definitions from static config. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionDefinition> BaseRegionDefinitions;

	/** 主区域列数（兼容旧网格接口）。/ Main-region columns (legacy compatibility). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventoryColumns = 10;

	/** 主区域行数（兼容旧网格接口）。/ Main-region rows (legacy compatibility). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventoryRows = 10;

	/** 容器是否启用“逐项搜索揭示”。/ Whether container-search reveal is enabled. */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Search")
	bool bEnableContainerSearch = false;

	/** 是否允许直接容器交互（无搜索门禁）。/ Allow direct container interaction bypassing search gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bAllowDirectContainerInteraction = false;

	/** 旧接口兼容槽位视图（主区域镜像）。/ Legacy-compatible slot view mirrored from primary region. */
	UPROPERTY(ReplicatedUsing = OnRep_InventorySlots, VisibleAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FGridInventorySlot> InventorySlots;

	/** 网格版本号（用于UI增量刷新）。/ Grid revision for UI incremental refresh. */
	UPROPERTY(ReplicatedUsing = OnRep_InventoryGridRevision, BlueprintReadOnly, Category = "Inventory")
	int32 InventoryGridRevision = 0;

	/** 启用中的区域口袋状态。/ Enabled runtime region-pocket states. */
	UPROPERTY(ReplicatedUsing = OnRep_RegionStates, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionRuntimeState> RegionStates;

	/** 网格变化广播（UI订阅）。/ Grid changed event for UI observers. */
	UPROPERTY(BlueprintAssignable)
	FInventoryGridChanged OnInventoryGridChanged;

	/** 搜索速度倍率（耗时=基础时长/倍率）。/ Search speed multiplier (duration = base/speed). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Search")
	float SearchSpeedMultiplier = 1.0f;

	/** 初始化网格与区域运行时状态。/ Initialize runtime grid and region states. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory();

	/** 按物品定义向网格添加物品（服务端）。/ Add item by definition into grid (server only). */
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool TryAddGridItemByDefinition(FDataRegistryId ItemDefId, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1);

	/** 按物品实例向网格添加物品（服务端）。/ Add item instance into grid (server only). */
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool TryAddGridItemInstance(UYcInventoryItemInstance* ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1);

	/** 从网格移除物品（服务端）。/ Remove an item from grid (server only). */
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool RemoveGridItem(UYcInventoryItemInstance* ItemInst);

	/** 物品加入库存时写入网格落位（服务端）。/ Register placement when item is added (server only). */
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool OnGridItemInstanceAdded(UYcInventoryItemInstance* ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1);

	/** 物品移除库存时清理网格占用（服务端）。/ Clear placement when item is removed (server only). */
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	void OnRemoveGridItem(UYcInventoryItemInstance* ItemInst);

	/** 获取当前搜索速度倍率。/ Get current container-search speed multiplier. */
	UFUNCTION(BlueprintCallable, Category = "Search")
	float GetSearchSpeedMultiplier() const;

	/** 设置搜索速度倍率（服务端）。/ Set container-search speed multiplier (server only). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Search")
	void SetSearchSpeedMultiplier(float NewValue);

	/** 判断物品在当前搜索会话中是否已揭示。/ Check whether item is revealed in current search session. */
	UFUNCTION(BlueprintCallable, Category = "Search")
	bool IsItemRevealedForCurrentSession(UYcInventoryItemInstance* ItemInst) const;

	/** 判断物品在当前搜索会话中是否可操作。/ Check whether item is operable in current search session. */
	UFUNCTION(BlueprintCallable, Category = "Search")
	bool IsItemOperableForCurrentSession(UYcInventoryItemInstance* ItemInst) const;

	/** 获取当前搜索进度信息。/ Get current search progress and counters. */
	UFUNCTION(BlueprintCallable, Category = "Search")
	bool GetCurrentSearchProgress(float& OutProgress01, float& OutRemainingSeconds, int32& OutRevealedCount, int32& OutTotalCount) const;

	/** 获取某容器会话修订号。/ Get search-session revision for a container. */
	UFUNCTION(BlueprintCallable, Category = "Search")
	bool GetSearchSessionRevisionForContainer(UGridInventoryManagerComponent* ContainerInventory, int32& OutRevision) const;

	/** 获取网格修订号。/ Get current inventory grid revision. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetInventoryGridRevision() const;

	/** 获取物品右键菜单动作列表。/ Collect context-menu actions for an item. */
	UFUNCTION(BlueprintCallable, Category = "ContextMenu")
	bool GetContextMenuActionsForItem(UYcInventoryItemInstance* ItemInst, TArray<FGridItemContextMenuAction>& OutActions);

	/** 检查动作是否可执行并返回动作定义。/ Check whether a context action can execute and return action definition. */
	UFUNCTION(BlueprintCallable, Category = "ContextMenu")
	bool CanExecuteContextAction(UYcInventoryItemInstance* ItemInst, FGameplayTag ActionTag, FGridItemContextMenuAction& OutActionDef);

	/** 请求服务端执行物品动作。/ Ask server to execute an item context action. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerRequestExecuteItemContextAction(UYcInventoryItemInstance* ItemInst, FGameplayTag ActionTag);

	/** 请求服务端开启容器搜索会话。/ Ask server to start a container search session. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerStartContainerSearchSession(UGridInventoryManagerComponent* ContainerInventory);

	/** 请求服务端重置搜索会话。/ Ask server to reset current search session. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerResetSearchSession();

	/** 请求服务端将物品丢弃到场景中。/ Ask server to drop an item into the world. */
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerDropInventoryItem(UYcInventoryItemInstance* ItemInst);

	/** 校验交换类操作是否合法（服务端）。/ Validate swap-like operation on server. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool ValidateSwapLikeOperation(const FYcInventoryOperation& InOperation, FString& OutReason);

	/** 执行交换类操作（服务端）。/ Execute swap-like operation on server. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool ExecuteSwapLikeOperation(const FYcInventoryOperation& InOperation, FString& OutReason);

	/** 构建交换类操作的同步增量。/ Build replication delta for swap-like operation result. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void BuildSwapLikeOperationDelta(const FYcInventoryOperation& InOperation, bool bSuccess, FYcInventoryOperationDelta& OutDelta);

	/** 以统一操作模型执行网格交换。/ Execute grid swap via unified operation model. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool ExecuteSwapGridOperation(UYcInventoryManagerComponent* OnDropInventory, UYcInventoryItemInstance* ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated, UYcInventoryManagerComponent* ExpectedSourceInventory, FString& OutReason, FString& OutSummary);

	/** 默认掉落生成的拾取 Actor 类。/ Pickup actor class used for item drops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Drop")
	TSubclassOf<AActor> DropPickupActorClass;

	/** 按物品ID查询左上角格子坐标。/ Query top-left tile by item id. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemLeftTopPosition(FYcItemInstanceId ItemID, FIntPoint& OutPoint);

	/** 获取当前网格物品到坐标映射。/ Get item-to-tile map in current grid. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TMap<UYcInventoryItemInstance*, FIntPoint> GetGridItemsTileMap();

	/** 获取指定区域/口袋的物品到坐标映射。/ Get item-to-tile map for a specific region/pocket. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TMap<UYcInventoryItemInstance*, FIntPoint> GetGridItemsTileMapByRegion(FGameplayTag RegionId, int32 PocketIndex = -1);

	/** 获取当前网格物品旋转状态映射。/ Get item rotation-state map in current grid. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TMap<UYcInventoryItemInstance*, bool> GetGridItemRotationMap();

	/** 在主流程下查找首个可放置位置。/ Find first fit tile in default placement flow. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindFirstFitPosition(FDataRegistryId ItemDefId, FIntPoint& Tile, bool& OutRotated);

	/** 在多区域/多口袋中查找首个可放置落位。/ Find first fit placement across enabled regions/pockets. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPlacement(FDataRegistryId ItemDefId, FGameplayTag& OutRegionId, int32& OutPocketIndex, FIntPoint& Tile, bool& OutRotated);

	/** 按实例在多区域/多口袋中查找首个可放置落位。/ Find first fit placement for an item instance. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPlacementForItemInst(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId, int32& OutPocketIndex, FIntPoint& Tile, bool& OutRotated);

	/** 兼容旧接口的首个可放置落位查找。/ Legacy-compatible first-fit placement query. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPlacementLegacy(FDataRegistryId ItemDefId, FGameplayTag& OutRegionId, FIntPoint& Tile, bool& OutRotated);

	/** 在指定区域/口袋中查找首个可放置位置。/ Find first fit tile in a specific region/pocket. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPositionInRegion(FDataRegistryId ItemDefId, FGameplayTag RegionId, int32 PocketIndex, FIntPoint& Tile, bool& OutRotated);

	/** 在指定区域中查找可放置位置（旧接口）。/ Legacy first-fit query in a specific region. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPositionInRegionLegacy(FDataRegistryId ItemDefId, FGameplayTag RegionId, FIntPoint& Tile, bool& OutRotated);

	/** 检查物品定义是否通过区域标签约束。/ Check region tag constraint for an item definition. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Rules")
	bool PassesRegionTagConstraintForItemDef(FDataRegistryId ItemDefId, FGameplayTag RegionId);

	/** 检查物品实例是否通过区域标签约束。/ Check region tag constraint for an item instance. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Rules")
	bool PassesRegionTagConstraintForItemInst(UYcInventoryItemInstance* ItemInst, FGameplayTag RegionId);

	/** 检查物品定义能否放在指定位置。/ Test placement feasibility for item definition. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceGridItem(FDataRegistryId ItemDefId, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1);

	/** 检查物品实例能否放在指定位置。/ Test placement feasibility for item instance. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceGridItemInst(UYcInventoryItemInstance* ItemInst, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1);

	/** 获取物品网格碎片数据。/ Get grid fragment from item definition. */
	UFUNCTION(BlueprintPure)
	FItemFragment_GridItem GetItemFragmentGrid(FDataRegistryId ItemDefId) const;

	/** 线性索引转格子坐标。/ Convert linear index to tile coordinates. */
	UFUNCTION(BlueprintCallable)
	FIntPoint IndexToTile(int32 Index) const;

	/** 格子坐标转线性索引。/ Convert tile coordinates to linear index. */
	UFUNCTION(BlueprintPure)
	int32 TileToIndex(FIntPoint Tile) const;

	/** 获取主区域ID。/ Get primary region id. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	FGameplayTag GetPrimaryRegionId() const;

	/** 获取区域主口袋索引。/ Get primary pocket index for a region. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetPrimaryPocketIndex(FGameplayTag RegionId) const;

	/** 获取已启用区域ID并按优先级排序。/ Get enabled region ids sorted by priority. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	TArray<FGameplayTag> GetEnabledRegionIdsSorted() const;

	/** 获取已启用口袋运行时状态。/ Get enabled runtime pocket states. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionRuntimeState> GetEnabledPocketStates() const;

	/** 查询物品当前区域/口袋归属。/ Get region/pocket placement for an item. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool GetItemPlacementRegion(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId, int32& OutPocketIndex) const;

	/** 查询物品当前区域归属（旧接口）。/ Legacy region-only placement query. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool GetItemPlacementRegionLegacy(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId) const;

	/** 获取区域列数。/ Get columns of a region pocket. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetRegionColumns(FGameplayTag RegionId, int32 PocketIndex = -1) const;

	/** 获取区域行数。/ Get rows of a region pocket. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetRegionRows(FGameplayTag RegionId, int32 PocketIndex = -1) const;

	/** 获取区域优先级。/ Get priority of a region pocket. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetRegionPriority(FGameplayTag RegionId, int32 PocketIndex = -1) const;

	/** 在指定区域/口袋中按实例查找首个可放置位置。/ Find first fit in specific region/pocket for item instance. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPositionInRegionForItemInst(UYcInventoryItemInstance* ItemInst, FGameplayTag RegionId, int32 PocketIndex, FIntPoint& Tile, bool& OutRotated);

	/** 预设下一次回归库存时使用的落点。/ Prime placement for the next returned item. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	void PreparePreferredReturnPlacement(FIntPoint Tile, FGameplayTag RegionId, int32 PocketIndex);

	/** 调试打印当前槽位占用。/ Print current grid slot occupancy for debugging. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DebugPrintSlots();

private:
	UFUNCTION()
	void OnRep_InventorySlots();
	UFUNCTION()
	void OnRep_InventoryGridRevision();
	UFUNCTION()
	void OnRep_RegionStates();
	// 客户端复制更新后的统一处理（重建主区域缓存并触发UI刷新）。
	void HandleReplicatedGridStateChanged();
	// 从复制到客户端的主区域槽位重建本地落位缓存（供蓝图查询函数使用）。
	void RebuildPrimaryPlacementCacheFromLegacySlots();

	// 处理库存物品变更消息。
	void OnInventoryChanged(FGameplayTag ActualTag, const FYcInventoryItemChangeMessage& Data);
	// 处理装备槽位变更消息。
	void OnEquipmentSlotChanged(FGameplayTag ActualTag, const FYcEquipmentSlotChangedMessage& Data);
	// 推进容器搜索会话计时并揭示物品。
	void TickSearchSession();
	// 直接执行物品落位交换（内部路径）。
	void InnerSwapItemPosition(UYcInventoryItemInstance* ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1);
	// 判断消息归属者是否与本组件归属一致。
	bool IsOwnerActorMatched(const AActor* MessageOwner, const AActor* LocalOwner) const;
	AActor* ResolveDropInstigatorActor() const;
	UClass* ResolveDropPickupActorClass() const;
	// 获取物品落位坐标与旋转信息。
	bool TryGetItemPlacementInfo(UYcInventoryItemInstance* ItemInst, FIntPoint& OutTile, bool& bOutRotated) const;
	// 获取物品完整落位信息（区域/口袋/坐标/旋转）。
	bool TryGetItemPlacementInfoWithRegion(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId, int32& OutPocketIndex, FIntPoint& OutTile, bool& bOutRotated) const;
	// 判断区域口袋是否启用。
	bool IsRegionStateEnabled(FGameplayTag RegionId, int32 PocketIndex) const;
	// 提交落位但不触发外部刷新广播。
	bool CommitPlacementWithoutBroadcast(UYcInventoryItemInstance* ItemInst, FIntPoint Tile, bool bRotated, FGameplayTag RegionId, int32 PocketIndex);
	// 根据落位映射重建区域槽位占用。
	void RebuildRegionSlotsFromPlacementMap();
	// 重置搜索会话内部状态。
	void ResetSearchSession_Internal();
	// 判断物品是否已记录为“本局已知”。
	bool IsItemKnownInMatch(UYcInventoryItemInstance* ItemInst) const;
	// 将物品标记为“本局已知”。
	void MarkItemKnownInMatch(UYcInventoryItemInstance* ItemInst);
	// 判断物品是否仍在指定容器中。
	bool IsItemStillInContainer(UYcInventoryItemInstance* ItemInst, UGridInventoryManagerComponent* ContainerInventory) const;
	// 保留当前项并重建搜索队列。
	void RebuildSearchQueueFromContainerPreserveCurrent();
	// 从容器内容重建搜索队列。
	void BuildSearchQueue(UGridInventoryManagerComponent* ContainerInventory);
	// 启动下一个待搜索物品。
	void StartNextSearchItem();
	// 将目标物品标记为已揭示并更新计数。
	void RevealItem(UYcInventoryItemInstance* ItemInst);
	// 判断物品在指定容器会话中是否已揭示。
	bool IsItemRevealedForContainerSession(UGridInventoryManagerComponent* ItemOuterInventory, UYcInventoryItemInstance* ItemInst) const;
	// 评估右键动作是否可执行并输出禁用原因。
	bool EvaluateContextActionExecutability(UYcInventoryItemInstance* ItemInst, const FGridItemContextMenuAction& ActionDef, FText& OutDisabledReason);
	// 校验动作执行器标签是否合法。
	bool IsValidContextActionExecutorTag(FGameplayTag ExecutorTag) const;
	// 查找指定动作标签对应的配置定义。
	bool TryFindContextMenuActionDef(UYcInventoryItemInstance* ItemInst, FGameplayTag ActionTag, FGridItemContextMenuAction& OutActionDef) const;
	// 在权威态检查动作执行前置条件。
	bool CanExecuteContextActionInAuthoritativeState(UYcInventoryItemInstance* ItemInst) const;
	// 将区域坐标转换为区域内线性索引。
	int32 TileToIndexInRegion(FIntPoint Tile, int32 RegionColumns) const;
	// 解析最终使用的放置区域ID。
	FGameplayTag ResolvePlacementRegionId(FGameplayTag RegionId) const;
	// 解析最终使用的放置口袋索引。
	int32 ResolvePlacementPocketIndex(FGameplayTag RegionId, int32 PocketIndex) const;
	// 获取指定区域口袋槽位快照。
	TArray<FGridInventorySlot> GetRegionSlots(FGameplayTag RegionId, int32 PocketIndex = -1) const;
	// 设置指定区域口袋槽位数据。
	void SetRegionSlots(FGameplayTag RegionId, int32 PocketIndex, const TArray<FGridInventorySlot>& InSlots);
	// 查找区域槽位存储索引。
	int32 FindRegionSlotsStorageIndex(FGameplayTag RegionId, int32 PocketIndex) const;
	// 设置区域口袋形状格子数据。
	void SetRegionShapeCells(FGameplayTag RegionId, int32 PocketIndex, const TArray<FIntPoint>& InCells);
	// 查找区域形状存储索引。
	int32 FindRegionShapeStorageIndex(FGameplayTag RegionId, int32 PocketIndex) const;
	// 设置区域标签约束缓存。
	void SetRegionTagConstraint(FGameplayTag RegionId, const FGridRegionTagConstraint& Constraint);
	// 查找区域标签约束存储索引。
	int32 FindRegionTagConstraintStorageIndex(FGameplayTag RegionId) const;
	// 获取区域标签约束缓存。
	FGridRegionTagConstraint GetRegionTagConstraint(FGameplayTag RegionId) const;
	// 判断区域格子是否可用。
	bool IsRegionCellAvailable(FGameplayTag RegionId, int32 PocketIndex, FIntPoint Tile) const;
	// 同步主区域到旧版槽位数组。
	void SyncPrimaryRegionToLegacySlots();
	// 按当前装备重建可用区域配置。
	void BuildRegionsFromCurrentLoadout();
	// 解析装备物品提供的区域ID集合。
	bool GetProvidedRegionIdsFromItem(UYcInventoryItemInstance* ItemInst, TArray<FGameplayTag>& OutRegionIds) const;
	// 计算物品占用格子面积。
	int32 GetGridItemArea(UYcInventoryItemInstance* ItemInst) const;
	// 在模拟口袋中尝试为物品寻找可用落位。
	bool TryFindFitInSimRegion(UYcInventoryItemInstance* ItemInst, FGameplayTag TargetRegionId, TArray<FUnequipRegionPocketSimState>& SimPockets, int32& OutPocketIndex, FIntPoint& OutTile, bool& OutRotated);
	// 在模拟口袋中尝试将物品放到指定位置。
	bool TryOccupyExactFitInSimRegion(UYcInventoryItemInstance* ItemInst, FGameplayTag TargetRegionId, TArray<FUnequipRegionPocketSimState>& SimPockets, int32 PreferredPocketIndex, FIntPoint PreferredTile, bool bPreferredRotated);
	// 基于目标装备物品构建卸装前重排计划并输出失败原因。
	bool TryBuildUnequipRelocationPlan(UYcInventoryItemInstance* EquippedItem, const FUnequipRelocateMove* PreferredEquipMove, TArray<FUnequipRelocateMove>& OutRelocateMoves, FUnequipRelocateMove& OutEquipMove, FString& OutReason);
	// 缓存最近一次可用的卸装重排计划，避免 CanApply/Apply 重复求解。
	struct FRelocationPlanCache
	{
		TWeakObjectPtr<UYcInventoryItemInstance> AnchorItem = nullptr;
		int32 GridRevision = INDEX_NONE;
		bool bValid = false;
		TArray<FUnequipRelocateMove> RelocateMoves;
		FUnequipRelocateMove EquipMove;
	};
	// 尝试读取并校验缓存计划。
	bool TryGetCachedRelocationPlan(UYcInventoryItemInstance* AnchorItem, TArray<FUnequipRelocateMove>& OutRelocateMoves, FUnequipRelocateMove& OutEquipMove) const;
	// 写入缓存计划。
	void CacheRelocationPlan(UYcInventoryItemInstance* AnchorItem, const TArray<FUnequipRelocateMove>& RelocateMoves, const FUnequipRelocateMove& EquipMove);
	// 清理缓存计划。
	void InvalidateRelocationPlanCache();

	// 区域口袋槽位运行时缓存。
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionSlotsStorage> RegionSlotsStorage;

	// 区域口袋形状运行时缓存。
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionShapeStorage> RegionShapeStorage;

	// 区域标签约束运行时缓存。
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionTagConstraintStorage> RegionTagConstraintStorage;

	// 物品实例到网格落位信息的主映射。
	TMap<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo> ItemInstanceToTileMap;
	// 交换操作中的临时缓存坐标。
	TOptional<FIntPoint> CachedTile;
	// 交换操作中的临时缓存区域ID。
	TOptional<FGameplayTag> CachedRegionId;
	// 交换操作中的临时缓存口袋索引。
	TOptional<int32> CachedPocketIndex;

	// 当前处理目标区域ID。
	FGameplayTag CurrentHandleRegionId;
	// 当前预期来源区域ID。
	FGameplayTag CurrentExpectedSourceRegionId;
	// 当前处理目标口袋索引。
	int32 CurrentHandlePocketIndex = -1;
	// 当前预期来源口袋索引。
	int32 CurrentExpectedSourcePocketIndex = -1;

	// 当前是否存在激活中的搜索会话。
	UPROPERTY(Replicated)
	bool bSearchSessionActive = false;

	// 当前搜索会话绑定的容器组件。
	UPROPERTY(Replicated)
	TObjectPtr<UGridInventoryManagerComponent> SearchContainerInventory = nullptr;

	// 当前会话已揭示物品列表。
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UYcInventoryItemInstance>> RevealedSearchItems;

	// 当前对局“已知物品”集合。
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UYcInventoryItemInstance>> KnownSearchItems;

	// 当前正在搜索揭示的物品。
	UPROPERTY(Replicated)
	TObjectPtr<UYcInventoryItemInstance> CurrentSearchingItem = nullptr;

	// 当前搜索目标的进度（0-1）。
	UPROPERTY(Replicated)
	float CurrentSearchProgress01 = 0.0f;

	// 当前搜索会话总物品数。
	UPROPERTY(Replicated)
	int32 SearchTotalItemCount = 0;

	// 当前搜索会话已揭示物品数。
	UPROPERTY(Replicated)
	int32 SearchRevealedItemCount = 0;

	// 搜索会话修订号（用于客户端刷新判定）。
	UPROPERTY(Replicated)
	int32 SearchSessionRevision = 0;

	// 等待揭示的搜索队列。
	TArray<TObjectPtr<UYcInventoryItemInstance>> PendingSearchQueue;
	// 当前搜索目标的理论耗时。
	float CurrentSearchTargetDuration = 0.0f;
	// 搜索推进计时器步长。
	float SearchTickInterval = 0.1f;
	// 搜索推进计时器句柄。
	FTimerHandle SearchTickTimerHandle;
	// 库存变更消息监听句柄。
	FGameplayMessageListenerHandle InventoryChangedHandle;
	// 装备槽位变更消息监听句柄。
	FGameplayMessageListenerHandle EquipmentSlotChangedHandle;
	// Router 操作处理器是否已注册。
	bool bOperationHandlersRegistered = false;
	// 最近一次重排计划缓存。
	FRelocationPlanCache CachedRelocationPlan;
	// 客户端已处理的网格修订号，用于避免同一修订重复广播刷新。
	int32 LastObservedClientGridRevision = INDEX_NONE;
};



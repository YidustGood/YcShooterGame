// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "System/YcSaveCoreTypes.h"
#include "System/YcMetaInventoryTypes.h"
#include "YcMetaInventorySubsystem.generated.h"

struct FGameplayTag;
struct FYcInventoryOperationStateMessage;
class UYcInventorySceneContext;
class UYcInventoryPersistenceExtensionProvider;
class UYcInventoryManagerComponent;
class UYcInventoryItemInstance;
class AActor;
class UObject;
class UClass;

/**
 * Meta Inventory 子系统（GameInstance 级）。
 * 负责局外库存上下文管理、快照构建/恢复、脏标记与持久化编排。
 */
UCLASS(Config=Game)
class YICHENINVENTORY_API UYcMetaInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 子系统初始化：注册消息监听并准备持久化提供者。 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** 子系统反初始化：落盘脏数据并清理运行时上下文。 */
	virtual void Deinitialize() override;
	
	static UYcMetaInventorySubsystem* Get(const UObject* WorldContextObject);

	/** 注册一个库存场景上下文。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	void RegisterSceneContext(UYcInventorySceneContext* Context);

	/** 注销一个库存场景上下文。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	void UnregisterSceneContext(UYcInventorySceneContext* Context);

	/** 加载账号快照；若不存在则初始化新档（仅局外上下文）。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool LoadOrInitializeProfile(UYcInventorySceneContext* Context);

	/** 保存指定上下文对应账号快照（仅局外上下文）。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool SaveProfile(UYcInventorySceneContext* Context);

	/** 批量保存当前标记为脏的账号（仅局外上下文）。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool SaveDirtyProfiles();

	/** 查询档位是否处于脏状态。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool IsProfileDirty(const FYcProfileIdentity& ProfileIdentity) const;

	/** 标记档位为脏（待保存）。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	void MarkProfileDirty(const FYcProfileIdentity& ProfileIdentity);

	/** 清理档位脏标记。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	void ClearProfileDirty(const FYcProfileIdentity& ProfileIdentity);

	/** 从运行时组件构建快照。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool BuildSnapshotFromContext(UYcInventorySceneContext* Context, FYcMetaInventoryRootSnapshot& OutSnapshot) const;

	/** 将快照应用到运行时组件。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool ApplySnapshotToContext(UYcInventorySceneContext* Context, const FYcMetaInventoryRootSnapshot& Snapshot);

	/** 快速搭建局外上下文并加载存档。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool SetupOutOfMatchContextAndLoad(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime);

	/** 保存指定档位对应的局外上下文。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool SaveOutOfMatchContext(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime);

	/**
	 * 从局外持久化档案读取玩家负载，并应用到局内玩家（背包/装备/快捷栏）。
	 * 仅恢复 Player 快照，不修改 Stash 快照。
	 * 不依赖 SceneContext 的 SceneType，目标是同账号根档中的 Player 分区。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool LoadPlayerLoadoutToInMatch(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime);

	/**
	 * 将局内玩家当前负载回写到局外档案（按是否成功撤离执行不同策略）。
	 * - 撤离成功：用局内玩家当前背包/装备/快捷栏覆盖档案中的 Player 快照。
	 * - 撤离失败：清空档案中的 Player 快照。
	 * 不修改 Stash 快照。
	 * 不依赖 SceneContext 的 SceneType，目标是同账号根档中的 Player 分区。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Meta")
	bool CommitInMatchPlayerLoadoutToProfile(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime, bool bExtractionSucceeded = true);

	bool BuildPlayerSnapshotFromRuntime(const FYcPlayerInventoryRuntime& Runtime, FYcMetaPlayerSnapshot& OutPlayerSnapshot) const;
	bool ApplyPlayerSnapshotToRuntime(const FYcPlayerInventoryRuntime& Runtime, const FYcMetaPlayerSnapshot& PlayerSnapshot);

private:
	/** 校验局外上下文是否允许执行档案读写。 */
	bool ValidateOutOfMatchContext(const UYcInventorySceneContext* Context, const TCHAR* Caller) const;

	/** 校验局内负载接口入参。 */
	bool ValidateRuntimeRequest(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime, bool bRequireOutOfMatchRuntime, const TCHAR* Caller) const;
	class UYcProfileSaveSubsystem* GetProfileSaveSubsystem() const;
	void RegisterContextProfileKey(UYcInventorySceneContext* Context);
	void UnregisterContextProfileKey(UYcInventorySceneContext* Context);

	/** 监听操作状态消息，在 Ack 后给相关账号打脏标。 */
	void OnOperationStateChanged(FGameplayTag ActualTag, const FYcInventoryOperationStateMessage& Message);
	/** 从某个库存组件构建“物品+扩展载荷”记录。 */
	bool BuildInventoryRecords(UYcInventoryManagerComponent* Inventory, TArray<FYcMetaInventoryItemRecord>& OutItems, TArray<FYcMetaInventoryExtensionPayload>& OutExtensions) const;
	/** 将“物品+扩展载荷”记录恢复到库存组件。 */
	bool RestoreInventoryRecords(UYcInventoryManagerComponent* Inventory, const TArray<FYcMetaInventoryItemRecord>& InItems, const TArray<FYcMetaInventoryExtensionPayload>& InExtensions, TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& OutItemMap);
	/** 构建库存扩展载荷。 */
	bool BuildInventoryExtensions(const UYcInventoryManagerComponent* Inventory, TArray<FYcMetaInventoryExtensionPayload>& OutExtensions) const;
	/** 应用库存扩展载荷。 */
	bool ApplyInventoryExtensions(UYcInventoryManagerComponent* Inventory, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& ItemMap, const TArray<FYcMetaInventoryExtensionPayload>& InExtensions) const;
	/** 收集当前可用的扩展 Provider 实例。 */
	void GatherPersistenceExtensionProviders(TArray<const UYcInventoryPersistenceExtensionProvider*>& OutProviders) const;

	/** 构建玩家侧快照（背包/装备/快捷栏）。 */
	bool BuildPlayerSnapshot(const FYcPlayerInventoryRuntime& Runtime, FYcMetaPlayerSnapshot& OutPlayerSnapshot) const;
	/** 将玩家侧快照应用到运行时（背包/装备/快捷栏）。 */
	bool ApplyPlayerSnapshot(const FYcPlayerInventoryRuntime& Runtime, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>* StashItemMap, const FYcMetaPlayerSnapshot& PlayerSnapshot);

	/** 构建装备槽记录。 */
	void BuildEquipmentRecords(const FYcPlayerInventoryRuntime& Runtime, TArray<FYcMetaEquipmentSlotRecord>& OutSlots) const;
	/** 构建快捷栏记录。 */
	void BuildQuickBarRecords(const FYcPlayerInventoryRuntime& Runtime, TArray<FYcMetaQuickBarSlotRecord>& OutSlots) const;

	/** 清空当前装备槽状态。 */
	void ClearEquipment(const FYcPlayerInventoryRuntime& Runtime) const;
	/** 清空当前快捷栏状态。 */
	void ClearQuickBar(const FYcPlayerInventoryRuntime& Runtime) const;
	/** 根据快照恢复装备槽。 */
	bool RestoreEquipment(const FYcPlayerInventoryRuntime& Runtime, const TArray<FYcMetaEquipmentSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const;
	/** 根据快照恢复快捷栏。 */
	bool RestoreQuickBar(const FYcPlayerInventoryRuntime& Runtime, const TArray<FYcMetaQuickBarSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const;

private:
	/** 已注册的场景上下文列表。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UYcInventorySceneContext>> SceneContexts;

	/** 档位键到上下文对象的快速映射。 */
	UPROPERTY(Transient)
	TMap<FYcProfileSaveKey, TObjectPtr<UYcInventorySceneContext>> ContextByProfileKey;

	/** 脏档位集合。 */
	UPROPERTY(Transient)
	TSet<FYcProfileSaveKey> DirtyProfiles;

	/** 未识别扩展载荷透传缓存（Key=ItemInstId，运行时缓存，非反射字段）。 */
	TMap<FYcItemInstanceId, TArray<FYcMetaItemExtensionPayload>> UnknownItemExtensionPayloads;

	/** 操作状态消息监听句柄。 */
	FGameplayMessageListenerHandle OperationStateListener;
};

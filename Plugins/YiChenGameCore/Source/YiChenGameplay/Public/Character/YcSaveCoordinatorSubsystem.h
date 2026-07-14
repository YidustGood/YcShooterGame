// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "GameplayTagContainer.h"
#include "YcSaveCoordinatorSubsystem.generated.h"

class APlayerController;
class UYcPlayerPersistenceComponent;
struct FYcInventoryItemChangeMessage;
struct FYcEquipmentSlotChangedMessage;
struct FYcQuickBarSlotsChangedMessage;
struct FYcPersistenceRequestMessage;
struct FYcPersistenceProfileMessage;
struct FYcInventoryProjectedStateChangedMessage;

/**
 * SaveCoordinator 对单个本地玩家的保存状态缓存。
 * 它记录了“是否已脏、下次自动保存截止时间、是否正在保存、失败后是否等待就绪重试”等信息。
 */
USTRUCT()
struct FYcSaveCoordinatorPlayerState
{
	GENERATED_BODY()

	/** 当前被跟踪的玩家控制器。 */
	TWeakObjectPtr<APlayerController> PlayerController;
	/** 最近一次把玩家标记为脏时携带的原因标签。 */
	FGameplayTag LastDirtyReasonTag;
	/** 最近一次保存失败时记录下来的原因标签。 */
	FGameplayTag LastFailureReasonTag;
	/** 自动保存的目标触发时间。 */
	double AutosaveDeadlineSeconds = 0.0;
	/** 最近一次成功保存的时间。 */
	double LastSuccessfulSaveTimeSeconds = 0.0;
	/** 当前是否存在尚未落盘的改动。 */
	bool bDirty = false;
	/** 当前是否正处于一次保存调用中。 */
	bool bSaveInFlight = false;
	/** 当前保存结束后是否还需要再补一次强制保存。 */
	bool bPendingFlushAfterCurrentSave = false;
	/** 当前条件不足时，是否需要等组件重新 Ready 后补保存。 */
	bool bPendingFlushWhenReady = false;
};

/**
 * 玩家保存协调子系统。
 * 负责监听背包/装备/快捷栏等运行时变更，把它们折叠成“脏标记 + 去抖自动保存 + 强制刷盘”
 * 的统一策略，避免每个业务模块各自直接落盘。
 */
UCLASS()
class YICHENGAMEPLAY_API UYcSaveCoordinatorSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	/** 通过任意 WorldContext 获取当前世界的保存协调器。 */
	static UYcSaveCoordinatorSubsystem* Get(const UObject* WorldContextObject);

	/** 立即尝试保存指定玩家；必要时可在未脏情况下强制执行。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	bool FlushPlayer(APlayerController* PlayerController, bool bForceIfNotDirty = false, FGameplayTag ReasonTag = FGameplayTag());

	/** 将指定玩家标记为脏，并按配置决定是否安排自动保存。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void MarkPlayerDirty(APlayerController* PlayerController, FGameplayTag ReasonTag = FGameplayTag(), bool bScheduleAutosave = true);

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	bool EnsureMessageListenersRegistered();
	void HandleInventoryStackChanged(FGameplayTag Channel, const FYcInventoryItemChangeMessage& Message);
	void HandleEquipmentSlotChanged(FGameplayTag Channel, const FYcEquipmentSlotChangedMessage& Message);
	void HandleQuickBarSlotsChanged(FGameplayTag Channel, const FYcQuickBarSlotsChangedMessage& Message);
	void HandleProjectedStateChanged(FGameplayTag Channel, const FYcInventoryProjectedStateChangedMessage& Message);
	void HandlePersistenceRequest(FGameplayTag Channel, const FYcPersistenceRequestMessage& Message);
	void HandleProfileHydrated(FGameplayTag Channel, const FYcPersistenceProfileMessage& Message);
	void HandleProfileChanged(FGameplayTag Channel, const FYcPersistenceProfileMessage& Message);

	void HandleProfileLifecycleEvent(const FYcPersistenceProfileMessage& Message);
	bool IsPersistenceManagedChange(APlayerController* PlayerController, const UObject* SourceObject) const;
	bool MatchesRuntimeObject(UYcPlayerPersistenceComponent* PersistenceComponent, const UObject* SourceObject) const;
	UYcPlayerPersistenceComponent* GetPersistenceComponent(APlayerController* PlayerController) const;
	void ScheduleAutosave(FYcSaveCoordinatorPlayerState& PlayerState);
	bool TryFlushPlayerState(FYcSaveCoordinatorPlayerState& PlayerState, bool bForceIfNotDirty);
	void FlushAllTrackedPlayers(bool bForceIfNotDirty);
	void PruneDeadStates();
	FYcSaveCoordinatorPlayerState& GetOrCreateState(APlayerController* PlayerController);

private:
	/** 自动保存去抖时间；在持续小改动期间尽量合并成一次刷盘。 */
	UPROPERTY(EditDefaultsOnly, Category = "Persistence")
	float AutosaveDebounceSeconds = 5.0f;

	/** 当前注册到 GameplayMessageSubsystem 的全部监听句柄。 */
	TArray<FGameplayMessageListenerHandle> ListenerHandles;
	/** GameplayMessageSubsystem 可用后再延迟注册监听，避免世界初始化早于 GameInstance 子系统时崩溃。 */
	bool bMessageListenersRegistered = false;
	/** 当前世界里所有被跟踪玩家的保存状态。 */
	TMap<TWeakObjectPtr<APlayerController>, FYcSaveCoordinatorPlayerState> PlayerStates;
};

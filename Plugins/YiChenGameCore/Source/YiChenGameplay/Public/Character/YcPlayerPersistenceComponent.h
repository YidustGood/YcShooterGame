// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "System/YcAccountTypes.h"
#include "System/YcMetaInventoryTypes.h"
#include "YcPlayerPersistenceComponent.generated.h"

class APlayerController;
class AActor;
class UAsyncAction_ExperienceReady;
class UYcAccountSessionSubsystem;

/** 持久化运行时当前所处阶段，用于区分“身份未就绪/运行时未就绪/已就绪/保存中”等状态。 */
UENUM(BlueprintType)
enum class EYcPlayerPersistenceRuntimeState : uint8
{
	Idle,
	WaitingForIdentity,
	WaitingForRuntime,
	Loading,
	Ready,
	Saving,
	Committing,
	Error
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYcOnPlayerPersistenceReadyChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYcOnPlayerPersistenceHydrated, FYcProfileIdentity, ProfileIdentity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FYcOnPlayerPersistenceDefaultLoadoutRequested);

/**
 * 玩家档案持久化组件。
 * 挂在 PlayerController 上，负责把“账号身份 + 当前场景模式 + 背包/装备/快捷栏运行时”
 * 组织成可装载、可保存、可结算提交的一条完整流程。
 */
UCLASS(ClassGroup = (YiChenGame), Blueprintable, meta = (BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcPlayerPersistenceComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UYcPlayerPersistenceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 主动触发一次流程评估；常用于外部依赖已经准备好时。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void InitializePersistenceFlow();

	/** 基于当前激活 Profile 重新装载运行时内容。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void RefreshFromCurrentProfile();

	/** 与 RefreshFromCurrentProfile 语义一致的别名入口。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void ReloadActiveProfile();

	/** 将局外运行时状态保存回当前 Profile。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	bool SaveCurrentProfile();

	/** 将当前 Profile 的局内负载装载到战斗运行时。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	bool LoadInMatchLoadout();

	/** 将本局战斗结果回写到当前 Profile。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	bool CommitMatchResult(bool bExtractionSucceeded);

	/** 当前是否满足“可以保存局外 Profile”的全部前置条件。 */
	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	bool CanSaveCurrentProfile() const;

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	EYcPlayerPersistenceSceneMode GetCurrentSceneMode() const { return SceneMode; }

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	EYcPlayerPersistenceRuntimeState GetCurrentRuntimeState() const { return RuntimeState; }

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	bool IsPersistenceReady() const { return bIsPersistenceReady; }

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	bool ShouldApplyDefaultLoadout() const { return bDefaultLoadoutRequested; }

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	bool HasEmptyHydratedSnapshot() const { return IsCachedSnapshotEmpty(); }

	/** 默认负载已经被外部应用完毕，通知组件清理请求标记并重建快照。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void NotifyDefaultLoadoutApplied();

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	FYcProfileIdentity GetCurrentProfileIdentity() const { return CurrentProfileIdentity; }

	UFUNCTION(BlueprintPure, Category = "YcGame|Persistence")
	FYcPlayerInventoryRuntime GetCurrentRuntimeHandle() const { return CurrentRuntime; }

	/** 开关整个 Persistence 流程。关闭后会回到 Idle 状态。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void SetPersistenceEnabled(bool bEnabled);

	/** 设置当前处于局内还是局外场景，这会决定使用哪套装载/保存策略。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void SetSceneMode(EYcPlayerPersistenceSceneMode NewMode);

	/** 指定局外 Stash 容器，供局外保存/装载时拼装完整运行时。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	void SetOutOfMatchStashActor(AActor* InStashActor);

	/** Persistence 是否已经进入可正常读写的 Ready 状态。 */
	UPROPERTY(BlueprintAssignable, Category = "YcGame|Persistence")
	FYcOnPlayerPersistenceReadyChanged OnPersistenceReadyChanged;

	/** 当前 Profile 完成 Hydrate 后广播。 */
	UPROPERTY(BlueprintAssignable, Category = "YcGame|Persistence")
	FYcOnPlayerPersistenceHydrated OnPersistenceHydrated;

	/** 当前档案为空时请求外部发放默认局内负载。 */
	UPROPERTY(BlueprintAssignable, Category = "YcGame|Persistence")
	FYcOnPlayerPersistenceDefaultLoadoutRequested OnDefaultLoadoutRequested;

protected:
	/** 组件总开关。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGame|Persistence")
	bool bPersistenceEnabled = true;

	/** 当前场景模式，决定是走局内还是局外持久化分支。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGame|Persistence")
	EYcPlayerPersistenceSceneMode SceneMode = EYcPlayerPersistenceSceneMode::InMatch;

	/** 局外仓库载体；局外存档需要把玩家背包和仓库一起纳入快照。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGame|Persistence")
	TObjectPtr<AActor> OutOfMatchStashActor = nullptr;

private:
	UFUNCTION()
	void HandleExperienceReady();

	UFUNCTION()
	void HandleLocalPlayerIdentityChanged(FYcPlayerIdentitySnapshot PlayerIdentity);

	UFUNCTION(Server, Reliable)
	void ServerRefreshFromCurrentProfile();

	UFUNCTION(Server, Reliable)
	void ServerSaveCurrentProfile();

	UFUNCTION(Server, Reliable)
	void ServerLoadInMatchLoadout();

	UFUNCTION(Server, Reliable)
	void ServerCommitMatchResult(bool bExtractionSucceeded);

	void QueueEvaluation();
	void EvaluateFlow(bool bForceReload = false);
	bool ResolveCurrentProfileIdentity(FYcProfileIdentity& OutProfileIdentity) const;
	FYcPlayerInventoryRuntime BuildRuntimeHandle() const;
	bool CaptureCurrentRuntimeSnapshot();
	bool ReapplyCachedSnapshotToRuntime(const FYcPlayerInventoryRuntime& Runtime);
	bool ExecuteHydrate(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime, bool bForceReload);
	bool ExecuteSaveCurrentProfile();
	bool ExecuteLoadInMatchLoadout();
	bool ExecuteCommitMatchResult(bool bExtractionSucceeded);
	void UpdateReadyState(bool bNewReady);
	void UpdatePostHydrateState(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime);
	void RequestDefaultLoadoutIfNeeded();
	void ClearDefaultLoadoutRequest();
	bool IsCachedSnapshotEmpty() const;
	APlayerController* GetOwningPlayerController() const;
	UYcAccountSessionSubsystem* GetAccountSessionSubsystem() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAsyncAction_ExperienceReady> ExperienceReadyTask = nullptr;

	/** 当前绑定的账号会话子系统，用于监听身份变化。 */
	UPROPERTY(Transient)
	TObjectPtr<UYcAccountSessionSubsystem> BoundAccountSessionSubsystem = nullptr;

	/** 当前识别出的运行时句柄，聚合了角色、背包、装备、快捷栏等对象。 */
	UPROPERTY(Transient)
	FYcPlayerInventoryRuntime CurrentRuntime;

	/** 当前已解析出的激活 Profile 身份。 */
	UPROPERTY(Transient)
	FYcProfileIdentity CurrentProfileIdentity;

	/** 最近一次成功装载的 Profile 身份，用于判断是否需要重新 Hydrate。 */
	UPROPERTY(Transient)
	FYcProfileIdentity LoadedProfileIdentity;

	/** 最近一次从运行时捕获的玩家快照，用于 Pawn 切换后的重放和默认负载判断。 */
	UPROPERTY(Transient)
	FYcMetaPlayerSnapshot CachedPlayerSnapshot;

	/** 当前 Persistence 状态机所处阶段。 */
	UPROPERTY(Transient)
	EYcPlayerPersistenceRuntimeState RuntimeState = EYcPlayerPersistenceRuntimeState::Idle;

	/** 最近一次成功装载时对应的场景模式。 */
	UPROPERTY(Transient)
	EYcPlayerPersistenceSceneMode LoadedSceneMode = EYcPlayerPersistenceSceneMode::Disabled;

	/** 是否有待处理的重新评估请求。 */
	bool bEvaluationQueued = false;
	/** Experience 是否已经 Ready；默认负载发放依赖它。 */
	bool bExperienceReady = false;
	/** 当前是否已经达到可正常读写的 Ready 状态。 */
	bool bIsPersistenceReady = false;
	/** 是否至少成功装载过一次 Profile。 */
	bool bHasLoadedProfile = false;
	/** CachedPlayerSnapshot 是否有效。 */
	bool bHasCachedSnapshot = false;
	/** 是否已经向外部请求过默认负载，避免重复广播。 */
	bool bDefaultLoadoutRequested = false;
};

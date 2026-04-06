// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "YcInventoryOperationTypes.h"
#include "YcInventoryOperationRouterComponent.generated.h"

class UYcInventoryManagerComponent;
class UYcInventoryItemInstance;
class AActor;
class UObject;

/**
 * 预测态变化消息（用于 UI 订阅）。
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryProjectedStateChangedMessage
{
	GENERATED_BODY()

	/** Router 所属 Owner。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	/** 当前权威版本号。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 AuthoritativeRevision = 0;

	/** 当前预测版本号。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 ProjectedRevision = 0;

	/** 当前 Pending 数量。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 PendingCount = 0;

	/** 最新预测态快照。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	FYcInventoryProjectedState ProjectedState;

	/** 触发本次变化的操作。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	FYcInventoryOperation Operation;

	/** 对应事件类型（Submitted / Acked / Nacked）。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	EYcInventoryOperationEvent Event = EYcInventoryOperationEvent::Submitted;
};

UCLASS(ClassGroup=(YiChenInventory), Blueprintable, meta=(BlueprintSpawnableComponent))
class YICHENINVENTORY_API UYcInventoryOperationRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcInventoryOperationRouterComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 查找 Actor 关联的 Router（优先走 Controller 链路）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Inventory)
	static UYcInventoryOperationRouterComponent* FindRouter(const AActor* Actor);

	/** 查找或创建 Router（统一挂在 PlayerController 上）。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	static UYcInventoryOperationRouterComponent* FindOrCreateRouter(AActor* Actor);

	/** 提交统一操作（客户端入队预测，必要时转发到服务端）。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	int64 SubmitInventoryOperation(UYcInventoryManagerComponent* CallingInventory, FYcInventoryOperation InOperation, bool bSendToServer);

	/** 服务端统一入口：执行校验 / 路由 / 回执。 */
	EYcInventoryOperationProcessResult HandleServerSubmit(const FYcInventoryOperation& InOperation, int32& OutRevision, FString& OutDetail);

	/** 客户端向服务端提交统一操作。 */
	UFUNCTION(Server, Reliable)
	void ServerSubmitInventoryOperation(const FYcInventoryOperation& InOperation);

	/** 服务端向客户端下发操作 Ack。 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveInventoryOperationAck(int64 OpId, int32 NewRevision, const FString& DeltaSummary);

	/** 服务端向客户端下发操作 Nack。 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveInventoryOperationNack(int64 OpId, int32 NewRevision, const FString& Reason);

	/** 本地处理 Ack（更新 Revision / Pending 并广播消息）。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void NotifyInventoryOperationAck(int64 OpId, int32 NewRevision, const FString& DeltaSummary);

	/** 本地处理 Nack（更新 Revision / Pending 并重建预测态）。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void NotifyInventoryOperationNack(int64 OpId, int32 NewRevision, const FString& Reason);

	/** 客户端收到服务端 Ack 后的处理入口。 */
	void HandleClientReceiveAck(int64 OpId, int32 NewRevision, const FString& DeltaSummary);

	/** 客户端收到服务端 Nack 后的处理入口。 */
	void HandleClientReceiveNack(int64 OpId, int32 NewRevision, const FString& Reason);

	/** 获取当前权威版本号。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	int32 GetAuthoritativeOperationRevision() const { return AuthoritativeOperationRevision; }

	/** 获取待确认操作数量。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	int32 GetPendingOperationCount() const { return PendingOperations.Num(); }

	/** 获取当前预测版本号。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	int32 GetProjectedOperationRevision() const { return ProjectedOperationRevision; }

	/** 获取可读的操作状态快照。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	FString GetOperationStateSnapshot() const;

	/** 打印操作状态快照。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	void LogOperationStateSnapshot() const;

	/** 注册操作处理器（支持精确 OpType 或前缀匹配）。 */
	bool RegisterOperationHandler(const FName& OpTypeOrPrefix, const FYcInventoryOperationHandler& Handler);

	/** 反注册操作处理器。 */
	bool UnregisterOperationHandler(const FName& OpTypeOrPrefix);

	/** 根据处理器拥有者批量反注册。 */
	bool UnregisterOperationHandlersByOwner(const UObject* HandlerOwner);

	/** 脚本组件注册通用操作处理器（通过函数名反射绑定）。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	bool RegisterScriptOperationHandler(const FName& OpTypeOrPrefix, UObject* HandlerObject, FName ValidateFunctionName, FName ExecuteFunctionName, FName BuildDeltaFunctionName, bool bPrefixMatch, int32 Priority = 0);

	/** 脚本组件反注册通用操作处理器。 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	bool UnregisterScriptOperationHandler(const FName& OpTypeOrPrefix, UObject* HandlerObject);

private:
	/** 注册内置默认处理器。 */
	void RegisterDefaultOperationHandlers();

	/** 根据 OpType 解析处理器（先精确匹配，再前缀匹配）。 */
	const FYcInventoryOperationHandler* ResolveOperationHandler(const FName& OpType) const;

	/** 服务端执行单次操作（校验 + 执行）。 */
	EYcInventoryOperationProcessResult ExecuteOperationOnServer(const FYcInventoryOperation& InOperation, FString& OutReason, FString& OutDeltaSummary);

	/** 执行处理器校验委托。 */
	bool ValidateOperationWithHandler(const FYcInventoryOperation& InOperation, const FYcInventoryOperationHandler& Handler, FString& OutReason) const;

	/** 执行处理器执行/增量构建委托。 */
	bool ExecuteOperationWithHandler(const FYcInventoryOperation& InOperation, const FYcInventoryOperationHandler& Handler, FString& OutReason, FString& OutDeltaSummary);

	/** Nack 后按 Pending 队列重建预测态。 */
	void RebuildProjectedStateFromPending(int64 NackedOpId);

	/** 广播操作状态消息。 */
	void BroadcastOperationStateMessage(EYcInventoryOperationEvent Event, const FYcInventoryOperation& Operation, const FString& Detail, const FYcInventoryOperationDelta* Delta = nullptr);

	/** 广播预测态变化消息。 */
	void BroadcastProjectedStateChanged(EYcInventoryOperationEvent Event, const FYcInventoryOperation& Operation);

	/** 记录单次操作链路日志。 */
	void LogOperationTrace(const TCHAR* Stage, const FYcInventoryOperation& Operation, const FString& Detail) const;

	/** 将一次操作投影到 ProjectedState。 */
	void RefreshProjectedStateFromAuthoritative(const FYcInventoryOperation& Operation);

	/** 权威版本号（由服务端推进）。 */
	UPROPERTY(Transient)
	int32 AuthoritativeOperationRevision = 0;

	/** Pending 操作表（OpId -> Operation）。 */
	UPROPERTY(Transient)
	TMap<int64, FYcInventoryOperation> PendingOperations;

	/** Pending 顺序队列（用于稳定重放/重建）。 */
	UPROPERTY(Transient)
	TArray<int64> PendingOperationOrder;

	/** 本地下一个可分配的操作 ID。 */
	UPROPERTY(Transient)
	int64 NextLocalOperationId = 1;

	/** 预测版本号（权威版本号 + Pending 数量）。 */
	UPROPERTY(Transient)
	int32 ProjectedOperationRevision = 0;

	/** 当前预测态快照。 */
	UPROPERTY(Transient)
	FYcInventoryProjectedState ProjectedState;

	/** 已注册处理器表（Key 为 OpType 或前缀）。 */
	TMap<FName, FYcInventoryOperationHandler> RegisteredOperationHandlers;

	/** 已注册处理器拥有者索引（用于按拥有者批量反注册）。 */
	TMap<FName, TWeakObjectPtr<const UObject>> RegisteredHandlerOwners;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "DynamicScreenUI/YcDynamicScreenUITypes.h"
#include "YcPlayerScreenUIComponent.generated.h"

class APlayerController;
class UCommonActivatableWidget;
class UYcManagedPlayerScreenSlot;

/**
 * 玩家屏幕动态 UI 宿主组件。
 *
 * 它是动态局内 UI 框架的本地执行核心，负责：
 * - 按 WidgetKey 管理玩家屏幕上的动态 Widget 槽位
 * - 监听请求层 GameplayMessage，并进行所属权过滤
 * - 在本地玩家屏幕上执行 show / update / hide
 * - 向具体 Widget 广播表现层消息
 *
 * 业务层不应该直接操作该组件，推荐统一通过通用路由工具发送请求。
 */
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class YICHENGAMEUI_API UYcPlayerScreenUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcPlayerScreenUIComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 本地直接显示一个动态 Widget。通常由路由层消费请求消息时调用。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	void ShowDynamicWidget(const FYcDynamicPlayerScreenUIShowRequest& Request);

	/** 服务器发给目标客户端，在其本地继续走统一请求入队路径。 */
	UFUNCTION(Client, Reliable)
	void ClientShowDynamicWidget(const FYcDynamicPlayerScreenUIShowRequest& Request);

	/** 本地更新指定 WidgetKey 的 Payload。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	void UpdateDynamicWidget(FName WidgetKey, const FInstancedStruct& Payload);

	/** 服务器发给目标客户端，在其本地继续走统一更新请求路径。 */
	UFUNCTION(Client, Reliable)
	void ClientUpdateDynamicWidget(FName WidgetKey, const FInstancedStruct& Payload);

	/** 本地隐藏指定 WidgetKey 的动态 Widget。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI", meta = (AutoCreateRefTerm = "Payload"))
	void HideDynamicWidget(FName WidgetKey, const FInstancedStruct& Payload = FInstancedStruct());

	/** 服务器发给目标客户端，在其本地继续走统一隐藏请求路径。 */
	UFUNCTION(Client, Reliable)
	void ClientHideDynamicWidget(FName WidgetKey, const FInstancedStruct& Payload = FInstancedStruct());

	/** 将显示请求投递到本地请求消息链路。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	void EnqueueShowDynamicWidgetRequest(const FYcDynamicPlayerScreenUIShowRequest& Request);

	/** 将更新请求投递到本地请求消息链路。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	void EnqueueUpdateDynamicWidgetRequest(FName WidgetKey, const FInstancedStruct& Payload);

	/** 将隐藏请求投递到本地请求消息链路。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI", meta = (AutoCreateRefTerm = "Payload"))
	void EnqueueHideDynamicWidgetRequest(FName WidgetKey, const FInstancedStruct& Payload = FInstancedStruct());

	/** 当槽位成功完成 Widget Push 后，由槽位对象回调到组件。 */
	void HandleScreenSlotWidgetPushed(FName WidgetKey, UCommonActivatableWidget* UserWidget);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ClientShowDynamicWidget_Implementation(const FYcDynamicPlayerScreenUIShowRequest& Request);
	void ClientUpdateDynamicWidget_Implementation(FName WidgetKey, const FInstancedStruct& Payload);
	void ClientHideDynamicWidget_Implementation(FName WidgetKey, const FInstancedStruct& Payload);

	void OnDynamicShowRequestReceived(FGameplayTag Channel, const FYcDynamicPlayerScreenUIShowRequest& Request);
	void OnDynamicUpdateRequestReceived(FGameplayTag Channel, const FYcDynamicPlayerScreenUIRouteMessage& Request);
	void OnDynamicHideRequestReceived(FGameplayTag Channel, const FYcDynamicPlayerScreenUIRouteMessage& Request);

	void BroadcastDynamicWidgetUpdate(FName WidgetKey, const FInstancedStruct& Payload);
	void BroadcastDynamicWidgetHide(FName WidgetKey, const FInstancedStruct& Payload);

	UYcManagedPlayerScreenSlot* ResolveScreenSlot(FName WidgetKey);
	void RegisterRouteListeners();
	void UnregisterRouteListeners();
	bool ShouldProcessRouteRequest(const APlayerController* TargetPlayerController, FName WidgetKey) const;
	APlayerController* GetOwningPlayerController() const;

private:
	/** 当前玩家所有动态 UI 槽位的运行时表。 */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UYcManagedPlayerScreenSlot>> ScreenSlots;

	/** 显示请求消息监听句柄。 */
	FGameplayMessageListenerHandle DynamicShowRequestHandle;

	/** 更新请求消息监听句柄。 */
	FGameplayMessageListenerHandle DynamicUpdateRequestHandle;

	/** 隐藏请求消息监听句柄。 */
	FGameplayMessageListenerHandle DynamicHideRequestHandle;

	/** 当前是否已经向 GameplayMessageSubsystem 注册过请求监听。 */
	bool bRouteListenersRegistered = false;
};

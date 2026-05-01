// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "YcManagedPlayerScreenSlot.generated.h"

class APlayerController;
class UAsyncAction_PushContentToLayerForPlayer;
class UCommonActivatableWidget;
class UYcPlayerScreenUIComponent;
struct FGameplayTag;

/**
 * 单个动态局内 UI 槽位的运行时宿主对象。
 *
 * 它负责：
 * - 跟踪某个 WidgetKey 当前是否已有激活中的 Widget
 * - 处理异步 Push 行为
 * - 缓存该槽位当前最新的 Payload
 *
 * 该类属于框架内部运行时对象，业务层通常不需要直接依赖。
 */
UCLASS()
class YICHENGAMEUI_API UYcManagedPlayerScreenSlot : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化槽位运行时信息。 */
	void InitializeSlot(UYcPlayerScreenUIComponent* InOwningScreenUIComponent, APlayerController* InOwningPlayerController, FName InWidgetKey);

	/** 异步显示并推送 Widget。 */
	void ShowWidget(TSoftClassPtr<UCommonActivatableWidget> InWidgetClass, FGameplayTag InWidgetLayer, bool bInSuspendInputUntilWidgetPushed);

	/** 隐藏当前 Widget，并取消可能尚未完成的 Push。 */
	void HideWidget();

	/** 当前槽位是否已经持有激活中的 Widget。 */
	bool HasActiveWidget() const;

	/** 设置当前缓存 Payload。 */
	void SetCurrentPayload(const FInstancedStruct& InPayload);

	/** 获取当前缓存 Payload。 */
	const FInstancedStruct& GetCurrentPayload() const;

private:
	/** Widget 推送完成后的回调。 */
	UFUNCTION()
	void HandleWidgetPushed(UCommonActivatableWidget* UserWidget);

private:
	/** 正在进行中的异步 Push 任务。 */
	UPROPERTY(Transient)
	TObjectPtr<UAsyncAction_PushContentToLayerForPlayer> PendingPushAction = nullptr;

	/** 当前槽位上激活中的 Widget。 */
	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidget> ActiveWidget = nullptr;

	/** 槽位所属玩家控制器。 */
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> OwningPlayerController = nullptr;

	/** 槽位所属屏幕 UI 组件。 */
	UPROPERTY(Transient)
	TObjectPtr<UYcPlayerScreenUIComponent> OwningScreenUIComponent = nullptr;

	/** 该槽位绑定的 WidgetKey。 */
	UPROPERTY(Transient)
	FName WidgetKey = NAME_None;

	/** 当前最新的业务载荷。 */
	UPROPERTY(Transient)
	FInstancedStruct CurrentPayload;
};

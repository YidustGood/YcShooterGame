// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "YcActivatableWidget.h"
#include "DynamicScreenUI/YcDynamicScreenUITypes.h"
#include "YcDynamicScreenWidgetBase.generated.h"

/**
 * 动态局内 Widget 的通用基类。
 *
 * 该基类负责：
 * - 记录自身所属的 WidgetKey
 * - 监听动态 UI 框架的通用更新/隐藏消息
 * - 缓存当前 Payload
 * - 将初始化、更新、隐藏这三个时机统一暴露给业务 Widget
 *
 * 业务层只需要继承它并解析自己的 Payload 即可。
 */
UCLASS(Abstract, Blueprintable)
class YICHENGAMEUI_API UYcDynamicScreenWidgetBase : public UYcActivatableWidget
{
	GENERATED_BODY()

public:
	/** 初始化动态 Widget 的槽位标识，并注册框架消息监听。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	void InitializeDynamicScreenWidget(FName InWidgetKey);

	/** 获取当前动态 Widget 对应的 WidgetKey。 */
	UFUNCTION(BlueprintPure, Category = "Dynamic Screen UI")
	FName GetDynamicScreenWidgetKey() const;

	/**
	 * 获取当前缓存的 Payload。
	 * 由于 UFUNCTION 无法直接返回 FInstancedStruct 引用，这里通过输出参数返回一份拷贝。
	 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	void GetDynamicScreenWidgetPayload(FInstancedStruct& OutPayload) const;

	/** 获取当前缓存 Payload 的只读引用，供 C++ 或支持引用访问的调用方减少拷贝开销。 */
	const FInstancedStruct& GetDynamicScreenWidgetPayloadRef() const;

protected:
	virtual void NativeDestruct() override;

	/** 动态 Widget 完成框架初始化后的回调。 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dynamic Screen UI")
	void OnDynamicScreenWidgetInitialized();

	/** 当前 Payload 被更新后的回调。 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dynamic Screen UI")
	void OnDynamicScreenWidgetPayloadUpdated();

	/** 当前 Widget 收到隐藏消息后的回调。 */
	UFUNCTION(BlueprintNativeEvent, Category = "Dynamic Screen UI")
	void OnDynamicScreenWidgetPayloadHidden();

private:
	void RegisterDynamicMessageListeners();
	void UnregisterDynamicMessageListeners();

	void OnDynamicScreenWidgetUpdated(FGameplayTag Channel, const FYcDynamicPlayerScreenUIMessage& Data);
	void OnDynamicScreenWidgetHidden(FGameplayTag Channel, const FYcDynamicPlayerScreenUIMessage& Data);

private:
	/** 当前动态 Widget 所属的稳定槽位标识。 */
	UPROPERTY(Transient)
	FName DynamicWidgetKey = NAME_None;

	/** 当前最新缓存的业务 Payload。 */
	UPROPERTY(Transient)
	FInstancedStruct CurrentPayload;

	/** 更新消息监听句柄。 */
	FGameplayMessageListenerHandle DynamicUpdateHandle;

	/** 隐藏消息监听句柄。 */
	FGameplayMessageListenerHandle DynamicHideHandle;

	/** 当前是否已完成框架消息监听注册。 */
	bool bDynamicListenersRegistered = false;
};

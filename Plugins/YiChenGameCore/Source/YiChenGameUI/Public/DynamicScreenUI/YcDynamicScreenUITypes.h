// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YcDynamicScreenUITypes.generated.h"

class APlayerController;
class UCommonActivatableWidget;

/**
 * 动态局内 UI 的显示请求。
 *
 * 该结构用于描述“把哪个动态 Widget 以什么初始数据推送到哪个玩家屏幕上”。
 * 业务层通常只需要构造此请求，再交给通用路由层发送即可。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEUI_API FYcDynamicPlayerScreenUIShowRequest
{
	GENERATED_BODY()

	FYcDynamicPlayerScreenUIShowRequest()
		: WidgetLayer(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game"), false))
	{
	}

	/** 目标玩家控制器。业务层可不主动填写，路由层会在本地请求路径中补全。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	TObjectPtr<APlayerController> TargetPlayerController = nullptr;

	/** 动态 UI 槽位标识。相同 UI 的 show / update / hide 必须使用同一个 Key。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FName WidgetKey = NAME_None;

	/** 要动态推送的 Widget 类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	TSoftClassPtr<UCommonActivatableWidget> WidgetClass;

	/** Widget 要推送到的 UI Layer。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FGameplayTag WidgetLayer;

	/** 推送过程中是否暂时挂起输入，直到 Widget 完成入栈。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	bool bSuspendInputUntilWidgetPushed = false;

	/** Widget 首次显示时使用的初始载荷数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FInstancedStruct InitialPayload;
};

/**
 * 动态局内 UI 的通用路由消息。
 *
 * 该结构用于 update / hide 等只需要“目标玩家 + WidgetKey + Payload”的请求场景。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEUI_API FYcDynamicPlayerScreenUIRouteMessage
{
	GENERATED_BODY()

	/** 目标玩家控制器。只有命中本地归属的组件才会消费该消息。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	TObjectPtr<APlayerController> TargetPlayerController = nullptr;

	/** 动态 UI 槽位标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FName WidgetKey = NAME_None;

	/** 这次路由附带的数据载荷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FInstancedStruct Payload;
};

/**
 * 动态局内 UI 的表现层消息。
 *
 * 这是路由层在本地真正执行 show / update / hide 后，向具体动态 Widget 广播的统一消息格式。
 * Widget 只需要关心自己的 WidgetKey 与 Payload，不需要理解网络与目标玩家路由逻辑。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEUI_API FYcDynamicPlayerScreenUIMessage
{
	GENERATED_BODY()

	/** 动态 UI 槽位标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FName WidgetKey = NAME_None;

	/** 当前广播附带的数据载荷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Screen UI")
	FInstancedStruct Payload;
};

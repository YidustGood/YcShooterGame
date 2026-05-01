// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DynamicScreenUI/YcDynamicScreenUITypes.h"
#include "YcDynamicScreenUIRouting.generated.h"

class APlayerController;

/**
 * 动态局内 UI 的通用路由函数库。
 *
 * 这是业务层推荐的唯一框架入口，负责：
 * - 将 UI 请求发送给单个玩家
 * - 将 UI 请求发送给多个玩家
 * - 将 UI 请求发送给当前局内的全部玩家
 * - 统一处理本地玩家与服务器远端玩家的分发差异
 *
 * 业务层应当只表达“把哪个 UI 发给谁”，而不需要直接感知屏幕组件与 RPC 细节。
 */
UCLASS()
class YICHENGAMEUI_API UYcDynamicScreenUIRoutingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 向单个玩家路由一个显示请求。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void RouteShowToPlayer(APlayerController* TargetPlayerController, const FYcDynamicPlayerScreenUIShowRequest& Request);

	/** 向单个玩家路由一次数据更新请求。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void RouteUpdateToPlayer(APlayerController* TargetPlayerController, FName WidgetKey, const FInstancedStruct& Payload);

	/** 向单个玩家路由一次隐藏请求。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI", meta = (AutoCreateRefTerm = "Payload"))
	static void RouteHideToPlayer(APlayerController* TargetPlayerController, FName WidgetKey, const FInstancedStruct& Payload = FInstancedStruct());

	/** 向多个玩家路由一个显示请求。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void RouteShowToPlayers(const TArray<APlayerController*>& InTargetPlayerControllers, const FYcDynamicPlayerScreenUIShowRequest& Request);

	/** 向多个玩家路由一次数据更新请求。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void RouteUpdateToPlayers(const TArray<APlayerController*>& InTargetPlayerControllers, FName WidgetKey, const FInstancedStruct& Payload);

	/** 向多个玩家路由一次隐藏请求。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI", meta = (AutoCreateRefTerm = "Payload"))
	static void RouteHideToPlayers(const TArray<APlayerController*>& InTargetPlayerControllers, FName WidgetKey, const FInstancedStruct& Payload = FInstancedStruct());

	/** 向当前局内全部玩家路由一个显示请求。默认不包含 Bot。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void RouteShowToAllPlayers(const FYcDynamicPlayerScreenUIShowRequest& Request, bool bIncludeBots = false);

	/** 向当前局内全部玩家路由一次数据更新请求。默认不包含 Bot。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void RouteUpdateToAllPlayers(FName WidgetKey, const FInstancedStruct& Payload, bool bIncludeBots = false);

	/** 向当前局内全部玩家路由一次隐藏请求。默认不包含 Bot。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI", meta = (AutoCreateRefTerm = "Payload"))
	static void RouteHideToAllPlayers(FName WidgetKey, const FInstancedStruct& Payload = FInstancedStruct(), bool bIncludeBots = false);

	/** 收集当前局内可用于动态 UI 路由的玩家控制器列表。 */
	UFUNCTION(BlueprintCallable, Category = "Dynamic Screen UI")
	static void ResolveMatchPlayerControllers(TArray<APlayerController*>& OutPlayerControllers, bool bIncludeBots = false);
};

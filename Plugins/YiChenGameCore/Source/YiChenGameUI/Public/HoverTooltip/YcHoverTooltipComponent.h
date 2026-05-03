// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "StructUtils/InstancedStruct.h"
#include "HoverTooltip/YcHoverTooltipTypes.h"
#include "YcHoverTooltipComponent.generated.h"

class APlayerController;
class UYcDynamicScreenWidgetBase;

/**
 * Hover Tooltip 在单个本地玩家屏幕上的运行时状态。
 *
 * 该状态块与组件实现一起形成纯本地的悬停提示子系统，
 * 未来若需要继续扩展为更复杂的 controller，也可以整体平移。
 */
USTRUCT()
struct FYcHoverTooltipRuntimeState
{
	GENERATED_BODY()

	TWeakObjectPtr<UObject> ActiveSource;
	FYcHoverTooltipRequest ActiveRequest;
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	FTimerHandle DelayTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UYcDynamicScreenWidgetBase> ActiveWidget = nullptr;

	bool bVisible = false;

	void ResetSessionData()
	{
		ActiveSource.Reset();
		ActiveRequest = FYcHoverTooltipRequest();
		ScreenPosition = FVector2D::ZeroVector;
		bVisible = false;
	}
};

/**
 * 本地玩家屏幕上的 Hover Tooltip 组件。
 *
 * 它只负责本地悬停提示会话，不参与服务端动态 UI 路由。
 */
UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class YICHENGAMEUI_API UYcHoverTooltipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcHoverTooltipComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	void BeginHoverTooltip(UObject* SourceObject, const FYcHoverTooltipRequest& Request, FVector2D InitialScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	void UpdateHoverTooltip(UObject* SourceObject, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	void EndHoverTooltip(UObject* SourceObject);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	void CancelHoverTooltip();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	APlayerController* GetOwningPlayerController() const;
	bool CanUseHoverTooltip() const;
	bool IsMouseAndKeyboardInputActive() const;
	void ShowHoverTooltipNow();
	void ShowOrUpdateHoverTooltipWidget(const FInstancedStruct& Payload);
	void DestroyHoverTooltipWidget();
	void ClearHoverTooltipState(bool bHideWidget);
	void BroadcastHoverTooltipUpdate(FName WidgetKey, const FInstancedStruct& Payload) const;
	void BroadcastHoverTooltipHide(FName WidgetKey, const FInstancedStruct& Payload) const;
	FYcHoverTooltipWidgetPayload BuildHoverTooltipWidgetPayload() const;

private:
	UPROPERTY(Transient)
	FYcHoverTooltipRuntimeState HoverTooltipState;
};

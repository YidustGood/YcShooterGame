// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcActivatableWidget.h"
#include "YcGameHUDLayout.generated.h"

/**
 *	用于布局玩家HUD的Widget(在项目种一般是在Experience中配置AddWidgetsAction的GameFeatureAction来动态添加给玩家)
 */
UCLASS()
class YICHENGAMEUI_API UYcGameHUDLayout : public UYcActivatableWidget
{
	GENERATED_BODY()
public:

	UYcGameHUDLayout(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;

protected:
	// 处理ESC返回动作(实际上就是处理ESC按下现实ESC菜单)
	void HandleEscapeAction() const;

	// ESC返回菜单类
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;
};
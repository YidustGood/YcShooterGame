// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "YcActivatableWidget.generated.h"

struct FUIInputConfig;

/** UI响应的输入模式 */
UENUM(BlueprintType)
enum class EYcWidgetInputMode : uint8
{
	Default,		// 默认
	GameAndMenu,	// 游戏+UI菜单
	Game,			// 仅游戏
	Menu			// 仅UI菜单
};

/**
 * 可激活的控件, 在激活时自动驱动所需的输入配置
 */
UCLASS(Abstract, Blueprintable)
class YICHENGAMEUI_API UYcActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UYcActivatableWidget(const FObjectInitializer& ObjectInitializer);
	
	//~UCommonActivatableWidget interface
	/** 根据下面的InputConfig属性返回UI输入配置, 以控制UI对于输入的处理逻辑 */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~End of UCommonActivatableWidget interface

#if WITH_EDITOR
	virtual void ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const override;
#endif
	
protected:
	/** 该UI激活时所需的输入模式,例如是否希望按键事件仍能传递到游戏/玩家控制器 */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EYcWidgetInputMode InputConfig = EYcWidgetInputMode::Default;

	/** 游戏获得输入时期望的鼠标行为 */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};

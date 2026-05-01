// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActionWidget.h"
#include "YcActionWidget.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;

/**
 * 扩展版动作控件。
 * 
 * 会优先读取与当前 Common Input 动作关联的 Enhanced Input Action，
 * 并根据玩家当前实际绑定的按键动态显示对应图标。
 */
UCLASS(BlueprintType, Blueprintable)
class YICHENGAMEUI_API UYcActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()
public:
	/** 获取当前动作应显示的输入图标。 */
	virtual FSlateBrush GetIcon() const override;

	/** 与当前 Common Input 动作关联的 Enhanced Input Action。 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	const TObjectPtr<UInputAction> AssociatedInputAction;

private:
	/** 获取当前本地玩家的 Enhanced Input 子系统。 */
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
};

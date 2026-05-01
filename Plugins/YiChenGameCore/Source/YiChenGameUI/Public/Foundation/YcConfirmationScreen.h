// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Messaging/CommonGameDialog.h"
#include "YcConfirmationScreen.generated.h"

class IWidgetCompilerLog;
class UCommonTextBlock;
class UCommonRichTextBlock;
class UDynamicEntryBox;
class UCommonBorder;

/**
 * 确认对话窗口 UI 基类。
 * 
 * 负责根据对话描述构建标题、正文与操作按钮，
 * 并在用户做出选择后回调对应的消息结果。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class YICHENGAMEUI_API UYcConfirmationScreen : public UCommonGameDialog
{
	GENERATED_BODY()
public:
	/** 根据描述对象初始化确认对话框内容与按钮。 */
	virtual void SetupDialog(UCommonGameDialogDescriptor* Descriptor, FCommonMessagingResultDelegate ResultCallback) override;
	/** 销毁当前对话框。 */
	virtual void KillDialog() override;

protected:
	/** 控件初始化时绑定点击空白区域关闭事件。 */
	virtual void NativeOnInitialized() override;
	/** 关闭确认窗口并返回指定结果。 */
	virtual void CloseConfirmationWindow(ECommonMessagingResult Result);

#if WITH_EDITOR
	/** 编译期校验必须的默认配置是否完整。 */
	virtual void ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const override;
#endif

private:
	/** 处理点击空白关闭区域的输入事件。 */
	UFUNCTION()
	FEventReply HandleTapToCloseZoneMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	/** 对话框关闭后的结果回调。 */
	FCommonMessagingResultDelegate OnResultCallback;

private:
	/** 对话框标题文本。 */
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> Text_Title;

	/** 对话框正文富文本。 */
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> RichText_Description;

	/** 按钮条目容器。 */
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UDynamicEntryBox> EntryBox_Buttons;

	/** 点击后可关闭对话框的空白区域。 */
	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UCommonBorder> Border_TapToCloseZone;

	/** 取消操作对应的输入动作配置。 */
	UPROPERTY(EditDefaultsOnly, meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle CancelAction;
};


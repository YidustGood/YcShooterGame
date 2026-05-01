// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "YcUIMessaging.generated.h"

class FSubsystemCollectionBase;
class UCommonGameDialog;
class UCommonGameDialogDescriptor;
class UObject;

/**
 * UI 消息子系统。
 * 
 * 负责加载项目内配置的确认框与错误框类，
 * 并将消息对话框推入主 UI 布局的模态层中显示。
 */
UCLASS()
class YICHENGAMEUI_API UYcUIMessaging : public UCommonMessagingSubsystem
{
	GENERATED_BODY()
public:
	UYcUIMessaging() { }

	/** 初始化并同步加载消息对话框类。 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 显示确认对话框。 */
	virtual void ShowConfirmation(UCommonGameDialogDescriptor* DialogDescriptor, FCommonMessagingResultDelegate ResultCallback = FCommonMessagingResultDelegate()) override;
	/** 显示错误对话框。 */
	virtual void ShowError(UCommonGameDialogDescriptor* DialogDescriptor, FCommonMessagingResultDelegate ResultCallback = FCommonMessagingResultDelegate()) override;

private:
	/** 已同步加载的确认对话框类。 */
	UPROPERTY()
	TSubclassOf<UCommonGameDialog> ConfirmationDialogClassPtr;

	/** 已同步加载的错误对话框类。 */
	UPROPERTY()
	TSubclassOf<UCommonGameDialog> ErrorDialogClassPtr;

	/** 配置项中的确认对话框软引用。 */
	UPROPERTY(config)
	TSoftClassPtr<UCommonGameDialog> ConfirmationDialogClass;

	/** 配置项中的错误对话框软引用。 */
	UPROPERTY(config)
	TSoftClassPtr<UCommonGameDialog> ErrorDialogClass;
};

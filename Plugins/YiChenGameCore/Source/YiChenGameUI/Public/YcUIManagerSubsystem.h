// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameUIManagerSubsystem.h"
#include "YcUIManagerSubsystem.generated.h"

/**
 * 插件提供的GameUIManagerSubsystem基类
 * 注意: 依赖于UCommonGameInstance::AddLocalPlayer()函数调用UGameUIManagerSubsystem::NotifyPlayerAdded来触发LayoutUI的创建
 * 使用本插件的时候直接使用YiChenGameplay模块的YcGameInstance及子类。
 */
UCLASS()
class YICHENGAMEUI_API UYcUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()
public:
	UYcUIManagerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	bool Tick(float DeltaTime);
	// 同步Layout可视性到HUD
	void SyncRootLayoutVisibilityToShowHUD();
	
	FTSTicker::FDelegateHandle TickHandle;
};

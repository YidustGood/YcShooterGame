// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"
#include "YcGameHUD.generated.h"

/**
 *  注意：通常你不需要扩展或修改这个类，而是在你的体验中使用"添加控件"操作来添加HUD布局和控件
 *  这个类主要存在于调试渲染
 */
UCLASS()
class YICHENGAMEUI_API AYcGameHUD : public AHUD
{
	GENERATED_BODY()
public:
	AYcGameHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UObject interface
	virtual void PreInitializeComponents() override;
	//~End of UObject interface

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AHUD interface
	virtual void GetDebugActorList(TArray<AActor*>& InOutList) override;
	//~End of AHUD interface
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CommonPlayerController.h"
#include "YcPlayerController.generated.h"

class AYcPlayerState;
class UYcAbilitySystemComponent;

/**
 * 插件Gameplay框架提供的玩家控制器基类, 与插件其它模块系统协同工作
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class YICHENGAMEPLAY_API AYcPlayerController : public ACommonPlayerController
{
	GENERATED_BODY()
public:
	AYcPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerController")
	AYcPlayerState* GetYcPlayerState() const;
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerController")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const;
	
	//~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface
};
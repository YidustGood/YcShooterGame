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
	
	/**
	 * 获取当前控制器对应的 YcPlayerState
	 * @return 转换后的 AYcPlayerState 指针，如果当前 PlayerState 不是 AYcPlayerState 类型则返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerController")
	AYcPlayerState* GetYcPlayerState() const;
	
	/**
	 * 获取当前控制器对应的 YcAbilitySystemComponent
	 * @return YcAbilitySystemComponent 指针，如果未找到则返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerController")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const;
	
	//~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface
	
	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~End of AController interface
	
	////////// Cheat相关 //////////
	/** 在服务器上执行一条作弊命令（仅作用于当前玩家） */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheat(const FString& Msg);

	/** 在服务器上执行一条作弊命令（作用于所有玩家） */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheatAll(const FString& Msg);
	////////// ~Cheat相关 //////////
};
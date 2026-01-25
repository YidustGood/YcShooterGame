// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularCharacter.h"
#include "YcCharacter.generated.h"

class AYcPlayerController;
class AYcPlayerState;
class UYcAbilitySystemComponent;
class UYcPawnExtensionComponent;
class UYcHealthComponent;

/**
 * 本插件使用的基础角色Pawn类
 * 负责向Pawn组件派发事件, 新的功能行为应尽可能通过Pawn组件进行添加, 可参考Hero组件
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class YICHENGAMEPLAY_API AYcCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AYcCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// 获取YcPlayerController
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Character")
	AYcPlayerController* GetYcPlayerController() const;

	// 获取YcPlayerState
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Character")
	AYcPlayerState* GetYcPlayerState() const;
	
	// 获取YcAbilitySystemComponent
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Character")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const;
	// 获取AbilitySystemComponent接口实现
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	// 当能力系统初始化时调用
	virtual void OnAbilitySystemInitialized();
	// 当能力系统反初始化时调用
	virtual void OnAbilitySystemUninitialized();
	
	// 当被控制器占有时调用
	virtual void PossessedBy(AController* NewController) override;
	// 当失去控制器占有时调用
	virtual void UnPossessed() override;

	// 控制器复制回调
	virtual void OnRep_Controller() override;
	// 玩家状态复制回调
	virtual void OnRep_PlayerState() override;
	
	// 设置玩家输入组件
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	//// 处理玩家生命周期相关的 ////
	
	// 开始角色死亡序列（禁用碰撞、禁用移动等）
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// 结束角色死亡序列（分离控制器、销毁Pawn等）
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);
	
	// 死亡事件完成后调用，销毁角色
	void DestroyDueToDeath();
	
	// 反初始化并销毁角色
	void UninitAndDestroy();

	// 蓝图实现-当角色死亡开始时调用
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathStarted"))
	void K2_OnDeathStarted();

	// Called when the death sequence for the character has completed，当角色的死亡序列完成时调用
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();
	
	// 关闭移动和碰撞
	UFUNCTION(BlueprintCallable)
	virtual void DisableMovementAndCollision();
	
	//// ~处理玩家生命周期相关的 ////
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcPawnExtensionComponent> PawnExtComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcHealthComponent> HealthComponent;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularCharacter.h"
#include "YcTeamAgentInterface.h"
#include "YcCharacter.generated.h"

class AYcPlayerController;
class AYcPlayerState;
class UYcAbilitySystemComponent;
class UYcPawnExtensionComponent;
class UYcHealthComponent;
class UYcMovementSyncComponent;

/**
 * 本插件使用的基础角色Pawn类
 * 
 * 功能说明：
 * - 负责向Pawn组件派发事件
 * - 实现能力系统接口（IAbilitySystemInterface）
 * - 实现队伍系统接口（IYcTeamAgentInterface）
 * - 新的功能行为应尽可能通过Pawn组件进行添加
 * 
 * 队伍系统说明：
 * - Character 实现 IYcTeamAgentInterface 接口
 * - 队伍信息从 Controller 的 PlayerState 中获取
 * - 这样 AI 感知系统可以直接从 Character 判断敌友关系
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class YICHENGAMEPLAY_API AYcCharacter : public AModularCharacter, public IAbilitySystemInterface, public IYcTeamAgentInterface
{
	GENERATED_BODY()
public:
	AYcCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~AActor interface
	virtual void Reset() override;
	//~End of AActor interface
	
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

	//~IYcTeamAgentInterface interface
	/**
	 * 设置队伍ID（Character 不允许直接设置）
	 * 注意：Character 的队伍ID由其 Controller 的 PlayerState 决定
	 * @param NewTeamID 新的队伍ID
	 */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	/**
	 * 获取队伍ID
	 * 性能优化：直接从 Controller 的 PlayerState 获取，避免使用 FindTeamAgentFromActor
	 * 这是高频调用的接口（AI 感知系统每帧都会调用）
	 * @return 当前队伍ID，如果没有找到则返回 NoTeam
	 */
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	/**
	 * 获取队伍变更委托
	 * 从 Controller 的 PlayerState 获取
	 * @return 队伍变更委托指针
	 */
	virtual FOnYcTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	
	/**
	 * 获取对其他 Actor 的队伍态度
	 * 性能优化：直接比较队伍ID，避免复杂的查找逻辑
	 * @param Other 目标Actor
	 * @return 队伍态度（友好/敌对/中立）
	 */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of IYcTeamAgentInterface interface
	
protected:
	virtual void BeginPlay() override;
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
	
public:
	//// 输入响应 ////
	// UYcHeroComponent 仅负责读取输入并做设备归一化, 然后把"移动/视角/下蹲的意图"转发到这里,
	// 由具体的角色决定如何响应(第一人称/第三人称/横板等), 从而让输入框架与玩法解耦。
	// 这些是 BlueprintNativeEvent, AngelScript 或蓝图子类可直接 BlueprintOverride 覆写来定制玩法。

	/**
	 * 处理移动输入。默认按控制器朝向做第三人称式移动。
	 * @param MovementVector 移动输入值, X=右, Y=前, 范围约[-1,1]
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "YcGameCore|Input")
	void HandleMoveInput(FVector2D MovementVector);
	virtual void HandleMoveInput_Implementation(FVector2D MovementVector);

	/**
	 * 处理视角输入。输入层已完成设备归一化(鼠标为帧增量, 摇杆按速率*DeltaTime换算成帧增量),
	 * 因此这里拿到的是"本帧应施加的偏航/俯仰增量", 无需再关心输入设备差异。
	 * @param LookDelta 本帧视角增量, X=偏航(Yaw), Y=俯仰(Pitch)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "YcGameCore|Input")
	void HandleLookInput(FVector2D LookDelta);
	virtual void HandleLookInput_Implementation(FVector2D LookDelta);

	/**
	 * 处理下蹲输入。默认无行为, 由子类根据玩法(切换/长按下蹲)覆写。
	 * @param bIsPressed 下蹲输入是否触发
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "YcGameCore|Input")
	void HandleCrouchInput(bool bIsPressed);
	virtual void HandleCrouchInput_Implementation(bool bIsPressed);
	//// ~输入响应 ////

protected:
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
	/** Pawn扩展功能维护组件, 协调各功能组件初始化 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcPawnExtensionComponent> PawnExtComponent;
	
	/** 健康组件, 让角色有生命值和生死的能力 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcHealthComponent> HealthComponent;
	
	/** 移动同步组件, 用于同步GE对移动功能的效果修改 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcMovementSyncComponent> MovementSyncComponent;
};

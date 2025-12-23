// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/YcGameplayAbility.h"
#include "YcGamePhaseAbility.generated.h"

/**
 * 游戏阶段技能基类
 * 用于改变活动游戏阶段的任何能力的基本玩法能力。
 * 通过游戏阶段技能+UYcGamePhaseSubsystem来控制游戏对局中的阶段, 每个阶段激活时可以做一些自定义的逻辑。
 * 本基类会在ActivateAbility、EndAbility的时候在权威端UYcGamePhaseSubsystem的OnBeginPhase和OnEndPhase
 * 主要是为了注册保存游戏阶段技能数据, 并通知这个游戏阶段开始/结束事件的观察者们
 * 
 * 游戏阶段案例：
 * 预热阶段：玩家在角色选择界面, 选择角色装备准备开始游戏对局
 * 游戏战斗阶段：正式的游戏对局阶段
 * 对局结束阶段：游戏对局结束
 */
UCLASS(Abstract, HideCategories = Input)
class YICHENGAMEPHASE_API UYcGamePhaseAbility : public UYcGameplayAbility
{
	GENERATED_BODY()

public:
	UYcGamePhaseAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
#if WITH_EDITOR
	/**
	 * 在编辑器中验证该游戏阶段能力的数据是否合法
	 * @param Context 数据验证上下文，用于收集错误和警告信息
	 * @return 数据验证结果（Valid/Invalid/NotValidated）
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	
	/**
	 * 获取该游戏阶段能力对应的阶段标签
	 * @return 表示当前游戏阶段的GameplayTag
	 */
	const FGameplayTag& GetGamePhaseTag() const { return GamePhaseTag; }

protected:
	/** 激活游戏阶段能力时调用，并在权威端通知游戏阶段子系统开始该阶段 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	/** 结束游戏阶段能力时调用，并在权威端通知游戏阶段子系统结束该阶段 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 定义该游戏阶段能力所属的游戏阶段标签。
	// 例如：如果游戏阶段为 GamePhase.RoundStart，则会结束所有处于同一层级的兄弟阶段；
	// 如果当前有 GamePhase.WaitingToStart 处于激活状态，激活 RoundStart 阶段将结束 WaitingToStart。
	// 为了支持嵌套阶段，可以使用层级化的标签：例如 GamePhase.Playing.NormalPlay 是 GamePhase.Playing 的子阶段，
	// 将其切换为 GamePhase.Playing.SuddenDeath 时，会结束所有绑定到 GamePhase.Playing.* 的能力，
	// 但不会结束仅绑定到 GamePhase.Playing 阶段的能力。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Game Phase")
	FGameplayTag GamePhaseTag;
};
// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "YcGamePhaseSubsystem.h"
#include "Components/GameStateComponent.h"
#include "YcGamePhaseComponent.generated.h"

struct FExperienceLoadedMessage;
struct FYcGamePhaseDefinition;
struct FGamePhaseMessage;
class UYcGamePhaseAbility;

/**
 * 游戏阶段定义结构
 * 用于描述一个游戏阶段的配置，包括阶段标签以及对应的阶段能力类
 * 典型用法：在 UYcGamePhaseComponent 中按顺序配置一组阶段定义，并据此驱动阶段切换
 */
USTRUCT(BlueprintType)
struct FYcGamePhaseDefinition
{
	GENERATED_BODY()
	
	/** 表示该阶段的唯一标签（用于消息通道与子系统管理） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag GamePhaseTag;
	
	/** 实现该阶段逻辑的游戏阶段能力类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UYcGamePhaseAbility> PhaseAbilityClass;
};

/**
 * 
 * 游戏阶段功能组件基类, 用于控制游戏阶段
 * 可以通过继承该组件, 实现更多自定义的阶段管理规则
 * UYcGamePhaseSubsystem没有做网络复制的功能, 他目前只工作在服务器上, 主要职责是提供一些全局的游戏阶段操作函数, 并记录一些相关的信息数据
 * UYcGamePhaseComponent的话就是用于制定具体的游戏阶段切换逻辑, 并且提供网络复制功能同步需要被所有客户端关注到的游戏阶段信息
 * 例如：
 * 1、添加比分机制, 当比分到达胜利目标后就激活游戏结束阶段
 * 2、添加时间机制, 当时间结束后就结束对局
 * 总结来说就是在这个组件通过记录某个游戏对局需要达成的目标, 达成后就进行游戏阶段的切换轮转
 * StartNextPhase()和广播PhaseMessage都可以实现触发阶段变更, 既可以在子组件中组织调用, 也可以在PhaseAbility里做组织调用
 */
UCLASS(ClassGroup=(YiChenGamePhase), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPHASE_API UYcGamePhaseComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UYcGamePhaseComponent(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 组件开始播放时调用
	 * 在这里注册游戏体验加载完成事件以及各阶段的消息监听
	 */
	virtual void BeginPlay() override;
	
	/**
	 * 按照顺序开始下一个游戏阶段（不会跳跃）
	 * 使用 GamePhases 数组中当前索引的下一个阶段定义来驱动阶段切换
	 */
	UFUNCTION(BlueprintCallable)
	void StartNextPhase();
	
	/**
	 * 获取当前游戏阶段的标签
	 * @return 当前游戏阶段的 GameplayTag，如果尚未开始或索引无效则返回空标签
	 */
	UFUNCTION(BlueprintCallable)
	FGameplayTag GetCurrentPhase();
	
protected:
	/** 接收到阶段激活消息时的回调，用于防止阶段倒退或重复激活，同步支持向前跳跃式阶段激活 */
	void OnGamePhaseMessage(FGameplayTag Channel, const FGamePhaseMessage& Message);
	
	/**
	 * Experience加载完毕后的回调
	 * 将在这里开始第一个游戏阶段技能
	 * @param Channel 消息通道
	 * @param Message 经验加载消息
	 */
	void OnExperienceLoaded(FGameplayTag Channel, const FExperienceLoadedMessage& Message);
	
	/** 获取某个游戏阶段定义数据 */
	FYcGamePhaseDefinition* GetPhaseDefinition(const FGameplayTag& PhaseTag);
	
	/** 获取指定游戏阶段标签在 GamePhases 数组中的下标 */
	int32 GetPhaseIndex(const FGameplayTag& PhaseTag);
	
private:
	//@TODO 需要考虑以什么样更优雅的方式配置游戏阶段信息, 例如使用DataAsset或者DataTable管理, 然后定制一个GFA来添加PhaseComponent, 同时指定Phasee
	//@TODO 需要把GameplayFeature的代码抽离成一个独立的模块, 这样避免Phase模块去依赖Gameplay
	/** 
	 *  按顺序配置的游戏阶段定义列表, 用于驱动整场对局的阶段流转, 注意一定要按顺序配置!
	 *  在BeginPlay()中会绑定OnExperienceLoaded消息,在OnExperienceLoaded后会自动激活数组中第一个游戏状态
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TArray<FYcGamePhaseDefinition> GamePhases;
	
	/** 当前游戏阶段下标 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 CurrentPhaseIndex;
};
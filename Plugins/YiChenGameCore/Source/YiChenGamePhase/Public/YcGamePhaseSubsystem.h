// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "YcGamePhaseSubsystem.generated.h"

template <typename T> class TSubclassOf;

class UYcGamePhaseAbility;

/** 蓝图动态委托：在阶段开始或结束时回调，传入对应的游戏阶段能力实例 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FYcGamePhaseDynamicDelegate, const UYcGamePhaseAbility*, Phase);
/** C++ 委托：在阶段开始或结束时回调，传入对应的游戏阶段能力实例 */
DECLARE_DELEGATE_OneParam(FYcGamePhaseDelegate, const UYcGamePhaseAbility* Phase);

/** 蓝图动态委托：在阶段开始或结束时回调，传入对应的阶段标签 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FYcGamePhaseTagDynamicDelegate, const FGameplayTag&, PhaseTag);
/** C++ 委托：在阶段开始或结束时回调，传入对应的阶段标签 */
DECLARE_DELEGATE_OneParam(FYcGamePhaseTagDelegate, const FGameplayTag& PhaseTag);

// 游戏阶段标签匹配规则，用于决定观察者接收哪些阶段事件
UENUM(BlueprintType)
enum class EPhaseTagMatchType : uint8
{
	// 精确匹配：仅接收与注册标签完全相同的阶段事件
	// （例如：注册 "A.B" 只会匹配 A.B，而不会匹配 A.B.C）
	ExactMatch,

	// 部分匹配：接收以注册标签为根的所有子标签事件
	// （例如：注册 "A.B" 会匹配 A.B 以及 A.B.C）
	PartialMatch
};

/** 
 * 使用GameplayTag以嵌套方式管理游戏阶段的世界子系统。
 * 支持父阶段与子阶段同时激活，但不允许同级的兄弟阶段同时存在。
 * 例如：Game.Playing 与 Game.Playing.WarmUp 可以共存，但 Game.Playing 与 Game.ShowingScore 不能共存。
 * 当一个新的阶段被启动时，所有不是该阶段“祖先”的已激活阶段都会被结束。
 * 例如：当 Game.Playing 和 Game.Playing.CaptureTheFlag 处于激活状态，此时启动 Game.Playing.PostGame，
 * 则 Game.Playing 仍然保持激活，而 Game.Playing.CaptureTheFlag 会被结束。
 * 
 * 游戏阶段技能依赖 GameState 上的 UYcAbilitySystemComponent，因为它们代表的是“整场对局”的状态而非某个具体玩家，GameState 是最合适的持有者。
 * 本子系统用于管理当前激活的游戏阶段，并提供注册/触发指定阶段开始与结束事件的能力。
 * 各个具体阶段的实际逻辑通过继承 UYcGamePhaseAbility 来实现。
 * 
 * 注意：该子系统仅工作在权威端, 为权威端的游戏阶段能力提供支持, 具体的游戏阶段组织切换逻辑由UYcGamePhaseComponent及其子类进行定制实现
 */
UCLASS()
class YICHENGAMEPHASE_API UYcGamePhaseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UYcGamePhaseSubsystem();
	
	/**
	 * 获取指定世界上下文中关联的游戏阶段子系统
	 * @param WorldContextObject 用于获取世界实例的上下文对象
	 * @return 对应世界中的游戏阶段子系统引用
	 */
	static UYcGamePhaseSubsystem& Get(const UObject* WorldContextObject);

	/** 在子系统创建并初始化完成后调用，用于执行额外初始化逻辑 */
	virtual void PostInitialize() override;

	/**
	 * 决定是否为给定Outer创建该子系统
	 * @param Outer 拟作为子系统Outer的对象
	 * @return 允许创建返回true，否则返回false
	 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	/**
	 * 启动一个新的游戏阶段
	 * 会在GameState的AbilitySystemComponent上授予并激活对应的阶段能力
	 * @param PhaseAbility 要启动的游戏阶段能力类
	 * @param PhaseEndedCallback 阶段结束时回调（可选）
	 */
	void StartPhase(TSubclassOf<UYcGamePhaseAbility> PhaseAbility, const FYcGamePhaseDelegate& PhaseEndedCallback = FYcGamePhaseDelegate());
	
	//TODO 应返回一个句柄，以便使用者能够主动删除这些观察者。否则，它们会一直累积，直到世界（World）被重置。
	//TODO 或许我们应该定期清理这些观察者？因为即便提供了句柄，也不是每个人都能正确地取消注册。
	/** 
	 * 注册观察者，在指定阶段开始时触发回调
	 * 如果该阶段当前已经处于活动状态，回调会立即执行
	 * @param PhaseTag 要观察的游戏阶段标签
	 * @param MatchType 标签匹配类型
	 * @param OnPhaseStarted 阶段开始时的回调
	 */
	void RegisterPhaseStartedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, const FYcGamePhaseTagDelegate& OnPhaseStarted);
	
	/** 
	 * 注册观察者，在指定阶段**结束**时触发回调。
	 * 注意：如果阶段当前未活动，回调不会被触发。
	 * @param PhaseTag 要观察的游戏阶段标签
	 * @param MatchType 标签匹配类型
	 * @param OnPhaseEnded 阶段结束时的回调
	 */
	void RegisterPhaseEndedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, const FYcGamePhaseTagDelegate& OnPhaseEnded);
	
	/**
	 * 蓝图版：启动一个新的游戏阶段
	 * @param PhaseAbility 要启动的游戏阶段能力类
	 * @param PhaseEndedDelegate 阶段结束时的动态委托回调
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Phase", meta = (DisplayName="Start Phase", AutoCreateRefTerm = "PhaseEnded"))
	void K2_StartPhase(TSubclassOf<UYcGamePhaseAbility> PhaseAbility, const FYcGamePhaseDynamicDelegate& PhaseEndedDelegate);

	/**
	 * 蓝图版：注册阶段开始观察者
	 * @param PhaseTag 要观察的游戏阶段标签
	 * @param MatchType 标签匹配类型（精确匹配或部分匹配）
	 * @param OnPhaseStarted 阶段开始时的动态委托回调
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Phase", meta = (DisplayName = "Register Phase Started Observer", AutoCreateRefTerm = "WhenPhaseActive"))
	void K2_RegisterPhaseStartedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, FYcGamePhaseTagDynamicDelegate OnPhaseStarted);

	/**
	 * 蓝图版：注册阶段结束观察者
	 * @param PhaseTag 要观察的游戏阶段标签
	 * @param MatchType 标签匹配类型（精确匹配或部分匹配）
	 * @param OnPhaseEnded 阶段结束时的动态委托回调
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game Phase", meta = (DisplayName = "Register Phase Ended Observer", AutoCreateRefTerm = "WhenPhaseEnd"))
	void K2_RegisterPhaseEndedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, FYcGamePhaseTagDynamicDelegate OnPhaseEnded);
	
	/**
	 * 检查指定阶段标签是否当前处于激活状态
	 * @param PhaseTag 要检查的游戏阶段标签
	 * @return 如果阶段处于激活状态返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintPure = false, meta = (AutoCreateRefTerm = "PhaseTag"))
	bool IsPhaseActive(const FGameplayTag& PhaseTag) const;
	
protected:
	/** 限制该子系统支持的世界类型, 对于EditorWord是不需要的, 只在Game/PIE类型的世界创建  */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	
	/** 当一个游戏阶段技能激活时被调用, 在UYcGamePhaseAbility::ActivateAbility()中进行调用 */ 
	void OnBeginPhase(const UYcGamePhaseAbility* PhaseAbility, const FGameplayAbilitySpecHandle PhaseAbilityHandle);
	/** 当一个游戏阶段技能结束时被调用, 在UYcGamePhaseAbility::EndAbility()中进行调用 */ 
	void OnEndPhase(const UYcGamePhaseAbility* PhaseAbility, const FGameplayAbilitySpecHandle PhaseAbilityHandle);

private:
	
	/** 游戏阶段记录结构体 */
	struct FYcGamePhaseEntry
	{
		/** 游戏阶段标签 */
		FGameplayTag PhaseTag;
		/** 阶段结束的回调委托 */
		FYcGamePhaseDelegate PhaseEndedCallback;
	};

	/** 游戏阶段观察者结构体 */
	struct FPhaseObserver
	{
		bool IsMatch(const FGameplayTag& ComparePhaseTag) const;
	
		/** 游戏阶段标签 */
		FGameplayTag PhaseTag;
		/** 阶段标签匹配模式 */
		EPhaseTagMatchType MatchType = EPhaseTagMatchType::ExactMatch;
		/** 阶段回调委托 */
		FYcGamePhaseTagDelegate PhaseCallback;
	};
	
	/** 当前处于激活状态的游戏阶段 */
	TMap<FGameplayAbilitySpecHandle, FYcGamePhaseEntry> ActivePhaseMap;
	/** 游戏阶段开始的观察者列表, 每个观察则会通过PhaseTag指定要观察的目标游戏阶段, 通过MatchType指定标签匹配模式 */
	TArray<FPhaseObserver> PhaseStartObservers;
	/** 游戏阶段结束的观察者列表, 每个观察则会通过PhaseTag指定要观察的目标游戏阶段, 通过MatchType指定标签匹配模式 */
	TArray<FPhaseObserver> PhaseEndObservers;

	friend class UYcGamePhaseAbility;
};
// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "UIExtensionSystem.h"
#include "YcInteractableTarget.h"
#include "Components/ActorComponent.h"
#include "YcInteractableComponent.generated.h"

/**
 * 为Actor提供游戏世界中可交互功能的组件, 通过配置Option中不同的InteractionAbilityToGrant实现不同的交互表现 
 */
UCLASS(ClassGroup=(YiChenGameplay), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcInteractableComponent : public UActorComponent, public IYcInteractableTarget
{
	GENERATED_BODY()

public:
	UYcInteractableComponent();
	
	// IYcInteractableTarget interface.
	/**
	 * 收集并构建此组件的交互选项。
	 *
	 * 当配置为“不可交互且不显示提示”时，这里会直接跳过，不向外部系统提供交互项；
	 * 当配置为“不可交互但显示禁用提示”时，依然会提供交互项，但交互执行层会阻止真正触发。
	 *
	 * @param InteractQuery	交互查询者提供的上下文信息（通常是玩家及其控制器）。
	 * @param InteractionBuilder	用于向外部填充 `FYcInteractionOption` 的构造器。
	 */
	virtual void GatherInteractionOptions(const FYcInteractionQuery& InteractQuery, FYcInteractionOptionBuilder& InteractionBuilder) override;

	/**
	 * 更新当前组件持有的交互配置。
	 *
	 * 如果UI类或挂载点发生变化，会自动重建当前聚焦中的交互提示Widget；
	 * 否则仅刷新当前已显示的Widget绑定数据。
	 *
	 * @param NewOption 新的交互配置
	 */
	virtual void UpdateInteractionOption(const FYcInteractionOption& NewOption) override;
	virtual void OnPlayerFocusBegin(const FYcInteractionQuery& InteractQuery) override;
	virtual void OnPlayerFocusEnd(const FYcInteractionQuery& InteractQuery) override;
	// ~IYcInteractableTarget interface.

	/**
	 * 直接整体设置交互配置。
	 * 适合外部一次性替换整个FYcInteractionOption。
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionOption(const FYcInteractionOption& NewOption);

	/** 获取当前交互配置。 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	FYcInteractionOption& GetInteractionOption();
	
	/** 获取当前交互配置。(Const版本) */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	const FYcInteractionOption& GetInteractionOptionConst() const;

	/**
	 * 运行时切换当前是否允许交互。
	 *
	 * 当切换为false时，是否仍显示交互提示由 `DisabledDisplayPolicy` 决定；
	 * 当玩家当前正聚焦该物体时，UI会立即刷新。
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bNewEnabled);

	/**
	 * 设置不可交互时的提示显示策略。
	 *
	 * 可用于在运行时切换“隐藏提示”和“显示禁用态提示”两种表现。
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetDisabledDisplayPolicy(EYcInteractionDisabledDisplayPolicy NewPolicy);

	/** 当前是否允许真正执行交互。 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteractionEnabled() const;

	/** 当前配置下，聚焦时是否应显示交互提示。 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool ShouldShowInteractionPrompt() const;
	

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerFocusBegin,const FYcInteractionQuery&, InteractQuery);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerFocusEnd,const FYcInteractionQuery&, InteractQuery);
	/**
	 * 当玩家开始注视/聚焦时触发的委托, 注意这只是发生在玩家本地发生的
	 * 例如可以用来实现物体被玩家聚焦时开启物体高亮描边
	 * @param InteractQuery 聚焦的玩家数据
	 */
	UPROPERTY(BlueprintAssignable)
	FPlayerFocusBegin OnPlayerFocusBeginEvent;
	
	/**
	 * 当玩家结束注视/聚焦时触发的事件, 注意这只是发生在玩家本地发生的
	 * 例如可以用来实现物体失去玩家焦点时关闭物体高亮描边
	 * @param InteractQuery 失焦的玩家数据
	 */
	UPROPERTY(BlueprintAssignable)
	FPlayerFocusEnd OnPlayerFocusEndEvent;

protected:
	/** 这个交互组件的交互配置信息, 通过配置不同的InteractionAbilityToGrant实现不同的交互表现 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interactable)
	FYcInteractionOption Option;
	
private:
	/**
	 * 刷新当前交互Widget的显示状态。
	 *
	 * 当组件正处于玩家聚焦状态时：
	 * 1. 如果当前不应显示提示，则移除Widget
	 * 2. 如果UI配置发生变化，则重建Widget
	 * 3. 否则刷新已有Widget的数据绑定
	 */
	void RefreshInteractionWidget(bool bForceRecreate = false);

	/** 注册交互提示Widget，并将当前交互组件绑定给Widget。 */
	void RegisterInteractionWidget();

	/** 注销当前交互提示Widget，并解除与Widget的绑定。 */
	void UnregisterInteractionWidget();
	
	/** 当前交互UI的句柄, 用于在失去玩家焦点后移除交互UI */
	FUIExtensionHandle InteractionWidgetHandle;

	/** 当前组件是否正处于玩家聚焦状态，用于支持运行时刷新UI。 */
	bool bIsFocused = false;

	/** 最近一次聚焦时的查询上下文，用于在运行时重建Widget时恢复显示上下文。 */
	FYcInteractionQuery LastFocusQuery;
};

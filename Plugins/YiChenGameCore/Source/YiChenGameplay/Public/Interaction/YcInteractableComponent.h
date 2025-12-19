// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

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
	 * @param InteractQuery	交互查询者提供的上下文信息（通常是玩家及其控制器）。
	 * @param InteractionBuilder	用于向外部填充 `FYcInteractionOption` 的构造器。
	 */
	virtual void GatherInteractionOptions(const FYcInteractionQuery& InteractQuery, FYcInteractionOptionBuilder& InteractionBuilder) override;
	virtual void OnPlayerFocusBegin(const FYcInteractionQuery& InteractQuery) override;
	virtual void OnPlayerFocusEnd(const FYcInteractionQuery& InteractQuery) override;
	// ~IYcInteractableTarget interface.
	

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
};
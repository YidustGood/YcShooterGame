// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcInteractionTypes.h"
#include "UObject/Interface.h"
#include "YcInteractableTarget.generated.h"

struct FYcInteractionQuery;

/**
 * @class FYcInteractionOptionBuilder
 * @brief 交互选项构建器。
 * 
 * 用于在运行时动态构建可交互对象的交互选项列表，并自动将当前交互目标的接口实例附加到每个选项中。
 */
class FYcInteractionOptionBuilder
{
public:
	/**
	 * 构造函数。
	 * @param InterfaceTargetScope 当前正在构建选项的可交互目标对象。
	 * @param InteractOptions 用于存储生成的交互选项的数组引用。
	 */
	FYcInteractionOptionBuilder(TScriptInterface<IYcInteractableTarget> InterfaceTargetScope, TArray<FYcInteractionOption>& InteractOptions)
		: Scope(InterfaceTargetScope)
		  , Options(InteractOptions)
	{
	}

	/**
	 * 添加一个交互选项到构建器中。
	 * 该函数会将传入的选项添加至列表，并将其 `InteractableTarget` 成员设置为当前范围内的可交互目标。
	 * @param Option 要添加的交互选项配置。
	 */
	void AddInteractionOption(const FYcInteractionOption& Option)
	{
		FYcInteractionOption& OptionEntry = Options.Add_GetRef(Option);
		OptionEntry.InteractableTarget = Scope;
	}

private:
	/** 当前交互目标的接口实例对象。*/
	TScriptInterface<IYcInteractableTarget> Scope;

	/** 交互选项列表的引用，用于添加新的选项。*/
	TArray<FYcInteractionOption>& Options;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UYcInteractableTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * @interface IYcInteractableTarget
 * @brief 定义可交互对象行为的接口。
 *
 * 任何希望能够被玩家或其他系统交互的Actor或对象都应实现此接口。
 */
class YICHENGAMEPLAY_API IYcInteractableTarget
{
	GENERATED_BODY()
public:
	
	/**
	 * 收集当前可交互对象的所有交互选项。
	 * 实现此函数的对象应根据查询信息，决定提供哪些交互选项，并通过OptionBuilder将它们添加进去。
	 * @param InteractQuery 包含交互发起者信息的查询结构体。
	 * @param OptionBuilder 用于构建和添加交互选项的工具。
	 */
	virtual void GatherInteractionOptions(const FYcInteractionQuery& InteractQuery, FYcInteractionOptionBuilder& OptionBuilder) = 0;

	/**
	 * 自定义交互时触发的Gameplay事件数据。
	 * 当交互成功并准备发送Gameplay事件时，此函数被调用，允许交互目标向事件负载中添加额外数据。
	 * @param InteractionEventTag 正在处理的交互事件的标签。
	 * @param InOutEventData 将要发送的Gameplay事件数据，可在此函数中进行修改和填充。
	 */
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData) {};
	
	/**
	 * 当玩家开始注视/聚焦时会被调用的接口, 注意这只是发生在玩家本地发生的
	 * 例如可以用来实现物体被玩家聚焦时开启物体高亮描边
	 * 接口是在UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions中被处理调用
	 * @param InteractQuery 聚焦的玩家数据
	 */
	virtual void OnPlayerFocusBegin(const FYcInteractionQuery& InteractQuery) {};
	
	/**
	 * 当玩家结束注视/聚焦时会被调用的接口, 注意这只是发生在玩家本地发生的
	 * 例如可以用来实现物体失去玩家焦点时关闭物体高亮描边
	 * @param InteractQuery 失焦的玩家数据
	 * 接口是在UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions中被处理调用
	 */
	virtual void OnPlayerFocusEnd(const FYcInteractionQuery& InteractQuery) {};
};
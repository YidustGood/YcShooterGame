// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbility.h"
#include "UObject/Object.h"
#include "YcInteractionTypes.generated.h"

class IYcInteractableTarget;
class UUserWidget;

/** 
 * 一个可交互对象的配置结构体
 * 用于配置具体交互逻辑的实现技能、交互UI、交互描述文本等
 */
USTRUCT(BlueprintType)
struct FYcInteractionOption
{
	GENERATED_BODY()

public:
	/** 可交互目标对象 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Option")
	TScriptInterface<IYcInteractableTarget> InteractableTarget;

	/** 交互主文本, 例如用来描述交互目标名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	/** 交互副文本, 例如用来描述交互效果 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SubText;

	// 1、将交互时需要激活的能力授予角色


	/** 当角色接近可交互物体时授予的能力类 */
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> InteractionAbilityToGrant;

	// - OR -

	// 2、允许可交互对象拥有自身的能力系统和交互能力，由对象来执行
	/** 目标对象上的 AbilitySystem，可用于存放句柄并发送事件 */
	UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly, Category="Option")
	TObjectPtr<UAbilitySystemComponent> TargetAbilitySystem = nullptr;

	/** 本交互选项在目标对象上激活的能力句柄 */
		UPROPERTY(BlueprintReadOnly)
	FGameplayAbilitySpecHandle TargetInteractionAbilityHandle;

	// UI
	//--------------------------------------------------------------

	/** 显示此交互的 UI 类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	//--------------------------------------------------------------

public:
	// 重载==运算符，将内部所有对象进行一一对比
	FORCEINLINE bool operator==(const FYcInteractionOption& Other) const
	{
		return InteractableTarget == Other.InteractableTarget &&
			InteractionAbilityToGrant == Other.InteractionAbilityToGrant &&
			TargetAbilitySystem == Other.TargetAbilitySystem &&
			TargetInteractionAbilityHandle == Other.TargetInteractionAbilityHandle &&
			InteractionWidgetClass == Other.InteractionWidgetClass &&
			Text.IdenticalTo(Other.Text) &&
			SubText.IdenticalTo(Other.SubText);
	}

	FORCEINLINE bool operator!=(const FYcInteractionOption& Other) const
	{
		return !operator==(Other);
	}

	FORCEINLINE bool operator<(const FYcInteractionOption& Other) const
	{
		return InteractableTarget.GetInterface() < Other.InteractableTarget.GetInterface();
	}
};

/** 
 * 交互查询者的信息结构体 / 射线查询发起者的信息 (在UAbilityTask_WaitForInteractableTargets_SingleLineTrace中使用)
 */
USTRUCT(BlueprintType)
struct FYcInteractionQuery
{
	GENERATED_BODY()

public:
	/** 发起交互的Actor对象, 通常就是玩家操控的Pawn或者Character */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> RequestingAvatar;

	/** 可选：交互发起者的控制器（可与 Pawn 所属者不同） */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AController> RequestingController;

	/** 用于存放交互所需额外数据的通用对象 */
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<UObject> OptionalObjectData;
};


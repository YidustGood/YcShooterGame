// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "InteractionWidgetInterface.generated.h"

class UYcInteractableComponent;

/**
 * 为Widget绑定交互对象组件的接口, 实现了该类就可以获取玩家当前聚焦的交互对象信息
 */
UINTERFACE(BlueprintType)
class UInteractionWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 为Widget绑定交互对象组件的接口, 实现了该类就可以获取玩家当前聚焦的交互对象信息
 */
class YICHENGAMEPLAY_API IInteractionWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void BindInteractableComponent(UYcInteractableComponent* InteractableComp);

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void UnbindInteractableComponent(const UYcInteractableComponent* InteractableComp);
};
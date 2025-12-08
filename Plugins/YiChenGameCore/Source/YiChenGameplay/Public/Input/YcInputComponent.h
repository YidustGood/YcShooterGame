// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "YcInputConfig.h"
#include "YiChenGameplay.h"
#include "YcInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

/**
 *	用于通过输入配置数据资产来管理输入映射和绑定的组件
 *	需要在项目设置中指定该组件为玩家默认输入组件
 */
UCLASS(Config = Input)
class YICHENGAMEPLAY_API UYcInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()
public:
	UYcInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(const UYcInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const UYcInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template <class UserClass, typename FuncType>
	void BindNativeAction(const UYcInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UYcInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
};

template <class UserClass, typename FuncType>
void UYcInputComponent::BindNativeAction(const UYcInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		const FString TagName = InputTag.GetTagName().ToString();
		const FString Msg = FString::Printf(TEXT("[IA: %s , Tag: %s]"),*IA->GetName(), *TagName);
		UE_LOG(LogYcInput, Log, TEXT("YcInputComponent::BindNativeAction: %s"), *Msg);
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UYcInputComponent::BindAbilityActions(const UYcInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);
	// 绑定InputConfig中的所有AbilityAction，当按下和释放对应IA的按键时回调用回调函数，并将Action.InputTag作为参数传入，以此来判定是哪个IA被按下/释放了，IA和Tag 一一对应
	// Ability的InputAction需要文件中配置对应IA的名称，否则会出现穿透触发的问题
	for (const FYcInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (!Action.InputAction || !Action.InputTag.IsValid()) continue;
		
		const FString TagName = Action.InputTag.GetTagName().ToString();
		const FString Msg = FString::Printf(TEXT("[IA: %s , Tag: %s]"), *Action.InputAction->GetName(), *TagName);
		UE_LOG(LogYcInput, Log, TEXT("YcInputComponent::BindAbilityActions: %s"), *Msg);
		// @TODO 目前只针对Triggered、Completed两个输入做了绑定, 对于其它类型是否需要做支持呢?
		if (PressedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
		}

		if (ReleasedFunc)
		{
			BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
		}
	}
}
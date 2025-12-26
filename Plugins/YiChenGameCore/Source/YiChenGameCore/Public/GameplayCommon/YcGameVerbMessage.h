// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "YcGameVerbMessage.generated.h"

struct FGameplayCueParameters;
class APlayerController;
class APlayerState;
class UObject;

/**
 * 表示一个通用游戏消息结构，格式为：Instigator（发起者）Verb（动作）Target（目标）（在Context（上下文）中，数值为Magnitude）
 * 用于在游戏系统中传递各种动作和事件信息, 例如玩家A对玩家B造成了伤害的消息
 */
USTRUCT(BlueprintType)
struct FYcGameVerbMessage
{
	GENERATED_BODY()

	/** 消息的动作标签，标识消息的类型或通道 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTag Verb;

	/** 发起动作的对象，通常是触发事件的Actor */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Instigator = nullptr;

	/** 动作的目标对象，通常是受影响的Actor */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Target = nullptr;

	/** 发起者相关的标签集合 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer InstigatorTags;

	/** 目标相关的标签集合 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer TargetTags;

	/** 上下文相关的标签集合 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer ContextTags;

	/** 消息的数值大小，默认为1.0 */
	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	double Magnitude = 1.0;

	/**
	 * 获取消息的调试字符串表示
	 * @return 格式化的字符串，包含消息的所有字段信息
	 */
	YICHENGAMECORE_API FString ToString() const;
};

/**
 * 游戏动作消息的辅助函数库
 * 提供消息格式转换、对象查找等实用功能
 */
UCLASS()
class YICHENGAMECORE_API UYcGameVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * 从任意对象中提取PlayerState
	 * 支持从PlayerController、PlayerState、Pawn等对象中获取PlayerState
	 * @param Object 源对象，可以是PlayerController、PlayerState或Pawn
	 * @return 找到的PlayerState，如果无法找到则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore")
	static APlayerState* GetPlayerStateFromObject(UObject* Object);

	/**
	 * 从任意对象中提取PlayerController
	 * 支持从PlayerController、PlayerState、Pawn、Actor等对象中获取PlayerController
	 * @param Object 源对象，可以是PlayerController、PlayerState、Pawn或Actor
	 * @return 找到的PlayerController，如果无法找到则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);

	/**
	 * 将游戏动作消息转换为GameplayCue参数
	 * @param Message 源消息结构
	 * @return 转换后的GameplayCue参数结构
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore")
	static FGameplayCueParameters VerbMessageToCueParameters(const FYcGameVerbMessage& Message);

	/**
	 * 将GameplayCue参数转换为游戏动作消息
	 * @param Params 源GameplayCue参数结构
	 * @return 转换后的游戏动作消息结构
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore")
	static FYcGameVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};
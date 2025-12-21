// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "ExperienceMessageTypes.generated.h"

/**
 * 定义GameplayMessageSubsystem广播消息时使用的消息通道GameplayTag
 */
namespace YcGameplayTags
{
	YICHENGAMECORE_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Experience_StateEvent_Loaded_HighPriority);
	YICHENGAMECORE_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Experience_StateEvent_Loaded);
	YICHENGAMECORE_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Experience_StateEvent_Loaded_LowPriority);
}

/**
 * Experience加载完成的消息数据
 * 配合GameplayMessageSubsystem广播消息, 实现跨模块低耦合的Experience加载完成消息通知
 */
USTRUCT(BlueprintType)
struct FExperienceLoadedMessage
{
	GENERATED_BODY()
	
	/** 使用UObject类型指针指向的Experience */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<const UObject> LoadedExperienceObject;
};
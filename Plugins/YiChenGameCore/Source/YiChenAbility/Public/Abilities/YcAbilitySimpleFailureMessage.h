// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "YcAbilitySimpleFailureMessage.generated.h"

/** 技能激活失败消息的GameplayTag，用于通过GameplayMessageSubsystem广播失败消息 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ABILITY_SIMPLE_FAILURE_MESSAGE);

/**
 * 技能激活失败消息结构
 * 用于通过GameplayMessageSubsystem向UI系统传递技能激活失败的用户友好提示信息
 * 当技能激活失败时，会根据FailureTagToUserFacingMessages映射表查找对应的用户提示文本并广播此消息
 */
USTRUCT(BlueprintType)
struct FYcAbilitySimpleFailureMessage
{
	GENERATED_BODY()

public:
	/** 接收失败消息的玩家控制器，用于确定消息的目标客户端 */
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	/** 导致技能激活失败的标签容器，包含所有失败原因标签 */
	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer FailureTags;

	/** 面向用户的失败原因文本，从FailureTagToUserFacingMessages映射表中查找得到 */
	UPROPERTY(BlueprintReadWrite)
	FText UserFacingReason;
};
// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "YcPersistenceMessages.generated.h"

class APlayerController;
class ULocalPlayer;

/** Persistence 流程统一使用的消息通道。 */
namespace YcPersistenceTags
{
	/** 标记数据已脏。 */
	YICHENGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Persistence_MarkDirty);
	/** 请求自动保存。 */
	YICHENGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Persistence_RequestAutosave);
	/** 请求立即刷盘。 */
	YICHENGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Persistence_RequestFlushSave);
	/** 请求提交局内结算。 */
	YICHENGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Persistence_RequestCommitMatchResult);
	/** Profile 已经完成 Hydrate，运行时内容已装载。 */
	YICHENGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Persistence_ProfileHydrated);
	/** 当前激活 Profile 发生变化。 */
	YICHENGAMEPLAY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Persistence_ProfileChanged);
}

/**
 * 发给 SaveCoordinator 的持久化请求消息。
 * 既可表达“只是脏了”，也可表达“需要立即落盘/提交局内结果”等更强语义。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEPLAY_API FYcPersistenceRequestMessage
{
	GENERATED_BODY()

	/** 优先使用的控制器引用；服务端真正执行保存时依赖它解析玩家上下文。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	/** 控制器暂不可用时的兜底引用，便于稍后重新解析 PlayerController。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	TObjectPtr<ULocalPlayer> LocalPlayer = nullptr;

	/** 触发本次请求的原因标签，用于日志、调试和失败回溯。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	FGameplayTag ReasonTag;

	/** 是否期望立即执行，而不是只进入延迟保存队列。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	bool bImmediate = false;

	/** 即使当前未脏，也允许强制触发一次保存。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	bool bForceIfNotDirty = false;

	/** 自动保存时是否允许去抖，避免频繁小改动反复刷盘。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	bool bAllowDebounce = true;

	/** 局内结算请求使用的结果标记，例如是否成功撤离。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	bool bExtractionSucceeded = false;
};

/** Profile 生命周期广播消息，用于通知“已装载”或“已切换档位”。 */
USTRUCT(BlueprintType)
struct YICHENGAMEPLAY_API FYcPersistenceProfileMessage
{
	GENERATED_BODY()

	/** 对应的玩家控制器。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	/** 对应的本地玩家，用于在控制器暂时无效时回溯上下文。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	TObjectPtr<ULocalPlayer> LocalPlayer = nullptr;

	/** 本次生命周期事件的原因标签，通常是 Hydrated 或 ProfileChanged。 */
	UPROPERTY(BlueprintReadOnly, Category = Persistence)
	FGameplayTag ReasonTag;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "YcPersistenceLibrary.generated.h"

class APlayerController;

/**
 * Persistence 蓝图入口。
 * 这一层本身不直接执行存档，而是把“脏标记/自动保存/立即刷盘/局内结算提交”
 * 这些请求统一转成 GameplayMessage，交给 SaveCoordinator 和 PersistenceComponent 处理。
 */
UCLASS()
class YICHENGAMEPLAY_API UYcPersistenceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 标记当前玩家档案已脏，供协调器后续安排自动保存。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	static bool MarkDirty(APlayerController* PlayerController, FGameplayTag ReasonTag);

	/** 请求一次自动保存；允许去抖时会进入延迟刷盘队列。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	static bool RequestAutosave(APlayerController* PlayerController, FGameplayTag ReasonTag, bool bAllowDebounce = true);

	/** 请求立即保存；常用于切场景、关闭界面或其他希望尽快落盘的时机。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	static bool RequestFlushSave(APlayerController* PlayerController, FGameplayTag ReasonTag, bool bForceIfNotDirty = false);

	/** 请求提交一局战斗的结果，将局内负载回写到当前 Profile。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	static bool RequestCommitMatchResult(APlayerController* PlayerController, bool bExtractionSucceeded, FGameplayTag ReasonTag = FGameplayTag());

	/** 不带额外原因标签的简化结算提交入口。 */
	UFUNCTION(BlueprintCallable, Category = "YcGame|Persistence")
	static bool RequestCommitMatchResultSimple(APlayerController* PlayerController, const bool bExtractionSucceeded);
};

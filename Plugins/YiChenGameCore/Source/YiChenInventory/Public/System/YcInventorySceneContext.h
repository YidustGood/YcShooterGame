// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcMetaInventoryTypes.h"
#include "YcInventorySceneContext.generated.h"

/**
 * 库存场景上下文。
 * 用于统一描述“当前在哪个场景（局内/局外）以及该场景绑定的库存对象”。
 */
UCLASS(BlueprintType)
class YICHENINVENTORY_API UYcInventorySceneContext : public UObject
{
	GENERATED_BODY()

public:
	/** 场景类型（局内/局外）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	EYcInventorySceneType SceneType = EYcInventorySceneType::InMatch;

	/** 当前激活 Profile 身份（用于持久化定位与脏标记）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcProfileIdentity ProfileIdentity;

	/** 显式运行时句柄。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcPlayerInventoryRuntime Runtime;

	/** 是否为局外场景。 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOutOfMatchContext() const { return SceneType == EYcInventorySceneType::OutOfMatch; }

	/** 是否为局内场景。 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInMatchContext() const { return SceneType == EYcInventorySceneType::InMatch; }

	/** 当前上下文是否满足“局外档案读写”前置条件。 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidForOutOfMatchPersistence() const;
};

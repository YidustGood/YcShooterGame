// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcInventorySceneTypes.h"
#include "YcInventorySceneContext.generated.h"

class UYcInventoryManagerComponent;
class AActor;

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

	/** 账号ID（用于持久化定位与脏标记）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FString AccountId;

	/** 上下文拥有者（通常是角色/控制器）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<AActor> ContextOwner = nullptr;

	/** 玩家自身库存组件引用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UYcInventoryManagerComponent> PlayerInventoryRef = nullptr;

	/** 容器库存组件引用（局外时通常是仓库）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UYcInventoryManagerComponent> ContainerInventoryRef = nullptr;

	/** 是否需要持久化提交（局外一般为 true）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bRequirePersistenceCommit = false;

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

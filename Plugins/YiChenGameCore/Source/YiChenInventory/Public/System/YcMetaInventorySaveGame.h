// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "System/YcMetaInventoryTypes.h"
#include "YcMetaInventorySaveGame.generated.h"

/**
 * Meta Inventory 本地存档对象。
 * 作为 SaveGame 的序列化载体，保存账号级库存根快照。
 */
UCLASS()
class YICHENINVENTORY_API UYcMetaInventorySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 账号级库存根快照。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcMetaInventoryRootSnapshot RootSnapshot;
};

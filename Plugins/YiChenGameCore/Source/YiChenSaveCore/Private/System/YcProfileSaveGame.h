// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "System/YcSaveCoreTypes.h"
#include "YcProfileSaveGame.generated.h"

/** SaveCore 本地落盘对象。 */
UCLASS()
class UYcProfileSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** Profile 根存档数据。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FYcProfileSaveRoot Root;
};

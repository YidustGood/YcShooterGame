// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YcTeamAsset.generated.h"

/**
 * 一个团队的数据资产, 可以自由扩展
 * 例如用来存储队伍颜色信息、队伍图标信息等
 */
UCLASS()
class YICHENTEAMS_API UYcTeamAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** 团队名称，可在编辑器中编辑，蓝图只读 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText TeamName;
};
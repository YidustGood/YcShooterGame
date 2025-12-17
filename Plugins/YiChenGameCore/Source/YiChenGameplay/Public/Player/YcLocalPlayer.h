// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonLocalPlayer.h"
#include "YcLocalPlayer.generated.h"

/**
 * 插件所提供的LocalPlayer类, 需要在项目设置->引擎-一般设置->默认类-本地玩家类中指定使用YcLocalPlayer或其子类
 * 游戏设置相关内容可以通过LocalPlayer进行部分实现
 */
UCLASS()
class YICHENGAMEPLAY_API UYcLocalPlayer : public UCommonLocalPlayer
{
	GENERATED_BODY()
};

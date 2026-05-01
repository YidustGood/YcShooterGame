// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * 动态局内 UI 框架使用的统一 GameplayTag 声明。
 *
 * 这些 Tag 属于框架级协议，不绑定具体业务语义。
 * 请求层、路由层、表现层都应统一复用这里的声明，避免分散定义。
 */
namespace YcDynamicScreenUITags
{
	YICHENGAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Dynamic_Request_Show);
	YICHENGAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Dynamic_Request_Update);
	YICHENGAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Dynamic_Request_Hide);
	YICHENGAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Dynamic_Update);
	YICHENGAMEUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Dynamic_Hide);
}

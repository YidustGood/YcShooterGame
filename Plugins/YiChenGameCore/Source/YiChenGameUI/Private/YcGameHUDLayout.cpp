// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGameHUDLayout.h"

#include "CommonUIExtensions.h"
#include "NativeGameplayTags.h"
#include "UITag.h"
#include "Input/CommonUIInputTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameHUDLayout)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");

UYcGameHUDLayout::UYcGameHUDLayout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UYcGameHUDLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// 绑定输入返回键输入Tag回调函数，用什么按键映射到Tag在项目设置-插件-common ui input settings-input actions中进行配置
	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE), false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)));
}

void UYcGameHUDLayout::HandleEscapeAction() const
{
	// 创建返回菜单UI
	if (ensure(!EscapeMenuClass.IsNull()))
	{
		UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, EscapeMenuClass);
	}
}
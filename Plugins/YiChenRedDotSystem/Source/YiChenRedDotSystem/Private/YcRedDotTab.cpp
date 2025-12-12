// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcRedDotTab.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcRedDotTab)

void UYcRedDotTab::NativeConstruct()
{
	Super::NativeConstruct();
	// 自动注册红点监听
	IYcRedDotProvider::InitializeRedDotProvider();
}

void UYcRedDotTab::NativeDestruct()
{
	// 自动注销红点监听
	IYcRedDotProvider::UninitializeRedDotProvider();
	Super::NativeDestruct();
}

FGameplayTag UYcRedDotTab::GetRedDotTag_Implementation() const
{
	return TabRedDotTag;
}

void UYcRedDotTab::OnRedDotStateChanged_Implementation(FGameplayTag RedDotTag, const FRedDotInfo& RedDotInfo)
{
	TabRedDotInfo = RedDotInfo;
}
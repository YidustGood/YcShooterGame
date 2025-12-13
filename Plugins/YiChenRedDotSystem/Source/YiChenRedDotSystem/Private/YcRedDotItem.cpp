// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcRedDotItem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcRedDotItem)

void UYcRedDotItem::NativeConstruct()
{
	Super::NativeConstruct();
	IYcRedDotDataProvider::InitializeRedDotDataProvider();
}

void UYcRedDotItem::NativeDestruct()
{
	IYcRedDotDataProvider::UninitializeRedDotDataProvider();
	Super::NativeDestruct();
}

FGameplayTag UYcRedDotItem::GetTargetRedDotTag_Implementation() const
{
	return TargetTabRedDotTag;
}

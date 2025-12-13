// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcRedDotDataProvider.h"

#include "YcRedDotManagerSubsystem.h"
#include "YiChenRedDotSystem.h"


void IYcRedDotDataProvider::InitializeRedDotDataProvider()
{
	UObject* Owner = Cast<UObject>(this);
	if (!Owner)
	{
		return;
	}

	if (!UYcRedDotManagerSubsystem::HasInstance(Owner))
	{
		return;
	}
    
	UYcRedDotManagerSubsystem& RedDotManager = UYcRedDotManagerSubsystem::Get(Owner);

	// 获取目标红点标签
	FGameplayTag TargetTag = Execute_GetTargetRedDotTag(Owner);
	if (!TargetTag.IsValid())
	{
		UE_LOG(LogYcRedDot, Error, TEXT("Invalid TargetRedDotTag to initialize."));
		return;
	}

	// 注册红点清理事件监听
	ClearedHandle = 
		RedDotManager.RegisterRedDotDataProvider(
			TargetTag,
			[Owner, this]()
			{
				Execute_OnNotifyRedDotCleared(Owner);
				// 通知清除就不再影响红点数据了, 所以可以直接注销掉了
				UninitializeRedDotDataProvider();
			});
}

void IYcRedDotDataProvider::UninitializeRedDotDataProvider()
{
	// 注销监听
	if (ClearedHandle.IsValid())
	{
		ClearedHandle.Unregister();
	}
}
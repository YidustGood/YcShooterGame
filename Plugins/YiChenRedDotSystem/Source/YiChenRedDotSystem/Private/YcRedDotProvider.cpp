// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcRedDotProvider.h"


#include "YcRedDotManagerSubsystem.h"
#include "YiChenRedDotSystem.h"

void IYcRedDotProvider::InitializeRedDotProvider()
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

    // 获取该提供者关联的所有红点标签
    FGameplayTag Tag = Execute_GetRedDotTag(Owner);
    if (!Tag.IsValid())
    {
        UE_LOG(LogYcRedDot, Error, TEXT("Invalid RedDotTag provided to RegisterRedDotTag"));
        return;
    }
    RedDotManager.RegisterRedDotTag(Tag);

    // 注册状态变化监听
    FYcRedDotStateChangedListenerHandle Handle = 
        RedDotManager.RegisterRedDotStateChangedListener(
            Tag,
            [Owner, Tag](FGameplayTag ChangedTag, const FRedDotInfo* Info)
            {
                
                Execute_OnRedDotStateChanged(
                    Owner, ChangedTag, *Info);
                Execute_OnNotifyRedDotUpdated(Owner, Info->Count > 0, Info->Count);
            },
            EYcRedDotTagMatch::PartialMatch,
            true);

    StateChangedHandles.Add(Tag, Handle);
    UE_LOG(LogYcRedDot, Log, TEXT("注册红点标签UI, Tag: %s"), *Tag.ToString());
}

void IYcRedDotProvider::UninitializeRedDotProvider()
{
    // 注销所有监听
    for (auto& Pair : StateChangedHandles)
    {
        Pair.Value.Unregister();
    }
    StateChangedHandles.Empty();
    // 获取该提供者关联的所有红点标签
    UObject* Owner = Cast<UObject>(this);
    if (Owner)
    {
        FGameplayTag Tag = Execute_GetRedDotTag(Owner);
        UE_LOG(LogYcRedDot, Log, TEXT("注销红点标签UI, Tag: %s"), *Tag.ToString());
    }
}

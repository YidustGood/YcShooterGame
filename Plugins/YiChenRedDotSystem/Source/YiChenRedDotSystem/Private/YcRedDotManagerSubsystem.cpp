// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcRedDotManagerSubsystem.h"

#include "YiChenRedDotSystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UObject/ObjectMacros.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcRedDotManagerSubsystem)

void FYcRedDotStateChangedListenerHandle::Unregister()
{
	
	if (UYcRedDotManagerSubsystem* StrongSubsystem = Subsystem.Get())
	{
		StrongSubsystem->UnregisterStateChangedListener(*this);
		Subsystem.Reset();
		RedDotTag = FGameplayTag();
		ID = 0;
	}
}

void FYcRedRelierListenerHandle::Unregister()
{
	if (UYcRedDotManagerSubsystem* StrongSubsystem = Subsystem.Get())
	{
		StrongSubsystem->UnregisterRelierListener(*this);
		Subsystem.Reset();
		RedDotTag = FGameplayTag();
		ID = 0;
	}
}

UYcRedDotManagerSubsystem& UYcRedDotManagerSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);
	UYcRedDotManagerSubsystem* RedDotManager = UGameInstance::GetSubsystem<UYcRedDotManagerSubsystem>(World->GetGameInstance());
	check(RedDotManager);
	return *RedDotManager;
}

bool UYcRedDotManagerSubsystem::HasInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	UYcRedDotManagerSubsystem* RedDotManager = World != nullptr ? UGameInstance::GetSubsystem<UYcRedDotManagerSubsystem>(World->GetGameInstance()) : nullptr;
	return RedDotManager != nullptr;
}

void UYcRedDotManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogYcRedDot, Log, TEXT("RedDotManagerSubsystem Initialized"));
	
}

void UYcRedDotManagerSubsystem::Deinitialize()
{
	RedDotStates.Empty();
	Super::Deinitialize();
}

bool UYcRedDotManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UYcRedDotManagerSubsystem::RegisterRedDotTag(FGameplayTag RedDotTag)
{
	if (!RedDotTag.IsValid())
	{
		UE_LOG(LogYcRedDot, Warning, TEXT("Invalid RedDotTag provided to RegisterRedDotTag"));
		return;
	}
	
	// 清理该标签相关的缓存，确保后续查询能获取最新的子标签
	InvalidateTagHierarchyCache(RedDotTag);
	
	// 初始化该标签的红点信息
	FRedDotInfo& Info = GetOrCreateRedDotInfo(RedDotTag);
}

void UYcRedDotManagerSubsystem::AddRedDotCount(FGameplayTag RedDotTag, int32 Count)
{
	if (!RedDotTag.IsValid())
	{
		UE_LOG(LogYcRedDot, Warning, TEXT("Invalid RedDotTag provided to SetRedDotState"));
		return;
	}
	
	
	FRedDotInfo& Info = GetOrCreateRedDotInfo(RedDotTag);
	
	if (Info.Count == 0 && Count <= 0) return;
	int32 OldCount = Info.Count;
	Info.Count = FMath::Max(Info.Count + Count, 0);
	Info.Delta = Info.Count == 0 ? -OldCount : Count;
	Info.Tag = RedDotTag;
	Info.TriggerTime = FDateTime::Now();
	
	// 应用更新, 广播更新事件
	UpdateRedDotState(RedDotTag, Info);
}

bool UYcRedDotManagerSubsystem::GetRedDotInfo(const FGameplayTag& RedDotTag, FRedDotInfo& InOutInfo)
{
	if (const FRedDotInfo* Info = RedDotStates.Find(RedDotTag))
	{
		InOutInfo = *Info;
		return true;
	}
    
	return false;
}

bool UYcRedDotManagerSubsystem::IsRedDotActive(const FGameplayTag& RedDotTag) const
{
	if (const FRedDotInfo* Info = RedDotStates.Find(RedDotTag))
	{
		return Info->Count > 0;
	}
    
	return false;
}

TArray<FGameplayTag> UYcRedDotManagerSubsystem::GetAllActiveRedDotTags() const
{
	TArray<FGameplayTag> ActiveTags;
    
	for (const auto& Pair : RedDotStates)
	{
		if (Pair.Value.Count > 0)
		{
			ActiveTags.Add(Pair.Key);
		}
	}
    
	return ActiveTags;
}

void UYcRedDotManagerSubsystem::GetAllRedDotInfos(TArray<FRedDotInfo>& InOutRedDots)
{
	for (const auto& Pair : RedDotStates)
	{
		InOutRedDots.Add(Pair.Value);
	}
}

void UYcRedDotManagerSubsystem::ClearRedDotStateInBranch(const FGameplayTag& ParentTag)
{
	if (!ParentTag.IsValid()) return;
	
	// 父节点向上更新穿透数据
	FRedDotInfo& ParentInfo = GetOrCreateRedDotInfo(ParentTag);
	ParentInfo.Tag = ParentTag;
	ParentInfo.Delta = -ParentInfo.Count;
	ParentInfo.Count = 0;
	ParentInfo.TriggerTime = FDateTime::Now();
	UpdateRedDotState(ParentTag, ParentInfo);
	BroadcastRedDotCleared(ParentTag);
	
	TArray<FGameplayTag> TagsToUpdate;
    
	// 添加父标签
	TagsToUpdate.Add(ParentTag);
	
	// 添加所有子标签
	TArray<FGameplayTag> ChildTags = FindAllChildTags(ParentTag);
	TagsToUpdate.Append(ChildTags);
	
	// 子节点全部直接置零,并调用回调通知更新RedDotState
	for (const FGameplayTag& Tag : ChildTags)
	{
		FRedDotInfo& Info = GetOrCreateRedDotInfo(Tag);
		Info.Tag = Tag;
		Info.Delta = 0;
		Info.Count = 0;
		Info.TriggerTime = FDateTime::Now();
		BroadcastRedDotChangedSingle(Tag, &Info);
		BroadcastRedDotCleared(Tag);
	}
}

FYcRedDotStateChangedListenerHandle UYcRedDotManagerSubsystem::RegisterRedDotStateChangedListener(FGameplayTag RedDotTag,
	TFunction<void(FGameplayTag, const FRedDotInfo*)>&& Callback, EYcRedDotTagMatch MatchType,
	bool bUnregisterOnWorldDestroyed)
{
	FYcRedDotListenerList& List = ListenerMap.FindOrAdd(RedDotTag);

	FYcRedDotStateChangedListenerData& Entry = List.Listeners.AddDefaulted_GetRef();
	Entry.bUnregisterOnWorldDestroyed = bUnregisterOnWorldDestroyed;
	Entry.ReceivedCallback = MoveTemp(Callback);
	Entry.HandleID = ++List.HandleID;
	Entry.MatchType = MatchType;

	FYcRedDotStateChangedListenerHandle Handle = FYcRedDotStateChangedListenerHandle(this, RedDotTag, Entry.HandleID);

	return Handle;
}

void UYcRedDotManagerSubsystem::BroadcastRedDotChangedForward(FGameplayTag RedDotTag, const FRedDotInfo* RedDotInfo)
{
	// 标记是否为初始标签, 因为我们会在循环中向前寻找父标签， 只有在第一次循环才是使用的初始标签
	bool bOnInitialTag = true;
	int Delta = RedDotInfo->Delta;
	for (FGameplayTag Tag = RedDotTag; Tag.IsValid(); Tag = Tag.RequestDirectParent())
	{
		const FYcRedDotListenerList* pList = ListenerMap.Find(Tag);
		FRedDotInfo* ParentRedDotInfo =  RedDotStates.Find(Tag);
		if (pList == nullptr || ParentRedDotInfo == nullptr) continue;
		if (!bOnInitialTag)
		{
			ParentRedDotInfo->Count += Delta;
		}
		// Copy in case there are removals while handling callbacks
		TArray<FYcRedDotStateChangedListenerData> ListenerArray(pList->Listeners);
		// 遍历当前标签的监听列表
		for (const FYcRedDotStateChangedListenerData& Listener : ListenerArray)
		{
			if (!bOnInitialTag && Listener.MatchType != EYcRedDotTagMatch::PartialMatch) continue;
			
			if (Listener.ReceivedCallback.IsSet())
			{
				Listener.ReceivedCallback(RedDotTag, ParentRedDotInfo);
			}
		}
		bOnInitialTag = false; // 第一次循环结束及后续都保持初始标记为false, 代表正在遍历父级标签
	}
}

void UYcRedDotManagerSubsystem::BroadcastRedDotChangedSingle(FGameplayTag RedDotTag, const FRedDotInfo* RedDotInfo)
{
	const FYcRedDotListenerList* pList = ListenerMap.Find(RedDotTag);
	if (pList == nullptr) return;
	// Copy in case there are removals while handling callbacks
	TArray<FYcRedDotStateChangedListenerData> ListenerArray(pList->Listeners);
	// 遍历当前标签的监听列表
	for (const FYcRedDotStateChangedListenerData& Listener : ListenerArray)
	{
		if (Listener.ReceivedCallback.IsSet())
		{
			Listener.ReceivedCallback(RedDotTag, RedDotInfo);
		}
	}
}

void UYcRedDotManagerSubsystem::BroadcastRedDotCleared(FGameplayTag RedDotTag)
{
	const FYcRedDotListenerList* pList = ListenerMap.Find(RedDotTag);
	if (pList == nullptr) return;
	TArray<FYcRedRelierData> RedRelierArray(pList->Reliers);
	for (auto& Relier : RedRelierArray)
	{
		if (Relier.ClearedCallback.IsSet())
		{
			Relier.ClearedCallback();
		}
	}
}

FYcRedRelierListenerHandle UYcRedDotManagerSubsystem::RegisterRelier(FGameplayTag RedDotTag, TFunction<void()>&& Callback)
{
	FYcRedDotListenerList& List = ListenerMap.FindOrAdd(RedDotTag);
	
	FYcRedRelierData& Relier = List.Reliers.AddDefaulted_GetRef();
	Relier.ClearedCallback = Callback;
	Relier.RedDotTag = RedDotTag;
	Relier.HandleID = ++List.HandleID;
	FYcRedRelierListenerHandle Handle = FYcRedRelierListenerHandle(this, RedDotTag, Relier.HandleID);
	
	return Handle;
}

FRedDotInfo& UYcRedDotManagerSubsystem::GetOrCreateRedDotInfo(const FGameplayTag& RedDotTag)
{
	FRedDotInfo* Info = RedDotStates.Find(RedDotTag);
	if (!Info)
	{
		Info = &RedDotStates.Add(RedDotTag);
		Info->Tag = RedDotTag;
		Info->TriggerTime = FDateTime::Now();
	}
    
	return *Info;
}

void UYcRedDotManagerSubsystem::UpdateRedDotState(const FGameplayTag& RedDotTag, const FRedDotInfo& NewInfo)
{
	// 广播全局委托, 通知节点发生变化
	BroadcastRedDotChangedForward(RedDotTag, &NewInfo);
}

TArray<FGameplayTag> UYcRedDotManagerSubsystem::FindAllChildTags(const FGameplayTag& ParentTag) const
{
	// 检查缓存，如果存在直接返回
	if (const TArray<FGameplayTag>* CachedChildren = TagHierarchyCache.Find(ParentTag))
	{
		return *CachedChildren;
	}
    
	TArray<FGameplayTag> AllChildren;
    
	// 遍历所有标签，查找子标签
	for (const auto& Pair : RedDotStates)
	{
		if (Pair.Key.MatchesTag(ParentTag) && Pair.Key != ParentTag)
		{
			AllChildren.Add(Pair.Key);
		}
	}
    
	// 更新缓存
	const_cast<UYcRedDotManagerSubsystem*>(this)->TagHierarchyCache.Add(ParentTag, AllChildren);
	return AllChildren;
}

void UYcRedDotManagerSubsystem::InvalidateTagHierarchyCache(const FGameplayTag& NewTag)
{
	if (!NewTag.IsValid())
	{
		return;
	}

	// 遍历缓存，找出所有包含 NewTag 作为子标签的父标签缓存，并清理它们
	TArray<FGameplayTag> TagsToRemove;
	
	for (const auto& CachePair : TagHierarchyCache)
	{
		const FGameplayTag& CachedParentTag = CachePair.Key;
		
		// 检查 NewTag 是否是 CachedParentTag 的子标签
		// 如果 NewTag 匹配 CachedParentTag 且不相等，说明 NewTag 是 CachedParentTag 的子标签
		if (NewTag.MatchesTag(CachedParentTag) && NewTag != CachedParentTag)
		{
			// 标记该缓存为需要删除
			TagsToRemove.Add(CachedParentTag);
		}
	}
	
	// 删除所有需要清理的缓存
	for (const FGameplayTag& TagToRemove : TagsToRemove)
	{
		TagHierarchyCache.Remove(TagToRemove);
	}
}

void UYcRedDotManagerSubsystem::UnregisterStateChangedListener(FYcRedDotStateChangedListenerHandle Handle)
{
	if (FYcRedDotListenerList* pList = ListenerMap.Find(Handle.RedDotTag))
	{
		int32 MatchIndex = pList->Listeners.IndexOfByPredicate([ID = Handle.ID](const FYcRedDotStateChangedListenerData& Other) { return Other.HandleID == ID; });
		if (MatchIndex != INDEX_NONE)
		{
			pList->Listeners.RemoveAtSwap(MatchIndex);
		}

		if (pList->Listeners.Num() == 0)
		{
			ListenerMap.Remove(Handle.RedDotTag);
		}
	}
}

void UYcRedDotManagerSubsystem::UnregisterRelierListener(FYcRedRelierListenerHandle Handle)
{
	if (FYcRedDotListenerList* pList = ListenerMap.Find(Handle.RedDotTag))
	{
		int32 MatchIndex = pList->Reliers.IndexOfByPredicate([ID = Handle.ID](const FYcRedRelierData& Other) { return Other.HandleID == ID; });
		if (MatchIndex != INDEX_NONE)
		{
			pList->Reliers.RemoveAtSwap(MatchIndex);
		}
		
		
		if (pList->Reliers.Num() == 0)
		{
			ListenerMap.Remove(Handle.RedDotTag);
		}
	}
}
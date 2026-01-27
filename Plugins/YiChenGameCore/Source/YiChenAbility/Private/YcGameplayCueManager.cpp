// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGameplayCueManager.h"

#include "AbilitySystemGlobals.h"
#include "GameplayCueSet.h"
#include "GameplayTagsManager.h"
#include "YiChenAbility.h"
#include "Engine/AssetManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayCueManager)

//////////////////////////////////////////////////////////////////////

/**
 * 编辑器加载模式枚举
 * 控制 GameplayCue 的加载策略
 */
enum class EYcEditorLoadMode
{
	/** 前置加载：在启动时加载所有 Cue，编辑器加载慢但 PIE 快，效果稳定 */
	LoadUpfront,

	/** 仅游戏模式预加载：编辑器中按需加载，游戏中按引用预加载，编辑器迭代快但可能看不到效果 */
	PreloadAsCuesAreReferenced_GameOnly,

	/** 始终预加载：编辑器和游戏中都按引用预加载，内存占用低但首次播放可能有延迟 */
	PreloadAsCuesAreReferenced
};

namespace YcGameplayCueManagerCvars
{
	/** 控制台命令：转储当前加载的 GameplayCue 信息 */
	static FAutoConsoleCommand CVarDumpGameplayCues(
		TEXT("Yc.DumpGameplayCues"),
		TEXT("Shows all assets that were loaded via SCGameplayCueManager and are currently in memory."),
		FConsoleCommandWithArgsDelegate::CreateStatic(UYcGameplayCueManager::DumpGameplayCues));

	/** 当前使用的加载模式，默认为前置加载 */
	static EYcEditorLoadMode LoadMode = EYcEditorLoadMode::LoadUpfront;
}

/** 是否在编辑器中也进行预加载 */
const bool bPreloadEvenInEditor = true;

//////////////////////////////////////////////////////////////////////

/**
 * GameplayCue Tag 线程同步任务
 * 用于将其他线程的 GameplayTag 加载事件同步到 GameThread 处理
 */
struct FGameplayCueTagThreadSynchronizeGraphTask : public FAsyncGraphTaskBase
{
	TFunction<void()> TheTask;
	FGameplayCueTagThreadSynchronizeGraphTask(TFunction<void()>&& Task) : TheTask(MoveTemp(Task)) { }
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) { TheTask(); }
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
};

//////////////////////////////////////////////////////////////////////

UYcGameplayCueManager::UYcGameplayCueManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UYcGameplayCueManager* UYcGameplayCueManager::Get()
{
	return Cast<UYcGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void UYcGameplayCueManager::OnCreated()
{
	Super::OnCreated();

	UpdateDelayLoadDelegateListeners();
}

void UYcGameplayCueManager::LoadAlwaysLoadedCues()
{
	if (ShouldDelayLoadGameplayCues())
	{
		UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	
		// @TODO: 尝试通过过滤原生 GameplayTag 中的 GameplayCue. 标签来收集这些 Cue
		TArray<FName> AdditionalAlwaysLoadedCueTags;

		// 遍历配置的始终加载 Cue 标签列表
		for (const FName& CueTagName : AdditionalAlwaysLoadedCueTags)
		{
			FGameplayTag CueTag = TagManager.RequestGameplayTag(CueTagName, /*ErrorIfNotFound=*/ false);
			if (CueTag.IsValid())
			{
				// 预加载该 Cue，OwningObject 为 nullptr 表示这是 AlwaysLoaded Cue
				ProcessTagToPreload(CueTag, nullptr);
			}
			else
			{
				UE_LOG(LogYcAbilitySystem, Warning, TEXT("UYcGameplayCueManager::AdditionalAlwaysLoadedCueTags contains invalid tag %s"), *CueTagName.ToString());
			}
		}
	}
}

bool UYcGameplayCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
{
	switch (YcGameplayCueManagerCvars::LoadMode)
	{
	case EYcEditorLoadMode::LoadUpfront:
		// 前置加载模式：返回 true，在启动时异步加载所有 Cue
		return true;
	case EYcEditorLoadMode::PreloadAsCuesAreReferenced_GameOnly:
#if WITH_EDITOR
		// 编辑器中不预加载，返回 false
		if (GIsEditor)
		{
			return false;
		}
#endif
		break;
	case EYcEditorLoadMode::PreloadAsCuesAreReferenced:
		break;
	}

	// 如果不延迟加载，则返回 false（专用服务器）
	return !ShouldDelayLoadGameplayCues();
}

bool UYcGameplayCueManager::ShouldSyncLoadMissingGameplayCues() const
{
	return false;
}

bool UYcGameplayCueManager::ShouldAsyncLoadMissingGameplayCues() const
{
	return true;
}

void UYcGameplayCueManager::DumpGameplayCues(const TArray<FString>& Args)
{
	UYcGameplayCueManager* GCM = Cast<UYcGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
	if (!GCM)
	{
		UE_LOG(LogYcAbilitySystem, Error, TEXT("DumpGameplayCues failed. No UYcGameplayCueManager found."));
		return;
	}

	const bool bIncludeRefs = Args.Contains(TEXT("Refs"));

	UE_LOG(LogYcAbilitySystem, Log, TEXT("=========== Dumping Always Loaded Gameplay Cue Notifies ==========="));
	for (UClass* CueClass : GCM->AlwaysLoadedCues)
	{
		UE_LOG(LogYcAbilitySystem, Log, TEXT("  %s"), *GetPathNameSafe(CueClass));
	}

	UE_LOG(LogYcAbilitySystem, Log, TEXT("=========== Dumping Preloaded Gameplay Cue Notifies ==========="));
	for (UClass* CueClass : GCM->PreloadedCues)
	{
		TSet<FObjectKey>* ReferencerSet = GCM->PreloadedCueReferencers.Find(CueClass);
		int32 NumRefs = ReferencerSet ? ReferencerSet->Num() : 0;
		UE_LOG(LogYcAbilitySystem, Log, TEXT("  %s (%d refs)"), *GetPathNameSafe(CueClass), NumRefs);
		if (bIncludeRefs && ReferencerSet)
		{
			for (const FObjectKey& Ref : *ReferencerSet)
			{
				UObject* RefObject = Ref.ResolveObjectPtr();
				UE_LOG(LogYcAbilitySystem, Log, TEXT("    ^- %s"), *GetPathNameSafe(RefObject));
			}
		}
	}

	UE_LOG(LogYcAbilitySystem, Log, TEXT("=========== Dumping Gameplay Cue Notifies loaded on demand ==========="));
	int32 NumMissingCuesLoaded = 0;
	if (GCM->RuntimeGameplayCueObjectLibrary.CueSet)
	{
		for (const FGameplayCueNotifyData& CueData : GCM->RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData)
		{
			if (CueData.LoadedGameplayCueClass && !GCM->AlwaysLoadedCues.Contains(CueData.LoadedGameplayCueClass) && !GCM->PreloadedCues.Contains(CueData.LoadedGameplayCueClass))
			{
				NumMissingCuesLoaded++;
				UE_LOG(LogYcAbilitySystem, Log, TEXT("  %s"), *CueData.LoadedGameplayCueClass->GetPathName());
			}
		}
	}

	UE_LOG(LogYcAbilitySystem, Log, TEXT("=========== Gameplay Cue Notify summary ==========="));
	UE_LOG(LogYcAbilitySystem, Log, TEXT("  ... %d cues in always loaded list"), GCM->AlwaysLoadedCues.Num());
	UE_LOG(LogYcAbilitySystem, Log, TEXT("  ... %d cues in preloaded list"), GCM->PreloadedCues.Num());
	UE_LOG(LogYcAbilitySystem, Log, TEXT("  ... %d cues loaded on demand"), NumMissingCuesLoaded);
	UE_LOG(LogYcAbilitySystem, Log, TEXT("  ... %d cues in total"), GCM->AlwaysLoadedCues.Num() + GCM->PreloadedCues.Num() + NumMissingCuesLoaded);
}

void UYcGameplayCueManager::OnGameplayTagLoaded(const FGameplayTag& Tag)
{
	// 线程安全：使用临界区保护共享数据
	FScopeLock ScopeLock(&LoadedGameplayTagsToProcessCS);
	bool bStartTask = LoadedGameplayTagsToProcess.Num() == 0;
	
	// 获取当前序列化上下文，找到引用该 Tag 的对象
	FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
	UObject* OwningObject = LoadContext ? LoadContext->SerializedObject : nullptr;
	
	// 将 Tag 和引用对象添加到待处理列表
	LoadedGameplayTagsToProcess.Emplace(Tag, OwningObject);
	
	// 如果这是第一个待处理的 Tag，创建 GameThread 同步任务
	if (bStartTask)
	{
		TGraphTask<FGameplayCueTagThreadSynchronizeGraphTask>::CreateTask().ConstructAndDispatchWhenReady([]()
			{
				if (GIsRunning)
				{
					if (UYcGameplayCueManager* StrongThis = Get())
					{
						// 如果正在进行垃圾回收，不能调用 StaticFindObject，延迟到 GC 后处理
						if (IsGarbageCollecting())
						{
							StrongThis->bProcessLoadedTagsAfterGC = true;
						}
						else
						{
							StrongThis->ProcessLoadedTags();
						}
					}
				}
			});
	}
}

void UYcGameplayCueManager::HandlePostGarbageCollect()
{
	if (bProcessLoadedTagsAfterGC)
	{
		ProcessLoadedTags();
	}
	bProcessLoadedTagsAfterGC = false;
}

void UYcGameplayCueManager::ProcessLoadedTags()
{
	TArray<FLoadedGameplayTagToProcessData> TaskLoadedGameplayTagsToProcess;
	{
		// 锁定 LoadedGameplayTagsToProcess 仅用于复制和清空，最小化锁定时间
		FScopeLock TaskScopeLock(&LoadedGameplayTagsToProcessCS);
		TaskLoadedGameplayTagsToProcess = LoadedGameplayTagsToProcess;
		LoadedGameplayTagsToProcess.Empty();
	}

	// 在关闭期间可能返回，此时不应继续处理
	if (GIsRunning)
	{
		if (RuntimeGameplayCueObjectLibrary.CueSet)
		{
			// 遍历所有待处理的 Tag
			for (const FLoadedGameplayTagToProcessData& LoadedTagData : TaskLoadedGameplayTagsToProcess)
			{
				// 检查该 Tag 是否在 GameplayCue 数据映射中
				if (RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueDataMap.Contains(LoadedTagData.Tag))
				{
					// 如果引用对象仍然有效，处理该 Tag 的预加载
					if (!LoadedTagData.WeakOwner.IsStale())
					{
						ProcessTagToPreload(LoadedTagData.Tag, LoadedTagData.WeakOwner.Get());
					}
				}
			}
		}
		else
		{
			UE_LOG(LogYcAbilitySystem, Warning, TEXT("UYcGameplayCueManager::OnGameplayTagLoaded processed loaded tag(s) but RuntimeGameplayCueObjectLibrary.CueSet was null. Skipping processing."));
		}
	}
}

void UYcGameplayCueManager::ProcessTagToPreload(const FGameplayTag& Tag, UObject* OwningObject)
{
	// 根据加载模式决定是否需要预加载
	switch (YcGameplayCueManagerCvars::LoadMode)
	{
	case EYcEditorLoadMode::LoadUpfront:
		// 前置加载模式下，所有 Cue 已在启动时加载，无需预加载
		return;
	case EYcEditorLoadMode::PreloadAsCuesAreReferenced_GameOnly:
#if WITH_EDITOR
		// 编辑器中不预加载
		if (GIsEditor)
		{
			return;
		}
#endif
		break;
	case EYcEditorLoadMode::PreloadAsCuesAreReferenced:
		break;
	}

	check(RuntimeGameplayCueObjectLibrary.CueSet);

	// 在 GameplayCue 数据映射中查找该 Tag
	int32* DataIdx = RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueDataMap.Find(Tag);
	if (DataIdx && RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData.IsValidIndex(*DataIdx))
	{
		const FGameplayCueNotifyData& CueData = RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData[*DataIdx];

		// 尝试查找该 Cue 是否已在内存中
		UClass* LoadedGameplayCueClass = FindObject<UClass>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
		if (LoadedGameplayCueClass)
		{
			// 已加载，直接注册
			RegisterPreloadedCue(LoadedGameplayCueClass, OwningObject);
		}
		else
		{
			// 未加载，异步加载该 Cue
			bool bAlwaysLoadedCue = OwningObject == nullptr;  // nullptr 表示 AlwaysLoaded
			TWeakObjectPtr<UObject> WeakOwner = OwningObject;
			StreamableManager.RequestAsyncLoad(
				CueData.GameplayCueNotifyObj, 
				FStreamableDelegate::CreateUObject(this, &ThisClass::OnPreloadCueComplete, CueData.GameplayCueNotifyObj, WeakOwner, bAlwaysLoadedCue), 
				FStreamableManager::DefaultAsyncLoadPriority, 
				false, 
				false, 
				TEXT("GameplayCueManager"));
		}
	}
}

void UYcGameplayCueManager::OnPreloadCueComplete(FSoftObjectPath Path, TWeakObjectPtr<UObject> OwningObject, bool bAlwaysLoadedCue)
{
	if (bAlwaysLoadedCue || OwningObject.IsValid())
	{
		if (UClass* LoadedGameplayCueClass = Cast<UClass>(Path.ResolveObject()))
		{
			RegisterPreloadedCue(LoadedGameplayCueClass, OwningObject.Get());
		}
	}
}

void UYcGameplayCueManager::RegisterPreloadedCue(UClass* LoadedGameplayCueClass, UObject* OwningObject)
{
	check(LoadedGameplayCueClass);

	const bool bAlwaysLoadedCue = OwningObject == nullptr;
	if (bAlwaysLoadedCue)
	{
		// AlwaysLoaded Cue：添加到 AlwaysLoadedCues，从 PreloadedCues 中移除
		AlwaysLoadedCues.Add(LoadedGameplayCueClass);
		PreloadedCues.Remove(LoadedGameplayCueClass);
		PreloadedCueReferencers.Remove(LoadedGameplayCueClass);
	}
	else if ((OwningObject != LoadedGameplayCueClass) && 
	         (OwningObject != LoadedGameplayCueClass->GetDefaultObject()) && 
	         !AlwaysLoadedCues.Contains(LoadedGameplayCueClass))
	{
		// Preloaded Cue：添加到 PreloadedCues，记录引用者
		// 排除自引用和 CDO 引用，已在 AlwaysLoadedCues 中的不重复添加
		PreloadedCues.Add(LoadedGameplayCueClass);
		TSet<FObjectKey>& ReferencerSet = PreloadedCueReferencers.FindOrAdd(LoadedGameplayCueClass);
		ReferencerSet.Add(OwningObject);
	}
}

void UYcGameplayCueManager::HandlePostLoadMap(UWorld* NewWorld)
{
	// 地图加载后，从运行时库中移除已加载的 Cue，避免重复
	if (RuntimeGameplayCueObjectLibrary.CueSet)
	{
		for (UClass* CueClass : AlwaysLoadedCues)
		{
			RuntimeGameplayCueObjectLibrary.CueSet->RemoveLoadedClass(CueClass);
		}

		for (UClass* CueClass : PreloadedCues)
		{
			RuntimeGameplayCueObjectLibrary.CueSet->RemoveLoadedClass(CueClass);
		}
	}

	// 清理无效的预加载 Cue 引用
	for (auto CueIt = PreloadedCues.CreateIterator(); CueIt; ++CueIt)
	{
		TSet<FObjectKey>& ReferencerSet = PreloadedCueReferencers.FindChecked(*CueIt);
		
		// 移除已失效的引用者
		for (auto RefIt = ReferencerSet.CreateIterator(); RefIt; ++RefIt)
		{
			if (!RefIt->ResolveObjectPtr())
			{
				RefIt.RemoveCurrent();
			}
		}
		
		// 如果没有引用者了，移除该 Cue
		if (ReferencerSet.Num() == 0)
		{
			PreloadedCueReferencers.Remove(*CueIt);
			CueIt.RemoveCurrent();
		}
	}
}

void UYcGameplayCueManager::UpdateDelayLoadDelegateListeners()
{
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.RemoveAll(this);
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	switch (YcGameplayCueManagerCvars::LoadMode)
	{
	case EYcEditorLoadMode::LoadUpfront:
		return;
	case EYcEditorLoadMode::PreloadAsCuesAreReferenced_GameOnly:
#if WITH_EDITOR
		if (GIsEditor)
		{
			return;
		}
#endif
		break;
	case EYcEditorLoadMode::PreloadAsCuesAreReferenced:
		break;
	}

	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.AddUObject(this, &ThisClass::OnGameplayTagLoaded);
	FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ThisClass::HandlePostGarbageCollect);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
}

bool UYcGameplayCueManager::ShouldDelayLoadGameplayCues() const
{
	const bool bClientDelayLoadGameplayCues = true;
	return !IsRunningDedicatedServer() && bClientDelayLoadGameplayCues;
}

const FPrimaryAssetType UFortAssetManager_GameplayCueRefsType = TEXT("GameplayCueRefs");
const FName UFortAssetManager_GameplayCueRefsName = TEXT("GameplayCueReferences");
const FName UFortAssetManager_LoadStateClient = FName(TEXT("Client"));

void UYcGameplayCueManager::RefreshGameplayCuePrimaryAsset()
{
	// 收集所有 GameplayCue 的软对象路径
	TArray<FSoftObjectPath> CuePaths;
	UGameplayCueSet* RuntimeGameplayCueSet = GetRuntimeCueSet();
	if (RuntimeGameplayCueSet)
	{
		RuntimeGameplayCueSet->GetSoftObjectPaths(CuePaths);
	}

	// 创建资产捆绑数据，将 Cue 路径添加到客户端加载状态
	FAssetBundleData BundleData;
	BundleData.AddBundleAssetsTruncated(UFortAssetManager_LoadStateClient, CuePaths);

	// 更新 AssetManager 中的动态资产
	FPrimaryAssetId PrimaryAssetId = FPrimaryAssetId(UFortAssetManager_GameplayCueRefsType, UFortAssetManager_GameplayCueRefsName);
	UAssetManager::Get().AddDynamicAsset(PrimaryAssetId, FSoftObjectPath(), BundleData);
}
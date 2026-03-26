// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "GameFeatures/Actions/YcGameFeatureAction_PreloadItems.h"
#include "YiChenGameplay.h"
#include "System/YcAssetManager.h"
#include "System/YcDataRegistryAssetResolver.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "Engine/StreamableManager.h"
#include "Misc/DataValidation.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_PreloadItems)

void UYcGameFeatureAction_PreloadItems::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	// 清理上下文数据
	if (FPerContextData* ContextDataPtr = ContextData.Find(Context))
	{
		Reset(*ContextDataPtr);
		ContextData.Remove(Context);
	}
	
	Super::OnGameFeatureDeactivating(Context);
}

void UYcGameFeatureAction_PreloadItems::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	// 获取或创建上下文数据
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);
	
	// 开始预加载
	StartPreload(ChangeContext, WorldContext);
}

#if WITH_EDITOR
EDataValidationResult UYcGameFeatureAction_PreloadItems::IsDataValid(FDataValidationContext& Context) const
{
	// 检查是否有预加载条目
	if (ItemsToPreload.Num() == 0)
	{
		Context.AddError(FText::FromString(TEXT("ItemsToPreload 为空，没有配置任何预加载条目")));
		return EDataValidationResult::Invalid;
	}
	
	// 检查每个条目的 DataRegistryId 是否有效
	for (int32 i = 0; i < ItemsToPreload.Num(); ++i)
	{
		const FYcPreloadItemEntry& Entry = ItemsToPreload[i];
		if (!Entry.DataRegistryId.IsValid())
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("ItemsToPreload[%d] 的 DataRegistryId 无效"), i)));
			return EDataValidationResult::Invalid;
		}
	}
	
	return EDataValidationResult::Valid;
}
#endif

void UYcGameFeatureAction_PreloadItems::TryRegisterActionPauser(const FWorldContext& WorldContext, FGameFeatureStateChangeContext Context)
{
	if (!bBlockActivation)
	{
		return;
	}
	
	const UWorld* World = WorldContext.World();
	if (!World)
	{
		return;
	}
	
	const AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}
	
	UYcExperienceManagerComponent* ExperienceManager = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
	if (!ExperienceManager)
	{
		return;
	}
	
	ExperienceManager->RegisterActionPauser();
	
	// 保存引用用于后续取消注册
	if (FPerContextData* ContextDataPtr = ContextData.Find(Context))
	{
		ContextDataPtr->RegisteredExperienceManager = ExperienceManager;
	}
	
	UE_LOG(LogYcGameplay, Verbose, TEXT("PreloadItems: 已注册 Experience 阻塞器"));
}

void UYcGameFeatureAction_PreloadItems::StartPreload(FGameFeatureStateChangeContext Context, const FWorldContext& WorldContext)
{
	// 收集需要预加载的资产
	TArray<FPrimaryAssetId> AssetIds;
	TArray<FName> BundleNames;
	
	if (!CollectAssetsToPreload(AssetIds, BundleNames))
	{
		UE_LOG(LogYcGameplay, Warning, TEXT("PreloadItems: 收集资产失败，跳过预加载"));
		
		// 标记为已完成
		if (FPerContextData* ContextDataPtr = ContextData.Find(Context))
		{
			ContextDataPtr->bLoadComplete = true;
			ContextDataPtr->bLoadSuccess = false;
		}
		return;
	}
	
	if (AssetIds.Num() == 0)
	{
		UE_LOG(LogYcGameplay, Log, TEXT("PreloadItems: 没有资产需要预加载"));
		
		// 标记为已完成
		if (FPerContextData* ContextDataPtr = ContextData.Find(Context))
		{
			ContextDataPtr->bLoadComplete = true;
			ContextDataPtr->bLoadSuccess = true;
		}
		return;
	}
	
	UE_LOG(LogYcGameplay, Log, TEXT("PreloadItems: 开始预加载 %d 个资产，Bundle: %d"),
		AssetIds.Num(), BundleNames.Num());
	
	// 如果需要阻塞，注册到 ExperienceManager 阻塞状态机
	TryRegisterActionPauser(WorldContext, Context);
	
	// 获取 AssetManager
	UYcAssetManager& AssetManager = UYcAssetManager::Get();
	
	// 进度回调
	FStreamableUpdateDelegate LoadingProgressCallback = FStreamableUpdateDelegate::CreateLambda([this](const TSharedRef<FStreamableHandle>& Handle)
	{
		float LoadingProgress = Handle->GetProgress();
		
		//已加载资源数
		int32 LoadedCount = 0;
		//总需要加载的资源数
		int32 RequestedCount = 0;
		Handle->GetLoadedCount(LoadedCount, RequestedCount);
		
		// @TODO 资源的加载进度信息是否需要传递出去呢？
		UE_LOG(LogYcGameplay, Verbose, TEXT("加载的进度为：%f"), LoadingProgress);
		UE_LOG(LogYcGameplay, Verbose, TEXT("已经加载的资源数量为：%d . 总共要加载的资源数量为：%d"), 
			   LoadedCount, RequestedCount);
	});
	
	// 创建完成回调
	FStreamableDelegate OnComplete = FStreamableDelegate::CreateLambda(
		[this, Context]()
		{
			UE_LOG(LogYcGameplay, Verbose, TEXT("PreloadItems: 预加载完成"));
			
			// 更新上下文数据
			if (FPerContextData* ContextDataPtr = ContextData.Find(Context))
			{
				ContextDataPtr->bLoadComplete = true;
				ContextDataPtr->bLoadSuccess = true;
				
				// 从 ExperienceManager 取消注册阻塞器
				if (UYcExperienceManagerComponent* ExperienceManager = ContextDataPtr->RegisteredExperienceManager.Get())
				{
					ExperienceManager->UnregisterActionPauser();
					ContextDataPtr->RegisteredExperienceManager.Reset();
					UE_LOG(LogYcGameplay, Verbose, TEXT("PreloadItems: 已取消 Experience 阻塞器"));
				}
			}
		});
	
	// 开始异步加载
	TSharedPtr<FStreamableHandle> Handle = AssetManager.PreloadPrimaryAssets(
		AssetIds, BundleNames, OnComplete, LoadingProgressCallback);
	
	// 保存加载句柄
	if (Handle.IsValid())
	{
		if (FPerContextData* ContextDataPtr = ContextData.Find(Context))
		{
			ContextDataPtr->ActiveHandles.Add(Handle);
		}
	}
	else
	{
		// 加载已立即完成或失败
		OnComplete.ExecuteIfBound();
	}
}

bool UYcGameFeatureAction_PreloadItems::CollectAssetsToPreload(
	TArray<FPrimaryAssetId>& OutAssetIds,
	TArray<FName>& OutBundleNames)
{
	// 获取 AssetManager 和解析器
	UYcAssetManager& AssetManager = UYcAssetManager::Get();
	TSharedPtr<IYcDataRegistryAssetResolver> Resolver = AssetManager.GetDataRegistryResolver();
	
	if (!Resolver.IsValid())
	{
		UE_LOG(LogYcGameplay, Warning, 
			TEXT("PreloadItems: DataRegistry 解析器未注册，无法解析 DataRegistryId"));
		return false;
	}
	
	// 收集所有资产
	TSet<FPrimaryAssetId> UniqueAssetIds;
	TSet<FName> UniqueBundleNames;
	
	for (const FYcPreloadItemEntry& Entry : ItemsToPreload)
	{
		// 解析 DataRegistryId 到 PrimaryAssetId
		TArray<FPrimaryAssetId> ResolvedAssetIds;
		if (Resolver->ResolveAssets(Entry.DataRegistryId, ResolvedAssetIds))
		{
			for (const FPrimaryAssetId& AssetId : ResolvedAssetIds)
			{
				UniqueAssetIds.Add(AssetId);
			}
		}
		else
		{
			UE_LOG(LogYcGameplay, Warning, 
				TEXT("PreloadItems: 解析 DataRegistryId %s 失败"),
				*Entry.DataRegistryId.ToString());
		}
		
		// 收集配置中的 Bundle 名称
		if (Entry.BundleNames.Num() > 0)
		{
			// 使用条目中指定的 Bundle
			for (const FName& Bundle : Entry.BundleNames)
			{
				UniqueBundleNames.Add(Bundle);
			}
		}
		
		// 追加接口返回的 Bundle
		TArray<FName> Bundles;
		Resolver->GetBundleNames(Entry.DataRegistryId, Bundles);
		for (const FName& Bundle : Bundles)
		{
			UniqueBundleNames.Add(Bundle);
		}
	}
	
	// 如果条目中没有指定 Bundle，使用全局默认值
	if (UniqueBundleNames.Num() == 0 && DefaultBundleNames.Num() > 0)
	{
		for (const FName& Bundle : DefaultBundleNames)
		{
			UniqueBundleNames.Add(Bundle);
		}
	}
	
	// 转换为数组输出
	OutAssetIds = UniqueAssetIds.Array();
	OutBundleNames = UniqueBundleNames.Array();
	
	return true;
}

void UYcGameFeatureAction_PreloadItems::Reset(FPerContextData& ContextDataRef)
{
	// 取消所有活跃的加载句柄
	for (TSharedPtr<FStreamableHandle>& Handle : ContextDataRef.ActiveHandles)
	{
		if (Handle.IsValid())
		{
			Handle->CancelHandle();
		}
	}
	
	// 从 ExperienceManager 取消注册阻塞器
	if (UYcExperienceManagerComponent* ExperienceManager = ContextDataRef.RegisteredExperienceManager.Get())
	{
		ExperienceManager->UnregisterActionPauser();
		ContextDataRef.RegisteredExperienceManager.Reset();
	}
	
	ContextDataRef.ActiveHandles.Empty();
	ContextDataRef.bLoadComplete = false;
	ContextDataRef.bLoadSuccess = false;
}

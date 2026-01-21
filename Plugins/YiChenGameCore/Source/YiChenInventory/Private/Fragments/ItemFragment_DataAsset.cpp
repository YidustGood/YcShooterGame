// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Fragments/ItemFragment_DataAsset.h"

#include "YcInventoryItemInstance.h"
#include "YiChenInventory.h"
#include "Engine/AssetManager.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemFragment_DataAsset)

TSharedPtr<struct FStreamableHandle> FYcDataAssetEntry::LoadDataAssetAsync(const FStreamableDelegate& OnLoadCompleted, bool bExecuteIfAlreadyLoaded) const
{
	if (!DataAssetId.IsValid())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("FYcDataAssetEntry::LoadDataAssetAsync - Invalid DataAssetId"));
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	
	// 检查是否已加载
	if (AssetManager.GetPrimaryAssetObject(DataAssetId))
	{
		UE_LOG(LogYcInventory, Verbose, TEXT("DataAsset already loaded: %s"), *DataAssetId.ToString());
		
		// 如果已加载且需要执行回调，则立即执行
		if (bExecuteIfAlreadyLoaded && OnLoadCompleted.IsBound())
		{
			OnLoadCompleted.Execute();
		}
		
		return nullptr;
	}

	UE_LOG(LogYcInventory, Log, TEXT("Loading DataAsset: %s with %d bundles"), 
		*DataAssetId.ToString(), AssetBundleNames.Num());
	
	// 如果资产还在异步加载过程中依旧会执行到这里, 但是不会由什么大问题, 就是每次调用传递的回调都会被调用
	return AssetManager.LoadPrimaryAsset(DataAssetId, AssetBundleNames, OnLoadCompleted);
}

void FYcDataAssetEntry::UnloadDataAsset(UObject* InRelatedObject, FGameplayTag AssetTag) const
{
	if (!DataAssetId.IsValid())
	{
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.UnloadPrimaryAsset(DataAssetId);
	
	// 广播卸载消息
	if (InRelatedObject)
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(InRelatedObject);
		
		FYcDataAssetLifecycleMessage Message;
		Message.LoadedDataAsset = nullptr;
		Message.AssetTag = AssetTag;
		Message.RelatedObject = InRelatedObject;
		Message.bIsLoaded = false;
		
		MessageSubsystem.BroadcastMessage(AssetTag, Message);
	}
	
	UE_LOG(LogYcInventory, Log, TEXT("Unloaded YcDataAssetEntry: %s"), *DataAssetId.ToString());
}

UPrimaryDataAsset* FYcDataAssetEntry::GetLoadedDataAsset() const
{
	if (!DataAssetId.IsValid())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	return Cast<UPrimaryDataAsset>(AssetManager.GetPrimaryAssetObject(DataAssetId));
}

bool FYcDataAssetEntry::IsDataAssetLoaded() const
{
	return GetLoadedDataAsset() != nullptr;
}

void FItemFragment_DataAsset::OnInstanceCreated(UYcInventoryItemInstance* Instance) const
{
	FYcInventoryItemFragment::OnInstanceCreated(Instance);
	
	// @TODO 每个ItemInstance创都会调用LoadAllDataAssetAsync(), 虽然会跳过已经加载的, 但是还是会产生DataAssetMap的遍历开销, 是否需要考虑优化?
	LoadAllDataAssetAsync(Instance);
}

void FItemFragment_DataAsset::LoadAllDataAssetAsync(UObject* InRelatedObject) const
{
	if (!InRelatedObject)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("FItemFragment_DataAsset::LoadAllDataAssetAsync - InRelatedObject is null"));
		return;
	}
	
	// 遍历所有数据资产映射
	for (const auto& Pair : DataAssetMap)
	{
		const FYcDataAssetEntry& Entry = Pair.Value;
		
		// 如果资产未加载且标记了自动加载，则进行异步加载
		if (!Entry.IsDataAssetLoaded() && Entry.bAutoLoad)
		{
			const FGameplayTag AssetTag = Pair.Key;
			const FPrimaryAssetId AssetId = Entry.DataAssetId;
			
			FStreamableDelegate OnLoadCompleted;
			OnLoadCompleted.BindLambda([InRelatedObject, AssetTag, AssetId]()
			{
				// 加载完成后广播全局消息
				UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(InRelatedObject);
				
				FYcDataAssetLifecycleMessage Message;
				Message.LoadedDataAsset = Cast<UPrimaryDataAsset>(UAssetManager::Get().GetPrimaryAssetObject(AssetId));
				Message.AssetTag = AssetTag;
				Message.RelatedObject = InRelatedObject;
				Message.bIsLoaded = true;
				
				MessageSubsystem.BroadcastMessage(AssetTag, Message);
				
				UE_LOG(LogYcInventory, Log, TEXT("YcDataAsset loaded and message broadcasted: %s"), *AssetTag.ToString());
			});
			
			// @TODO 如果在资产异步加载期间多次调用了LoadAllDataAssetAsync, 会广播多次资产加载完成消息, 这个看是否需要在加载端处理, 还是交给消息监听端自己处理
			// 异步加载数据资产
			Entry.LoadDataAssetAsync(OnLoadCompleted, false);
		}
	}
}

const FYcDataAssetEntry* FItemFragment_DataAsset::GetDataAssetByTag(FGameplayTag Tag) const
{
	return DataAssetMap.Find(Tag);
}

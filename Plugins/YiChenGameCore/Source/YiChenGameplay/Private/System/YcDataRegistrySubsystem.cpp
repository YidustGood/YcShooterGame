// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcDataRegistrySubsystem.h"

#include "DataRegistrySubsystem.h"
#include "YiChenGameplay.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDataRegistrySubsystem)

//~=============================================================================
// USubsystem Interface
//~=============================================================================

bool UYcDataRegistrySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UYcDataRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UYcDataRegistrySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

//~=============================================================================
// 核心查询接口
//~=============================================================================

bool UYcDataRegistrySubsystem::IsItemCached(const FDataRegistryId& ItemId)
{
	const UDataRegistrySubsystem* Subsystem = GetEngineSubsystem();
	if (!Subsystem)
	{
		return false;
	}
	
	const UScriptStruct* OutStruct = nullptr;
	const uint8* OutData = nullptr;
	const FDataRegistryCacheGetResult CacheResult = Subsystem->GetCachedItemRaw(OutData, OutStruct, ItemId);
	
	
	return CacheResult.WasFound() && OutData != nullptr;
}

//~=============================================================================
// 蓝图查询接口
//~=============================================================================

bool UYcDataRegistrySubsystem::GetAllRegistryIds(FDataRegistryType RegistryType, TArray<FDataRegistryId>& OutIds)
{
	const UDataRegistry* Registry = GetRegistry(RegistryType);
	if (!Registry)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcDataRegistrySubsystem::GetAllRegistryIds - Cannot find DataRegistry: %s"), 
			*RegistryType.ToString());
		return false;
	}
	
	Registry->GetPossibleRegistryIds(OutIds);
	return true;
}

void UYcDataRegistrySubsystem::FindCachedItem(const FDataRegistryId ItemId, FDataRegistryFindResult& OutResult)
{
	UDataRegistrySubsystem::FindCachedItemBP(ItemId, OutResult.OutResult, OutResult.OutItem);
}

bool UYcDataRegistrySubsystem::IsItemCachedBP(FDataRegistryId ItemId)
{
	return IsItemCached(ItemId);
}

bool UYcDataRegistrySubsystem::FindCachedPrimaryDataAssetId(FDataRegistryId ItemId, FYcPrimaryDataAssetIdWrapper& OutDataAssetId)
{
	const FYcPrimaryDataAssetIdWrapper* CachedWrapper = GetCachedItem<FYcPrimaryDataAssetIdWrapper>(ItemId);
	if (CachedWrapper)
	{
		OutDataAssetId.AssetId = CachedWrapper->AssetId;
		return true;
	}
	return false;
}

//~=============================================================================
// 异步加载接口
//~=============================================================================

bool UYcDataRegistrySubsystem::AcquireItem(const FDataRegistryId ItemId, const FDataRegistryItemAcquiredBPCallback AcquireCallback)
{
	return UDataRegistrySubsystem::AcquireItemBP(ItemId, AcquireCallback);
}

int32 UYcDataRegistrySubsystem::AcquireItems(const TArray<FDataRegistryId>& ItemIds, FDataRegistryItemAcquiredBPCallback AcquireCallback)
{
	int32 SuccessCount = 0;
	for (const FDataRegistryId& ItemId : ItemIds)
	{
		if (UDataRegistrySubsystem::AcquireItemBP(ItemId, AcquireCallback))
		{
			++SuccessCount;
		}
	}
	return SuccessCount;
}

//~=============================================================================
// 动态注册接口
//~=============================================================================

void UYcDataRegistrySubsystem::RegisterDataTableToRegistry(FDataRegistryType RegistryType, TSoftObjectPtr<UDataTable> DataTable)
{
	UDataRegistrySubsystem* Subsystem = GetEngineSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcDataRegistrySubsystem::RegisterDataTableToRegistry - DataRegistrySubsystem not available."));
		return;
	}
	
	if (DataTable.IsNull())
	{
		UE_LOG(LogYcGameplay, Warning, TEXT("UYcDataRegistrySubsystem::RegisterDataTableToRegistry - DataTable is null."));
		return;
	}
	
	FAssetData AssetData;
	if (UAssetManager::IsInitialized())
	{
		UAssetManager::Get().GetAssetDataForPath(DataTable.ToSoftObjectPath(), AssetData);
		Subsystem->RegisterSpecificAsset(RegistryType, AssetData);
	}
	else
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcDataRegistrySubsystem::RegisterDataTableToRegistry - AssetManager not initialized."));
	}
}

bool UYcDataRegistrySubsystem::LoadDataRegistry(const TSoftObjectPtr<UDataRegistry>& RegistryPath)
{
	UDataRegistrySubsystem* Subsystem = GetEngineSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcDataRegistrySubsystem::LoadDataRegistry - DataRegistrySubsystem not available."));
		return false;
	}
	
	if (RegistryPath.IsNull())
	{
		UE_LOG(LogYcGameplay, Warning, TEXT("UYcDataRegistrySubsystem::LoadDataRegistry - RegistryPath is null."));
		return false;
	}
	
	return Subsystem->LoadRegistryPath(RegistryPath.ToSoftObjectPath());
}

//~=============================================================================
// 工具方法
//~=============================================================================

const UDataRegistry* UYcDataRegistrySubsystem::GetRegistry(FDataRegistryType RegistryType)
{
	const UDataRegistrySubsystem* Subsystem = GetEngineSubsystem();
	if (!Subsystem)
	{
		return nullptr;
	}
	return Subsystem->GetRegistryForType(RegistryType);
}

UDataRegistrySubsystem* UYcDataRegistrySubsystem::GetEngineSubsystem()
{
	if (!GEngine)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcDataRegistrySubsystem::GetEngineSubsystem - GEngine is null."));
		return nullptr;
	}
	
	UDataRegistrySubsystem* Subsystem = GEngine->GetEngineSubsystem<UDataRegistrySubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcDataRegistrySubsystem::GetEngineSubsystem - DataRegistrySubsystem not available."));
	}
	return Subsystem;
}

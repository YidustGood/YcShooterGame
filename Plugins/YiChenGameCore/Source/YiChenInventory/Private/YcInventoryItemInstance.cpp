// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryItemInstance.h"

#include "DataRegistrySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"
#include "YcInventoryManagerComponent.h"
#include "YiChenInventory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryItemInstance)

UYcInventoryItemInstance::UYcInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYcInventoryItemInstance::IsSupportedForNetworking() const
{
	return true;
}

void UYcInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcInventoryItemInstance, ItemRegistryId);
	DOREPLIFETIME(UYcInventoryItemInstance, ItemInstId);
	DOREPLIFETIME(UYcInventoryItemInstance, TagsStack);
}

void UYcInventoryItemInstance::SetItemRegistryId(const FDataRegistryId& InItemRegistryId)
{
	ItemRegistryId = InItemRegistryId;
	// 立即尝试缓存物品定义
	CacheItemDefFromRegistry();
}

void UYcInventoryItemInstance::SetItemInstId(const FName NewId)
{
	ItemInstId = NewId;
}

bool UYcInventoryItemInstance::CacheItemDefFromRegistry()
{
	// 清除旧缓存
	ItemDef = nullptr;
	
	if (!ItemRegistryId.IsValid())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("UYcInventoryItemInstance::CacheItemDefFromRegistry - ItemRegistryId is invalid."));
		return false;
	}
	
	const UDataRegistrySubsystem* Subsystem = UDataRegistrySubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryItemInstance::CacheItemDefFromRegistry - DataRegistrySubsystem not available."));
		return false;
	}
	
	// 使用模板方法直接获取类型安全的指针
	ItemDef = Subsystem->GetCachedItem<FYcInventoryItemDefinition>(ItemRegistryId);
	
	if (!ItemDef)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("UYcInventoryItemInstance::CacheItemDefFromRegistry - Item not found in cache: %s. It may need to be acquired first."),
			*ItemRegistryId.ToString());
		return false;
	}
	
	return true;
}

const FYcInventoryItemDefinition* UYcInventoryItemInstance::GetItemDef()
{
	// 如果已缓存，直接返回
	if (ItemDef) return ItemDef;
	
	// 尝试从Registry获取
	CacheItemDefFromRegistry();
	return ItemDef;
}

bool UYcInventoryItemInstance::K2_GetItemDef(FYcInventoryItemDefinition& OutItemDef)
{
	const FYcInventoryItemDefinition* Def = GetItemDef();
	if (Def == nullptr) return false;
	OutItemDef = *Def;
	return true;
}

TInstancedStruct<FYcInventoryItemFragment> UYcInventoryItemInstance::FindItemFragment(const UScriptStruct* FragmentStructType)
{
	TInstancedStruct<FYcInventoryItemFragment> NullStruct;
	if (!GetItemDef()) return NullStruct;
	
	for (const auto& Frag : ItemDef->Fragments)
	{	
		if (FragmentStructType == Frag.GetScriptStruct())
		{
			return Frag;
		}
	}
	return NullStruct;
}

void UYcInventoryItemInstance::AddStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	TagsStack.AddStack(Tag, StackCount);
}

void UYcInventoryItemInstance::RemoveStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	TagsStack.RemoveStack(Tag, StackCount);
}

int32 UYcInventoryItemInstance::GetStatTagStackCount(const FGameplayTag Tag) const
{
	return TagsStack.GetStackCount(Tag);
}

bool UYcInventoryItemInstance::HasStatTag(const FGameplayTag Tag) const
{
	return TagsStack.ContainsTag(Tag);
}

UYcInventoryManagerComponent* UYcInventoryItemInstance::GetInventoryManager() const
{
	const AActor* OwnerActor = GetActorOuter();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryItemInstance::GetInventoryManager - Owner actor is nullptr."));
		return nullptr;
	}
	
	UYcInventoryManagerComponent* InventoryManager = OwnerActor->FindComponentByClass<UYcInventoryManagerComponent>();
	if (InventoryManager == nullptr)
	{
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryItemInstance::GetInventoryManager - InventoryManagerComponent not found on owner actor."));
		return nullptr;
	}
	return InventoryManager;
}

void UYcInventoryItemInstance::OnRep_ItemRegistryId()
{
	// 清除旧缓存并尝试重新获取
	ItemDef = nullptr;
	CacheItemDefFromRegistry();
}

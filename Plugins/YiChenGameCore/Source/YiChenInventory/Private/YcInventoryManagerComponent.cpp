// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryManagerComponent.h"

#include "DataRegistrySubsystem.h"
#include "NativeGameplayTags.h"
#include "YcInventoryItemInstance.h"
#include "YiChenInventory.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_Message_StackChanged, "Yc.Inventory.Message.StackChanged");

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryManagerComponent)

//////////////////////////////////////////////////////////////////////
// FYcInventoryItemEntry

FString FYcInventoryItemEntry::GetDebugString() const
{
	if (Instance == nullptr)
	{
		return FString::Printf(TEXT("(null) x %d"), StackCount);
	}
	
	const FYcInventoryItemDefinition* ItemDef = Instance->GetItemDef();
	const FName ItemId = ItemDef ? ItemDef->ItemId : NAME_None;
	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *ItemId.ToString());
}

//////////////////////////////////////////////////////////////////////
// FYcInventoryItemList

void FYcInventoryItemList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		FYcInventoryItemEntry& Stack = Items[Index];
		BroadcastChangeMessage(Stack, Stack.StackCount, 0);
		Stack.LastObservedCount = 0;
		RemoveItemToMap_Internal(Items[Index]);
	}
}

void FYcInventoryItemList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		FYcInventoryItemEntry& Stack = Items[Index];
		BroadcastChangeMessage(Stack, 0, Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
		AddItemToMap_Internal(Items[Index]);
		
		// 如果物品被添加了，但是Outer还不是库存组件的Owner，那么进行更新
		// 服务器在添加的时候就会修改Outer，但由于没有网络复制，所以借助这里为客户端同步修改Outer
		if (Stack.Instance && Stack.Instance->GetOuter() != OwnerComponent->GetOwner())
		{
			Stack.Instance->Rename(nullptr, OwnerComponent->GetOwner());
		}
	}
}

void FYcInventoryItemList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		FYcInventoryItemEntry& Stack = Items[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
		BroadcastChangeMessage(Stack, Stack.LastObservedCount, Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
		ChangeItemToMap_Internal(Items[Index]);
	}
}

TArray<UYcInventoryItemInstance*> FYcInventoryItemList::GetAllItemInstance() const
{
	TArray<UYcInventoryItemInstance*> Results;
	Results.Reserve(Items.Num());
	for (const FYcInventoryItemEntry& Entry : Items)
	{
		if (Entry.Instance != nullptr)
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

UYcInventoryItemInstance* FYcInventoryItemList::AddItem(const FDataRegistryId& ItemRegistryId, const int32 StackCount)
{
	check(OwnerComponent);
	
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());
	
	// 验证ItemRegistryId有效性
	if (!ItemRegistryId.IsValid())
	{
		UE_LOG(LogYcInventory, Error, TEXT("FYcInventoryItemList::AddItem - ItemRegistryId is invalid."));
		return nullptr;
	}
	
	// 从DataRegistry获取物品定义
	const UDataRegistrySubsystem* Subsystem = UDataRegistrySubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogYcInventory, Error, TEXT("FYcInventoryItemList::AddItem - DataRegistrySubsystem not available."));
		return nullptr;
	}
	
	// 使用模板方法直接获取类型安全的指针
	const FYcInventoryItemDefinition* ItemDef = Subsystem->GetCachedItem<FYcInventoryItemDefinition>(ItemRegistryId);
	if (!ItemDef)
	{
		UE_LOG(LogYcInventory, Error, TEXT("FYcInventoryItemList::AddItem - Failed to get ItemDef from DataRegistry: %s. Make sure the item is cached/acquired first."),
			*ItemRegistryId.ToString());
		return nullptr;
	}
	
	// 检查物品是否启用
	if (!ItemDef->bEnableItem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("FYcInventoryItemList::AddItem - Item is disabled: %s"), *ItemDef->ItemId.ToString());
		return nullptr;
	}
	
	// 创建ItemEntry
	FYcInventoryItemEntry& NewItemEntry = Items.AddDefaulted_GetRef();
	
	// 创建ItemInstance，如定义中的ItemInstanceType无效则使用默认类型
	TSubclassOf<UYcInventoryItemInstance> ItemInstanceClass = ItemDef->ItemInstanceType;
	if (!ItemInstanceClass)
	{
		ItemInstanceClass = UYcInventoryItemInstance::StaticClass();
	}
	
	// @TODO: Using the actor instead of component as the outer due to UE-127172
	NewItemEntry.Instance = NewObject<UYcInventoryItemInstance>(OwningActor, ItemInstanceClass);
	if (!NewItemEntry.Instance)
	{
		UE_LOG(LogYcInventory, Error, TEXT("FYcInventoryItemList::AddItem - Failed to create ItemInstance for: %s"), *ItemRegistryId.ToString());
		Items.RemoveAt(Items.Num() - 1);
		return nullptr;
	}
	
	// 设置ItemRegistryId（这会触发缓存物品定义）
	NewItemEntry.Instance->SetItemRegistryId(ItemRegistryId);
	NewItemEntry.Instance->SetItemInstId(GenerateNextItemId(NewItemEntry));
	
	// 从ItemDef中获取所有的Fragment，遍历调用Fragment->OnInstanceCreated通知
	for (const auto& Fragment : ItemDef->Fragments)
	{
		// 使用GetPtr<>()避免对象切片，保持多态
		if (const FYcInventoryItemFragment* FragPtr = Fragment.GetPtr<FYcInventoryItemFragment>())
		{
			FragPtr->OnInstanceCreated(NewItemEntry.Instance);
		}
	}
	
	NewItemEntry.StackCount = StackCount;
	AddItemToMap_Internal(NewItemEntry);
	MarkItemDirty(NewItemEntry);
	
	// 广播变化消息（权威端调用，客户端由FastArray辅助函数调用）
	BroadcastChangeMessage(NewItemEntry, 0, NewItemEntry.StackCount);
	
	return NewItemEntry.Instance;
}

bool FYcInventoryItemList::AddItem(UYcInventoryItemInstance* Instance, const int32 StackCount)
{
	check(Instance != nullptr);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FYcInventoryItemEntry& NewItemEntry = Items.AddDefaulted_GetRef();
	
	// 尝试重新指定Outer（例如从别人的库存系统中获取过来的ItemInst）
	if (Instance->GetOuter() != OwningActor)
	{
		if (!Instance->Rename(nullptr, OwningActor))
		{
			UE_LOG(LogYcInventory, Error, TEXT("FYcInventoryItemList::AddItem - Failed to rename ItemInstance outer."));
			Items.RemoveAt(Items.Num() - 1);
			return false;
		}
	}
	
	NewItemEntry.Instance = Instance;
	NewItemEntry.StackCount = StackCount;
	AddItemToMap_Internal(NewItemEntry);
	MarkItemDirty(NewItemEntry);
	BroadcastChangeMessage(NewItemEntry, 0, NewItemEntry.StackCount);
	
	return true;
}

bool FYcInventoryItemList::RemoveItem(UYcInventoryItemInstance* Instance)
{
	for (auto ItemEntryIt = Items.CreateIterator(); ItemEntryIt; ++ItemEntryIt)
	{
		FYcInventoryItemEntry& ItemEntry = *ItemEntryIt;
		if (ItemEntry.Instance == Instance)
		{
			RemoveItemToMap_Internal(ItemEntry);
			BroadcastChangeMessage(ItemEntry, ItemEntry.StackCount, 0);
			ItemEntryIt.RemoveCurrent();
			MarkArrayDirty();
			return true;
		}
	}
	return false;
}

void FYcInventoryItemList::BroadcastChangeMessage(const FYcInventoryItemEntry& ItemEntry, const int32 OldCount, const int32 NewCount) const
{
	FYcInventoryItemChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.ItemInstance = ItemEntry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - FMath::Max(OldCount, 0); // 保持OldCount不小于0

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_Yc_Inventory_Message_StackChanged, Message);
}

void FYcInventoryItemList::AddItemToMap_Internal(FYcInventoryItemEntry& Item)
{
	if (Item.Instance)
	{
		ItemsMap.Add(Item.Instance->GetItemInstId(), &Item);
	}
}

void FYcInventoryItemList::ChangeItemToMap_Internal(FYcInventoryItemEntry& Item)
{
	if (Item.Instance)
	{
		ItemsMap[Item.Instance->GetItemInstId()] = &Item;
	}
}

void FYcInventoryItemList::RemoveItemToMap_Internal(const FYcInventoryItemEntry& Item)
{
	if (Item.Instance)
	{
		ItemsMap.Remove(Item.Instance->GetItemInstId());
	}
}

FName FYcInventoryItemList::GenerateNextItemId(const FYcInventoryItemEntry& Item) const
{
	const FYcInventoryItemDefinition* ItemDef = Item.Instance ? Item.Instance->GetItemDef() : nullptr;
	if (!ItemDef) return NAME_None;
	
	FName NextItemId = ItemDef->ItemId;
	int32 Attempts = 0;
	const int32 MaxAttempts = 1000; // 防止无限循环
	
	while (Attempts < MaxAttempts)
	{
		if (!ItemsMap.Contains(NextItemId))
		{
			return NextItemId;
		}
		const int32 AppendNo = FMath::Rand();
		NextItemId = FName(*FString::Printf(TEXT("%s_%d"), *ItemDef->ItemId.ToString(), AppendNo));
		++Attempts;
	}
	
	UE_LOG(LogYcInventory, Error, TEXT("FYcInventoryItemList::GenerateNextItemId - Failed to generate unique ID after %d attempts for: %s"),
		MaxAttempts, *ItemDef->ItemId.ToString());
	return NAME_None;
}

//////////////////////////////////////////////////////////////////////
// UYcInventoryManagerComponent

UYcInventoryManagerComponent::UYcInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ItemList(this)
{
	// 组件必须被复制才能复制子对象
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true; //开启后使用AddReplicatedSubObject()将对象注册就会自动复制，否则需要重写ReplicateSubobjects手动复制
}

void UYcInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcInventoryManagerComponent, ItemList);	// 使用FastArray，不需要使用PushModel
}

void UYcInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	
	// 注册现有的ItemInstance到子对象复制列表中
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FYcInventoryItemEntry& ItemEntry : ItemList.Items)
		{
			if (IsValid(ItemEntry.Instance))
			{
				AddReplicatedSubObject(ItemEntry.Instance);
			}
		}
	}
}

bool UYcInventoryManagerComponent::CanAddItemDefinition_Implementation(const FYcInventoryItemDefinition& ItemDef, int32 StackCount)
{
	// 默认实现：只检查物品是否启用
	return ItemDef.bEnableItem;
}

UYcInventoryItemInstance* UYcInventoryManagerComponent::AddItem(const FDataRegistryId& ItemRegistryId, const int32 StackCount)
{
	UYcInventoryItemInstance* Result = ItemList.AddItem(ItemRegistryId, StackCount);
	
	if (Result && IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(Result);
	}
	
	return Result;
}

bool UYcInventoryManagerComponent::AddItemInstance(UYcInventoryItemInstance* ItemInstance, const int32 StackCount)
{
	const bool bResult = ItemList.AddItem(ItemInstance, StackCount);
	
	if (bResult && IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		// 添加到子对象复制列表中，以实现ItemInstance对象及其内部的网络复制
		AddReplicatedSubObject(ItemInstance);
	}
	
	return bResult;
}

bool UYcInventoryManagerComponent::RemoveItemInstance(UYcInventoryItemInstance* ItemInstance)
{
	if (!IsValid(ItemInstance))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("UYcInventoryManagerComponent::RemoveItemInstance - ItemInstance is invalid."));
		return false;
	}
	
	const bool bSuccess = ItemList.RemoveItem(ItemInstance);

	if (bSuccess && IsUsingRegisteredSubObjectList())
	{
		// 移除ItemInstance时，将其从子对象复制列表中移除
		RemoveReplicatedSubObject(ItemInstance);
	}
	
	return bSuccess;
}

TArray<UYcInventoryItemInstance*> UYcInventoryManagerComponent::GetAllItemInstance() const
{
	return ItemList.GetAllItemInstance();
}

UYcInventoryItemInstance* UYcInventoryManagerComponent::FindFirstItemInstByDefinition(const FYcInventoryItemDefinition& ItemDef) const
{
	const FYcInventoryItemEntry* const* ItemEntry = ItemList.ItemsMap.Find(ItemDef.ItemId);
	if (ItemEntry == nullptr) return nullptr;
	return (*ItemEntry)->Instance;
}

int32 UYcInventoryManagerComponent::GetTotalItemCountByDefinition(const FYcInventoryItemDefinition& ItemDef)
{
	int32 TotalCount = 0;
	for (const FYcInventoryItemEntry& ItemEntry : ItemList.Items)
	{
		if (IsValid(ItemEntry.Instance))
		{
			const FYcInventoryItemDefinition* EntryItemDef = ItemEntry.Instance->GetItemDef();
			if (EntryItemDef && EntryItemDef->ItemId == ItemDef.ItemId)
			{
				++TotalCount;
			}
		}
	}
	return TotalCount;
}

bool UYcInventoryManagerComponent::ConsumeItemsByDefinition(const FYcInventoryItemDefinition& ItemDef, const int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}
	
	// @TODO 目前的实现方式并没有去消耗物品的StackCount,而是直接移除了物品
	// 这里时间复杂度为O(n), 因为FindFirstItemStackByDefinition我们使用哈希表优化为了O(1)
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		UYcInventoryItemInstance* Instance = FindFirstItemInstByDefinition(ItemDef);
		if (Instance)
		{
			ItemList.RemoveItem(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcInventoryManagerComponent.h"

// 创建UGameplayMessageSubsystem广播信息Tag
#include "NativeGameplayTags.h"
#include "YcInventoryItemInstance.h"
#include "YiChenInventory.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_Message_StackChanged, "Yc.Inventory.Message.StackChanged");

//////////////////////////////////////////////////////////////////////
// FYcInventoryEntry
FString FYcInventoryItemEntry::GetDebugString() const
{
	FYcInventoryItemDefinition ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}
	
	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef.StaticStruct()));
}

//////////////////////////////////////////////////////////////////////
// FYcInventoryList
void FYcInventoryItemList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		FYcInventoryItemEntry& Stack = Items[Index];
		BroadcastChangeMessage(Stack,Stack.StackCount, 0);
		Stack.LastObservedCount = 0;
		RemoveItemToMap_Internal(Items[Index]);
	}
}

void FYcInventoryItemList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		FYcInventoryItemEntry& Stack = Items[Index];
		BroadcastChangeMessage(Stack,0,Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
		AddItemToMap_Internal(Items[Index]);
		// 如果物品被添加了,但是Outer还不是库存组件的Owner那么进行更新。服务器在添加的时候就会修改outer,但是由于没有网络复制,所以我们借助这里为客户端同步修改Outer
		if (Stack.Instance.GetOuter() != OwnerComponent->GetOwner())
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
		BroadcastChangeMessage(Stack,Stack.LastObservedCount,Stack.StackCount);
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
		if (Entry.Instance == nullptr) continue;
		Results.Add(Entry.Instance);
	}
	return Results;
}

UYcInventoryItemInstance* FYcInventoryItemList::AddItem(const FYcInventoryItemDefinition& ItemDef, const int32 StackCount)
{
	check(OwnerComponent);
	
	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FYcInventoryItemEntry& NewItemEntry = Items.AddDefaulted_GetRef(); // 构造一个默认的新元素并返回引用
	
	// 创建ItemInstance,如定义中的ItemInstanceType无效则使用默认的ItemInstance类型
	if (const TSubclassOf<UYcInventoryItemInstance> ItemInstanceClass = ItemDef.ItemInstanceType)
	{
		// @TODO: Using the actor instead of component as the outer due to UE-127172
		NewItemEntry.Instance = NewObject<UYcInventoryItemInstance>(OwningActor, ItemInstanceClass);
	}
	else
	{
		NewItemEntry.Instance = NewObject<UYcInventoryItemInstance>(OwningActor, UYcInventoryItemInstance::StaticClass());
	}
	
	if (NewItemEntry.Instance == nullptr)
	{
		UE_LOG(LogYcInventory, Error, TEXT("Failed to add items to inventory. Invalid Instance object."));
		return nullptr;
	}
	
	NewItemEntry.Instance->SetItemDef(ItemDef); // 设置所属ItemDef
	NewItemEntry.Instance->SetItemInstId(GenerateNextItemId(NewItemEntry)); // 自动生成填充ItemInstId
	// 从ItemDef中获取所有的Fragment,遍历调用Fragment->OnInstanceCreated(ItemInstance)函数通知ItemInstance创建 
	for (const auto& Fragment : ItemDef.Fragments)
	{
		auto ItemFragment = Fragment.Get<FYcInventoryItemFragment>();
		ItemFragment.OnInstanceCreated(NewItemEntry.Instance);
	}
	NewItemEntry.StackCount = StackCount;
	AddItemToMap_Internal(NewItemEntry);
	MarkItemDirty(NewItemEntry);
	// 这个函数会在权威端调用,拥有的客户端的这个函数由FastArray的三个辅助函数调用
	BroadcastChangeMessage(NewItemEntry, /*新添加的old为0*/ 0, NewItemEntry.StackCount);
	return NewItemEntry.Instance;
}

bool FYcInventoryItemList::AddItem(UYcInventoryItemInstance* Instance, const int32 StackCount)
{
	check(Instance != nullptr);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	FYcInventoryItemEntry& NewItemEntry = Items.AddDefaulted_GetRef();
	
	// 尝试重新指定Outer,例如从别人的库存系统中获取过来的ItemInst它的Outer是需要调整的
	if (Instance->GetOuter() != OwningActor->GetOuter())
	{
		if (!Instance->Rename(nullptr, OwningActor))
		{
			UE_LOG(LogYcInventory, Error, TEXT("Rename Item Instance failed!"));
			return false;
		}
	}
	// @TODO 是否需要通知Fragment其拥有者发生变化了
	
	NewItemEntry.Instance = Instance;
	NewItemEntry.StackCount = StackCount;
	AddItemToMap_Internal(NewItemEntry);
	MarkItemDirty(NewItemEntry);
	BroadcastChangeMessage(NewItemEntry, 0,  NewItemEntry.StackCount);
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
			// 这个函数会在权威端调用,拥有的客户端的这个函数由FastArray的三个辅助函数调用
			BroadcastChangeMessage(ItemEntry,ItemEntry.StackCount,0); // NewCount=0代表移除了/消耗完了
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
	Message.Delta = NewCount - FMath::Clamp(OldCount, 0, OldCount); // 保持OldCount不小于0,因为默认LastObservedCount = INDEX_NONE的值是-1

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_Yc_Inventory_Message_StackChanged, Message);
}

void FYcInventoryItemList::AddItemToMap_Internal(FYcInventoryItemEntry& Item)
{
	ItemsMap.Add(Item.Instance->GetItemInstId(), &Item);
}

void FYcInventoryItemList::ChangeItemToMap_Internal(FYcInventoryItemEntry& Item)
{
	ItemsMap[Item.Instance->GetItemInstId()] = &Item;
}

void FYcInventoryItemList::RemoveItemToMap_Internal(const FYcInventoryItemEntry& Item)
{
	// 由于TMap不可复制，所以移除Tag的时候，TMap中映射的Tag需要我们收到预删除通知时手动进行清理，以保证与服务器达成同步，后面的添加修改同理，在收到相应同步后，从Stacks列表中取得数据并同步设置到TMap中
	ItemsMap.Remove(Item.Instance->GetItemInstId());
}

FName FYcInventoryItemList::GenerateNextItemId(const FYcInventoryItemEntry& Item) const
{
	const FYcInventoryItemDefinition& ItemDef = Item.Instance->GetItemDef();
	FName NextItemId = ItemDef.ItemId;
	while (true)
	{
		if (!ItemsMap.Contains(NextItemId)) // 知道找到不重复的ItemId才返回
		{
			return NextItemId;
		}
		const int32 AppendNo = FMath::Rand32();
		FString NewItemNameStr = FString::Printf(TEXT("%s_%d"), *ItemDef.ItemId.ToString(), AppendNo);
		NextItemId = FName(*NewItemNameStr);
	}
}

//////////////////////////////////////////////////////////////////////
// UYcInventoryManagerComponent
UYcInventoryManagerComponent::UYcInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), ItemList(this)
{
	// 组件必须被复制才能复制子对象
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true; //开启后使用AddReplicatedSubObject()将对象注册就会自动复制，否则需要重写ReplicateSubobjects手动复制
}

void UYcInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UYcInventoryManagerComponent, ItemList); // 使用FastArray，不需要使用PushModel
}

void UYcInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	// 注册现有的ItemInstance到子对象复制列表中
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FYcInventoryItemEntry& ItemEntry : ItemList.Items)
		{
			UYcInventoryItemInstance* Instance = ItemEntry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

bool UYcInventoryManagerComponent::CanAddItemDefinition_Implementation(const FYcInventoryItemDefinition& ItemDef, int32 StackCount)
{
	//@TODO: 可增加条件限制功能
	return true;
}

UYcInventoryItemInstance* UYcInventoryManagerComponent::AddItemByDefinition(const FYcInventoryItemDefinition& ItemDef, int32 StackCount)
{
	UYcInventoryItemInstance* Result = ItemList.AddItem(ItemDef, StackCount);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
	{
		AddReplicatedSubObject(Result);
	}
	return Result;
}

bool UYcInventoryManagerComponent::AddItemInstance(UYcInventoryItemInstance* ItemInstance, const int32 StackCount)
{
	const bool bResult = ItemList.AddItem(ItemInstance, StackCount);
	if (!bResult) return false;
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance); // 添加到子对象复制列表中，以实现ItemInstance对象及其内部的网络复制
	}
	return bResult;
}

bool UYcInventoryManagerComponent::RemoveItemInstance(UYcInventoryItemInstance* ItemInstance)
{
	if (!IsValid(ItemInstance))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("The item instance to be removed is invalid. "));
		return false;
	}
	const bool bSuccess = ItemList.RemoveItem(ItemInstance);

	if (bSuccess && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance); // 移除ItemInstance时，将其从子对象复制列表中移除
	}
	return bSuccess;
}

TArray<UYcInventoryItemInstance*> UYcInventoryManagerComponent::GetAllItemInstance() const
{
	return ItemList.GetAllItemInstance();
}

UYcInventoryItemInstance* UYcInventoryManagerComponent::FindFirstItemInstByDefinition(const FYcInventoryItemDefinition& ItemDef) const
{
	const FYcInventoryItemEntry* ItemEntry = *ItemList.ItemsMap.Find(ItemDef.ItemId);
	if (ItemEntry)
	{
		return ItemEntry->Instance;
	}

	return nullptr;
}

int32 UYcInventoryManagerComponent::GetTotalItemCountByDefinition(const FYcInventoryItemDefinition& ItemDef) const
{
	int32 TotalCount = 0;
	for (const FYcInventoryItemEntry& ItemEntry : ItemList.Items)
	{
		const UYcInventoryItemInstance* Instance = ItemEntry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef().ItemId.IsEqual(ItemDef.ItemId))
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
	if (!OwningActor || !OwningActor->HasAuthority()) return false; //权威权限检查
	
	// @TODO 目前的实现方式并没有去消耗物品的StackCount,而是直接移除了物品
	// 这里时间复杂度为O(n),因为FindFirstItemStackByDefinition我们使用哈希表优化为了O(1)
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (UYcInventoryItemInstance* Instance = FindFirstItemInstByDefinition(ItemDef))
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
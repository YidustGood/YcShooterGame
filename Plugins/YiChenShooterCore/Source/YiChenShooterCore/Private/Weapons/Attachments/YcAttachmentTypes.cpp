// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Attachments/YcAttachmentTypes.h"
#include "Weapons/Attachments/YcWeaponAttachmentComponent.h"
#include "DataRegistrySubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAttachmentTypes)

DEFINE_LOG_CATEGORY_STATIC(LogYcAttachment, Log, All);

// ═══════════════════════════════════════════════════════════════
// FYcAttachmentInstance 方法实现
// ═══════════════════════════════════════════════════════════════

void FYcAttachmentInstance::Reset()
{
	AttachmentRegistryId = FDataRegistryId();
	SlotType = FGameplayTag();
	bIsInstalled = false;
	TuningValues.Empty();
	MeshComponent.Reset();
	CachedDef = nullptr;
}

void FYcAttachmentInstance::MarkUninstalled()
{
	// 保留 AttachmentRegistryId 和 SlotType，便于客户端处理卸载逻辑
	bIsInstalled = false;
	TuningValues.Empty();
	MeshComponent.Reset();
	// 注意：不清空 CachedDef，客户端可能需要访问
}

bool FYcAttachmentInstance::CacheDefinitionFromRegistry() const
{
	if (!AttachmentRegistryId.IsValid())
	{
		UE_LOG(LogYcAttachment, Warning, TEXT("CacheDefinitionFromRegistry: Invalid AttachmentRegistryId"));
		CachedDef = nullptr;
		return false;
	}

	const UDataRegistrySubsystem* Subsystem = UDataRegistrySubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogYcAttachment, Error, TEXT("CacheDefinitionFromRegistry: DataRegistrySubsystem not available"));
		CachedDef = nullptr;
		return false;
	}

	CachedDef = Subsystem->GetCachedItem<FYcAttachmentDefinition>(AttachmentRegistryId);
	
	if (!CachedDef)
	{
		UE_LOG(LogYcAttachment, Warning, 
			TEXT("CacheDefinitionFromRegistry: Attachment not found in cache: %s"),
			*AttachmentRegistryId.ToString());
		return false;
	}
	
	return true;
}

const FYcAttachmentDefinition* FYcAttachmentInstance::GetDefinition() const
{
	// 如果已缓存，直接返回
	if (CachedDef)
	{
		return CachedDef;
	}
	
	// 尝试从 DataRegistry 缓存
	CacheDefinitionFromRegistry();
	return CachedDef;
}

FString FYcAttachmentInstance::GetDebugString() const
{
	return FString::Printf(TEXT("Attachment[%s] in Slot[%s]"),
		*AttachmentRegistryId.ToString(),
		*SlotType.ToString());
}

// ═══════════════════════════════════════════════════════════════
// FYcAttachmentArray Fast Array 回调实现
// ═══════════════════════════════════════════════════════════════

void FYcAttachmentArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		FYcAttachmentInstance& Instance = Items[Index];
		// 这个函数是在配件从数组中移除前调用的, 我们要标注这个配件为未安装, 以避免在本帧的武器数组计算中错误的收集到这已经卸下的配件的数值修改器
		Instance.bIsInstalled = false;
		
		UE_LOG(LogYcAttachment, Log, TEXT("PreReplicatedRemove: %s"), *Instance.GetDebugString());
		
		if (OwnerComponent)
		{
			OwnerComponent->HandleAttachmentRemoved(Instance);
		}
	}
}

void FYcAttachmentArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		FYcAttachmentInstance& Instance = Items[Index];
		
		UE_LOG(LogYcAttachment, Log, TEXT("PostReplicatedAdd: Slot[%s] Valid=%s"), 
			*Instance.SlotType.ToString(),
			Instance.IsValid() ? TEXT("true") : TEXT("false"));
		
		// 跳过无效的实例（空槽位）- 但仍然需要处理动态槽位
		if (!Instance.IsValid())
		{
			// 空槽位也需要通知，以便客户端同步槽位结构
			// 但不需要处理配件安装逻辑
			continue;
		}
		
		// 从 DataRegistry 缓存定义
		Instance.CacheDefinitionFromRegistry();
		
		UE_LOG(LogYcAttachment, Log, TEXT("PostReplicatedAdd: %s"), *Instance.GetDebugString());
		
		if (OwnerComponent)
		{
			OwnerComponent->HandleAttachmentAdded(Instance);
		}
	}
}

void FYcAttachmentArray::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		FYcAttachmentInstance& Instance = Items[Index];
		
		// 根据 bIsInstalled 判断是安装还是卸载
		if (!Instance.bIsInstalled)
		{
			// 配件被卸载
			UE_LOG(LogYcAttachment, Log, TEXT("PostReplicatedChange: Attachment uninstalled from Slot[%s], was: %s"), 
				*Instance.SlotType.ToString(),
				*Instance.AttachmentRegistryId.ToString());
			
			if (OwnerComponent)
			{
				// 使用保留的 AttachmentRegistryId 广播卸载事件
				OwnerComponent->HandleAttachmentRemoved(Instance);
			}
			
			// // 卸载处理完成后，清空数据, 注释原因：保留, 装备下一个配件直接覆盖就好了
			// Instance.AttachmentRegistryId = FDataRegistryId();
			// Instance.CachedDef = nullptr;
		}
		else
		{
			// 配件被安装或更换
			Instance.CachedDef = nullptr; // 重置缓存
			Instance.CacheDefinitionFromRegistry();
			
			UE_LOG(LogYcAttachment, Log, TEXT("PostReplicatedChange: Attachment installed/changed: %s"), 
				*Instance.GetDebugString());
			
			if (OwnerComponent)
			{
				OwnerComponent->HandleAttachmentChanged(Instance);
			}
		}
	}
}

// ═══════════════════════════════════════════════════════════════
// FYcAttachmentDefinition 辅助方法实现
// ═══════════════════════════════════════════════════════════════

bool FYcAttachmentDefinition::HasProvidedSlot(FGameplayTag InSlotType) const
{
	for (const FYcAttachmentSlotDef& Slot : ProvidedSlots)
	{
		if (Slot.SlotType.MatchesTagExact(InSlotType))
		{
			return true;
		}
	}
	return false;
}

TArray<FGameplayTag> FYcAttachmentDefinition::GetProvidedSlotTypes() const
{
	TArray<FGameplayTag> Result;
	for (const FYcAttachmentSlotDef& Slot : ProvidedSlots)
	{
		Result.Add(Slot.SlotType);
	}
	return Result;
}

TArray<FYcStatModifier> FYcAttachmentDefinition::GetModifiersForStat(FGameplayTag StatTag) const
{
	TArray<FYcStatModifier> Result;
	
	for (const FYcStatModifier& Modifier : StatModifiers)
	{
		if (Modifier.StatTag.MatchesTag(StatTag))
		{
			Result.Add(Modifier);
		}
	}
	
	return Result;
}

bool FYcAttachmentDefinition::IsIncompatibleWith(FGameplayTag OtherAttachmentId) const
{
	return IncompatibleAttachments.HasTag(OtherAttachmentId);
}

bool FYcAttachmentDefinition::HasRequiredAttachments(const TArray<FGameplayTag>& InstalledAttachmentIds) const
{
	// 如果没有前置要求，直接返回true
	if (RequiredAttachments.IsEmpty())
	{
		return true;
	}
	
	// 检查所有前置配件是否已安装
	for (const FGameplayTag& RequiredTag : RequiredAttachments)
	{
		bool bFound = false;
		for (const FGameplayTag& InstalledTag : InstalledAttachmentIds)
		{
			if (InstalledTag.MatchesTag(RequiredTag))
			{
				bFound = true;
				break;
			}
		}
		
		if (!bFound)
		{
			return false;
		}
	}
	
	return true;
}

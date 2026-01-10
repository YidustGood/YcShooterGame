// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Attachments/YcAttachmentDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAttachmentDefinition)

UYcAttachmentDefinition::UYcAttachmentDefinition()
{
}

FPrimaryAssetId UYcAttachmentDefinition::GetPrimaryAssetId() const
{
	// 使用类名作为AssetType，资产名作为AssetName
	return FPrimaryAssetId(GetClass()->GetFName(), GetFName());
}

TArray<FYcStatModifier> UYcAttachmentDefinition::GetModifiersForStat(FGameplayTag StatTag) const
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

bool UYcAttachmentDefinition::IsIncompatibleWith(FGameplayTag OtherAttachmentId) const
{
	return IncompatibleAttachments.HasTag(OtherAttachmentId);
}

bool UYcAttachmentDefinition::HasRequiredAttachments(const TArray<FGameplayTag>& InstalledAttachmentIds) const
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

bool UYcAttachmentDefinition::HasProvidedSlot(FGameplayTag InSlotType) const
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

TArray<FGameplayTag> UYcAttachmentDefinition::GetProvidedSlotTypes() const
{
	TArray<FGameplayTag> Result;
	for (const FYcAttachmentSlotDef& Slot : ProvidedSlots)
	{
		Result.Add(Slot.SlotType);
	}
	return Result;
}

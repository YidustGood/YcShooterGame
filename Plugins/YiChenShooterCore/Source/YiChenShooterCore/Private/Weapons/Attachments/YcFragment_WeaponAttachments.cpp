// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Attachments/YcFragment_WeaponAttachments.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcFragment_WeaponAttachments)

FYcFragment_WeaponAttachments::FYcFragment_WeaponAttachments()
	: MaxAttachments(5)
{
}

const FYcAttachmentSlotDef* FYcFragment_WeaponAttachments::GetSlotDef(FGameplayTag SlotType) const
{
	for (const FYcAttachmentSlotDef& SlotDef : AttachmentSlots)
	{
		if (SlotDef.SlotType.MatchesTagExact(SlotType))
		{
			return &SlotDef;
		}
	}
	return nullptr;
}

bool FYcFragment_WeaponAttachments::HasSlot(FGameplayTag SlotType) const
{
	return GetSlotDef(SlotType) != nullptr;
}

TArray<FGameplayTag> FYcFragment_WeaponAttachments::GetAllSlotTypes() const
{
	TArray<FGameplayTag> Result;
	Result.Reserve(AttachmentSlots.Num());
	
	for (const FYcAttachmentSlotDef& SlotDef : AttachmentSlots)
	{
		Result.Add(SlotDef.SlotType);
	}
	
	return Result;
}

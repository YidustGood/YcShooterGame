// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGameplayAbility_FromEquipment.h"

#include "YcEquipmentInstance.h"
#include "YcInventoryItemInstance.h"
#include "Misc/DataValidation.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility_FromEquipment)

UYcGameplayAbility_FromEquipment::UYcGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UYcEquipmentInstance* UYcGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UYcEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

UYcInventoryItemInstance* UYcGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	if (const UYcEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UYcInventoryItemInstance>(Equipment->GetAssociatedItem());
	}
	return nullptr;
}

TInstancedStruct<FYcInventoryItemFragment> UYcGameplayAbility_FromEquipment::FindItemFragment(
	const UScriptStruct* FragmentStructType) const
{
	TInstancedStruct<FYcInventoryItemFragment> NullStruct;
	
	 UYcInventoryItemInstance* ItemInst = GetAssociatedItem();
	if (!ItemInst) return NullStruct;
	
	return ItemInst->FindItemFragment(FragmentStructType);
}

TInstancedStruct<FYcEquipmentFragment> UYcGameplayAbility_FromEquipment::FindEquipmentFragment(
	const UScriptStruct* FragmentStructType) const
{
	TInstancedStruct<FYcEquipmentFragment> NullStruct;
	
	UYcEquipmentInstance* EquipmentInst = GetAssociatedEquipment();
	if (!EquipmentInst) return NullStruct;
	return EquipmentInst->FindEquipmentFragment(FragmentStructType);
}

#if WITH_EDITOR
EDataValidationResult UYcGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
	{
		Context.AddError(NSLOCTEXT("YcGameCore", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Equipment/YcEquipmentInputFragment.h"

#include "Character/YcHeroComponent.h"
#include "Input/YcInputConfig.h"
#include "YcEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentInputFragment)

void FEquipmentFragment_InputBindings::OnEquipped(UYcEquipmentInstance* Instance) const
{
	if (!Instance)
	{
		return;
	}

	APawn* Pawn = Instance->GetPawn();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	UYcHeroComponent* HeroComponent = UYcHeroComponent::FindYcHeroComponent(Pawn);
	if (!HeroComponent)
	{
		return;
	}

	for (const FYcInputMappingContextAndPriority& MappingEntry : InputMappings)
	{
		if (const UInputMappingContext* InputMapping = MappingEntry.InputMapping.LoadSynchronous())
		{
			HeroComponent->AddAdditionalInputMapping(InputMapping, MappingEntry.Priority);
		}
	}

	for (const TSoftObjectPtr<const UYcInputConfig>& ConfigEntry : InputConfigs)
	{
		if (const UYcInputConfig* InputConfig = ConfigEntry.LoadSynchronous())
		{
			HeroComponent->AddAdditionalInputConfig(InputConfig);
		}
	}
}

void FEquipmentFragment_InputBindings::OnUnequipped(UYcEquipmentInstance* Instance) const
{
	if (!Instance)
	{
		return;
	}

	APawn* Pawn = Instance->GetPawn();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	UYcHeroComponent* HeroComponent = UYcHeroComponent::FindYcHeroComponent(Pawn);
	if (!HeroComponent)
	{
		return;
	}

	for (const TSoftObjectPtr<const UYcInputConfig>& ConfigEntry : InputConfigs)
	{
		if (const UYcInputConfig* InputConfig = ConfigEntry.Get())
		{
			HeroComponent->RemoveAdditionalInputConfig(InputConfig);
		}
	}

	for (const FYcInputMappingContextAndPriority& MappingEntry : InputMappings)
	{
		if (const UInputMappingContext* InputMapping = MappingEntry.InputMapping.Get())
		{
			HeroComponent->RemoveAdditionalInputMapping(InputMapping);
		}
	}
}

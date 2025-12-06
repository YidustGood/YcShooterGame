// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGameplayEffectContext.h"
#include "YcAbilitySourceInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayEffectContext)

class FArchive;

FYcGameplayEffectContext* FYcGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FYcGameplayEffectContext::StaticStruct()))
	{
		return (FYcGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FYcGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

void FYcGameplayEffectContext::SetAbilitySource(const IYcAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const IYcAbilitySourceInterface* FYcGameplayEffectContext::GetAbilitySource() const
{
	return Cast<IYcAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FYcGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}
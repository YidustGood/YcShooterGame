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

	// 序列化伤害类型标签
	DamageTypeTag.NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	Ar << CueImpactPositions;
	Ar << CueImpactNormals;
	Ar << CueImpactSurfaceTypes;

	// @TODO 是否需要处理RuntimePayloads的 NetSerialize

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

void FYcGameplayEffectContext::AddOrReplaceRuntimePayload(const FInstancedStruct& InPayload)
{
	if (!InPayload.IsValid())
	{
		return;
	}

	const UScriptStruct* PayloadType = InPayload.GetScriptStruct();
	if (!PayloadType)
	{
		return;
	}

	for (FInstancedStruct& ExistingPayload : RuntimePayloads)
	{
		if (ExistingPayload.GetScriptStruct() == PayloadType)
		{
			ExistingPayload = InPayload;
			return;
		}
	}

	RuntimePayloads.Add(InPayload);
}

const FInstancedStruct* FYcGameplayEffectContext::FindRuntimePayload(const UScriptStruct* PayloadType) const
{
	if (!PayloadType)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Payload : RuntimePayloads)
	{
		if (Payload.GetScriptStruct() == PayloadType)
		{
			return &Payload;
		}
	}

	return nullptr;
}

void FYcGameplayEffectContext::SetCueImpactData(const TArray<FVector>& InImpactPositions, const TArray<FVector>& InImpactNormals, const TArray<uint8>& InImpactSurfaceTypes)
{
	CueImpactPositions = InImpactPositions;
	CueImpactNormals = InImpactNormals;
	CueImpactSurfaceTypes = InImpactSurfaceTypes;
}

bool FYcGameplayEffectContext::GetCueImpactData(TArray<FVector>& OutImpactPositions, TArray<FVector>& OutImpactNormals, TArray<uint8>& OutImpactSurfaceTypes) const
{
	if (!HasCueImpactData())
	{
		return false;
	}

	OutImpactPositions = CueImpactPositions;
	OutImpactNormals = CueImpactNormals;
	OutImpactSurfaceTypes = CueImpactSurfaceTypes;
	return true;
}

bool FYcGameplayEffectContext::HasCueImpactData() const
{
	const int32 NumImpacts = CueImpactPositions.Num();
	return NumImpacts > 0 &&
		CueImpactNormals.Num() == NumImpacts &&
		CueImpactSurfaceTypes.Num() == NumImpacts;
}

const UPhysicalMaterial* FYcGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}

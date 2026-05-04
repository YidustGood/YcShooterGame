// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Feedback/ContextEffects/AssetTypeActions_YcContextEffectsLibrary.h"

#include "Feedback/ContextEffects/YcContextEffectsLibrary.h"

class UClass;

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_YcContextEffectsLibrary::GetSupportedClass() const
{
	return UYcContextEffectsLibrary::StaticClass();
}

#undef LOCTEXT_NAMESPACE

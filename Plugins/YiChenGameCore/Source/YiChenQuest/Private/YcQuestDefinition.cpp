// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcQuestDefinition.h"

#include "AssetRegistry/AssetBundleData.h"
#include "YcQuestObjective.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestDefinition)

const FPrimaryAssetType UYcQuestDefinition::QuestDefinitionType(TEXT("YcQuestDefinition"));

FPrimaryAssetId UYcQuestDefinition::GetPrimaryAssetId() const
{
    const FName FinalQuestId = QuestId.IsNone() ? GetFName() : QuestId;
    return FPrimaryAssetId(QuestDefinitionType, FinalQuestId);
}

#if WITH_EDITOR
EDataValidationResult UYcQuestDefinition::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

    if (QuestId.IsNone())
    {
        Context.AddError(FText::FromString(TEXT("QuestId must not be None.")));
        Result = EDataValidationResult::Invalid;
    }

    if (!RootObjective)
    {
        Context.AddError(FText::FromString(TEXT("RootObjective is required.")));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif

#if WITH_EDITORONLY_DATA
void UYcQuestDefinition::UpdateAssetBundleData()
{
    Super::UpdateAssetBundleData();

    for (const FYcQuestAssetRef& AssetRef : AssetRefs)
    {
        if (AssetRef.BundleName.IsNone())
        {
            continue;
        }

        const FSoftObjectPath AssetPath = AssetRef.Asset.ToSoftObjectPath();
        if (AssetPath.IsValid())
        {
            AssetBundleData.AddBundleAsset(AssetRef.BundleName, AssetPath);
        }

        const FSoftObjectPath ClassPath = AssetRef.AssetClass.ToSoftObjectPath();
        if (ClassPath.IsValid())
        {
            AssetBundleData.AddBundleAsset(AssetRef.BundleName, ClassPath);
        }
    }
}
#endif

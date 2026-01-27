// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/Actions/YcGameFeatureAction_AddGameplayCuePath.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_AddGameplayCuePath)


#define LOCTEXT_NAMESPACE "GameFeatures"

UYcGameFeatureAction_AddGameplayCuePath::UYcGameFeatureAction_AddGameplayCuePath()
{
	// 添加一个常用的默认路径
	DirectoryPathsToAdd.Add(FDirectoryPath{ TEXT("/GameplayCues") });
}

#if WITH_EDITOR
EDataValidationResult UYcGameFeatureAction_AddGameplayCuePath::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// 验证每个配置的目录路径是否有效
	for (const FDirectoryPath& Directory : DirectoryPathsToAdd)
	{
		if (Directory.Path.IsEmpty())
		{
			const FText InvalidCuePathError = FText::Format(
				LOCTEXT("InvalidCuePathError", "'{0}' is not a valid path!"), 
				FText::FromString(Directory.Path));
			Context.AddError(InvalidCuePathError);
			Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
		}
	}

	return CombineDataValidationResults(Result, EDataValidationResult::Valid);
}
#endif	// WITH_EDITOR

#undef LOCTEXT_NAMESPACE
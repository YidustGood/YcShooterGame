// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGameplayTagsLibrary.h"
#include "GameplayTagsManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayTagsLibrary)

FGameplayTag UYcGameplayTagsLibrary::MakeGameplayTagFromString(const FString& TagString, bool bErrorIfNotFound)
{
	if (TagString.IsEmpty())
	{
		return FGameplayTag::EmptyTag;
	}
    
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), bErrorIfNotFound);
    
	if (bErrorIfNotFound && !Tag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("GameplayTag not found: %s"), *TagString);
	}
    
	return Tag;
}

FGameplayTag UYcGameplayTagsLibrary::MakeGameplayTagFromStringArray(const TArray<FString>& TagParts, bool bErrorIfNotFound)
{
	if (TagParts.Num() == 0)
	{
		return FGameplayTag::EmptyTag;
	}

	// 过滤有效部件
	TArray<FString> FilteredParts = FilterValidTagParts(TagParts, true, true);
    
	if (FilteredParts.Num() == 0)
	{
		return FGameplayTag::EmptyTag;
	}

	// 用点号连接所有部件
	FString FullTagString = FString::Join(FilteredParts, TEXT("."));
    
	// 转换为GameplayTag
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*FullTagString), bErrorIfNotFound);
    
	if (bErrorIfNotFound && !Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create GameplayTag from array: %s"), *FullTagString);
	}
    
	return Tag;
}

FGameplayTag UYcGameplayTagsLibrary::MakeGameplayTagFromName(FName TagName, bool bErrorIfNotFound)
{
	if (TagName.IsNone())
	{
		return FGameplayTag::EmptyTag;
	}
    
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, bErrorIfNotFound);
    
	if (bErrorIfNotFound && !Tag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("GameplayTag not found: %s"), *TagName.ToString());
	}
    
	return Tag;
}

FGameplayTag UYcGameplayTagsLibrary::MakeGameplayTagFromNameArray(const TArray<FName>& TagParts, bool bErrorIfNotFound)
{
	if (TagParts.Num() == 0)
	{
		return FGameplayTag::EmptyTag;
	}

	TArray<FString> StringParts;
	for (const FName& NamePart : TagParts)
	{
		if (!NamePart.IsNone())
		{
			StringParts.Add(NamePart.ToString());
		}
	}
    
	return MakeGameplayTagFromStringArray(StringParts, bErrorIfNotFound);
}

FGameplayTag UYcGameplayTagsLibrary::MakeGameplayTagFromText(const FText& TagText, bool bErrorIfNotFound)
{
	if (TagText.IsEmpty())
	{
		return FGameplayTag::EmptyTag;
	}
    
	FString TagString = TagText.ToString();
	return MakeGameplayTagFromString(TagString, bErrorIfNotFound);
}

FGameplayTag UYcGameplayTagsLibrary::MakeGameplayTagFromTextArray(const TArray<FText>& TagParts, bool bErrorIfNotFound)
{
	if (TagParts.Num() == 0)
	{
		return FGameplayTag::EmptyTag;
	}

	TArray<FString> StringParts;
	for (const FText& TextPart : TagParts)
	{
		if (!TextPart.IsEmpty())
		{
			StringParts.Add(TextPart.ToString());
		}
	}
    
	return MakeGameplayTagFromStringArray(StringParts, bErrorIfNotFound);
}

FGameplayTagContainer UYcGameplayTagsLibrary::MakeGameplayTagsFromStringArrays(const TArray<FTagPartsArray>& TagArrays, 
																						 bool bErrorIfNotFound)
{
	FGameplayTagContainer ResultContainer;
    
	for (const FTagPartsArray& TagArray : TagArrays)
	{
		FGameplayTag Tag = MakeGameplayTagFromStringArray(TagArray.TagParts, bErrorIfNotFound);
		if (Tag.IsValid())
		{
			ResultContainer.AddTag(Tag);
		}
	}
    
	return ResultContainer;
}

TArray<FString> UYcGameplayTagsLibrary::FilterValidTagParts(const TArray<FString>& TagParts, bool bRemoveEmpty,
	bool bTrimWhitespace)
{
	TArray<FString> FilteredParts;
    
	for (const FString& Part : TagParts)
	{
		FString ProcessedPart = Part;
        
		// 修剪空白字符
		if (bTrimWhitespace)
		{
			ProcessedPart.TrimStartAndEndInline();
		}
        
		// 检查是否为空
		if (!bRemoveEmpty || !ProcessedPart.IsEmpty())
		{
			FilteredParts.Add(ProcessedPart);
		}
	}
    
	return FilteredParts;
}

bool UYcGameplayTagsLibrary::DoesGameplayTagExist(const FString& TagString)
{
	if (TagString.IsEmpty())
	{
		return false;
	}
    
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
	return Tag.IsValid();
}
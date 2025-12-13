// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcGameplayTagsLibrary.generated.h"

USTRUCT(BlueprintType)
struct FTagPartsArray
{
	GENERATED_BODY()
	TArray<FString> TagParts;
};

/**
 * GameplayTag工具函数库, 让蓝图也可以通过FString、FName、FText构建FGameplayTag
 * 例如从服务器获取的库存信息JSON中解析出物品, 然后通过物品的JSON层级动态拼接构建GameplayTag然后物品是未读物品就可以把它注册到RedDotSystem
 */
UCLASS()
class YICHENREDDOTSYSTEM_API UYcGameplayTagsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 从字符串创建GameplayTag
	UFUNCTION(BlueprintCallable, Category = "GameplayTags", 
			  meta = (Keywords = "tag string create", 
					  ToolTip = "Create a GameplayTag from a string"))
	static FGameplayTag MakeGameplayTagFromString(const FString& TagString, bool bErrorIfNotFound = false);
	
	// 从字符串数组创建GameplayTag（用点号连接）
	UFUNCTION(BlueprintCallable, Category = "GameplayTags|Array",
			  meta = (Keywords = "tag array string create",
					  ToolTip = "Create a GameplayTag by joining an array of strings with dots"))
	static FGameplayTag MakeGameplayTagFromStringArray(const TArray<FString>& TagParts, 
													   bool bErrorIfNotFound = false);

	// 从FName创建GameplayTag
	UFUNCTION(BlueprintCallable, Category = "GameplayTags",
			  meta = (Keywords = "tag name create"))
	static FGameplayTag MakeGameplayTagFromName(FName TagName, bool bErrorIfNotFound = false);
	
	// 从FName数组创建GameplayTag
	UFUNCTION(BlueprintCallable, Category = "GameplayTags|Array",
			  meta = (Keywords = "tag name array create"))
	static FGameplayTag MakeGameplayTagFromNameArray(const TArray<FName>& TagParts, 
													 bool bErrorIfNotFound = false);

	// 从FText创建GameplayTag
	UFUNCTION(BlueprintCallable, Category = "GameplayTags",
			  meta = (Keywords = "tag text create"))
	static FGameplayTag MakeGameplayTagFromText(const FText& TagText, bool bErrorIfNotFound = false);
	
	// 从FText数组创建GameplayTag
	UFUNCTION(BlueprintCallable, Category = "GameplayTags|Array",
			  meta = (Keywords = "tag text array create"))
	static FGameplayTag MakeGameplayTagFromTextArray(const TArray<FText>& TagParts, 
													 bool bErrorIfNotFound = false);
	
	// 创建多个GameplayTag（批量处理）
	UFUNCTION(BlueprintCallable, Category = "GameplayTags|Array",
			  meta = (Keywords = "tags array batch create"))
	static FGameplayTagContainer MakeGameplayTagsFromStringArrays(const TArray<FTagPartsArray>& TagArrays, 
																  bool bErrorIfNotFound = false);
	
	// 验证并筛选有效的Tag部件
	UFUNCTION(BlueprintCallable, Category = "GameplayTags|Array",
			  meta = (Keywords = "filter tag parts valid"))
	static TArray<FString> FilterValidTagParts(const TArray<FString>& TagParts, 
											   bool bRemoveEmpty = true, 
											   bool bTrimWhitespace = true);
	
	// 检查Tag是否存在
	UFUNCTION(BlueprintCallable, Category = "GameplayTags",
			  meta = (Keywords = "tag exists check"))
	static bool DoesGameplayTagExist(const FString& TagString);
};

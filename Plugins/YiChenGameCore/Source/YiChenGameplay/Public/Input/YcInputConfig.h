// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "YcInputConfig.generated.h"

class UInputAction;
class UObject;

/**
 * 输入动作映射结构体
 * 
 * 将一个UInputAction与一个GameplayTag绑定, 将输入和标签绑定, 实现基于标签驱动的输入系统。
 * 在编辑器中配置，可用于蓝图。
 */
USTRUCT(BlueprintType)
struct FYcInputAction
{
	GENERATED_BODY()
	
	/** 输入动作资源 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/** 此输入动作对应的游戏标签 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * 输入配置资源
 * 
 * 运行时不可变的输入配置数据资产，包含所有输入动作的映射配置。
 * 分为两个类别：
 *   - NativeInputActions: 原生输入动作，需要手动绑定处理
 *   - AbilityInputActions: 技能输入动作，自动绑定到对应标签的技能
 */
UCLASS(BlueprintType, Const)
class YICHENGAMEPLAY_API UYcInputConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UYcInputConfig(const FObjectInitializer& ObjectInitializer);
	
#if WITH_EDITORONLY_DATA
	/** 编辑器中的输入配置描述，便于开发阶段标注 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Develop")
	FString Description;
#endif

	/**
	 * 查找指定标签对应的原生InputAction
	 * @param InputTag 要查找的输入标签
	 * @param bLogNotFound 未找到时是否输出错误日志
	 * @return 对应的输入动作，未找到返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Pawn")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	/**
	 * 查找指定标签对应的技能InputAction
	 * @param InputTag 要查找的输入标签
	 * @param bLogNotFound 未找到时是否输出错误日志
	 * @return 对应的输入动作，未找到返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Pawn")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	/**
	 * 原生输入动作列表
	 * 这些输入动作映射到游戏标签，需要手动绑定处理逻辑
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FYcInputAction> NativeInputActions;

	/**
	 * 技能输入动作列表
	 * 这些输入动作映射到游戏标签，自动绑定到具有匹配输入标签的技能
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FYcInputAction> AbilityInputActions;
};
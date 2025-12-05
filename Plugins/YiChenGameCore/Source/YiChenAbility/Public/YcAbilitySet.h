// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "YcAbilitySet.generated.h"

class UYcGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UYcAbilitySystemComponent;

/**
 * 该文件中实现了技能集合定义功能和授予功能, 通过定义技能集合数据资产来组织技能, 然后统一授予目标ASC组件
 */

/**
 * 技能集合中用于授予游戏技能的数据, 包含技能类、登记、输入标签
 */
USTRUCT(BlueprintType)
struct FYcAbilitySet_GameplayAbility
{
	GENERATED_BODY()
	
	// 要授予的游戏技能
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UYcGameplayAbility> Ability = nullptr;

	// 授予的技能等级
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 AbilityLevel = 1;

	// 用于处理该技能输入的标签
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"), BlueprintReadOnly)
	FGameplayTag InputTag;
};

/**
 * 技能集合中用于授予GE的数据
 */
USTRUCT(BlueprintType)
struct FYcAbilitySet_GameplayEffect
{
	GENERATED_BODY()
	
	// 授予的GE类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;
	
	// 授予的GE等级
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float EffectLevel = 1.0f;
};

/**
 * 技能集合中用于授予属性集的数据
 */
USTRUCT(BlueprintType)
struct FYcAbilitySet_AttributeSet
{
	GENERATED_BODY()
	
	// 要授予的属性集
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UAttributeSet> AttributeSet;
};

/**
 *	用于存储技能集合已授予内容的句柄数据, 用于追踪管理
 *	提供对已授予的游戏技能、游戏效果、属性集的句柄管理，以及移除功能
 */
USTRUCT(BlueprintType)
struct YICHENABILITY_API FYcAbilitySet_GrantedHandles
{
	GENERATED_BODY()
	
	// 添加GA句柄
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	// 添加GE句柄
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	// 添加AttributeSet
	void AddAttributeSet(UAttributeSet* Set);

	// 从ACS中移除已授予的GA、GE、AttributeSet
	void RemoveFromAbilitySystem(UYcAbilitySystemComponent* ASC);

protected:
	// 已授予游戏技能的句柄数组
	UPROPERTY(BlueprintReadOnly)
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	// 已授予游戏效果的句柄数组
	UPROPERTY(BlueprintReadOnly)
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	// 已授予属性集的指针数组
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};

/**
 * 技能集合数据资产, 用于定义需要授予的GA、GE、AttributeSet
 */
UCLASS(BlueprintType)
class YICHENABILITY_API UYcAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYcAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
#if WITH_EDITORONLY_DATA
	// 此技能集合的本地化描述,仅存在与编辑器状态下方便开发使用
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Description")
	FText Description = FText::GetEmpty();
#endif
	
	/**
	 * 将技能集合授予指定的技能系统组件
	 * 返回的句柄可用于后续移除所有已授予的内容
	 * @param ASC 目标技能系统组件
	 * @param OutGrantedHandles 输出的授予句柄
	 * @param SourceObject 源对象（可选）
	 */
	void GiveToAbilitySystem(UYcAbilitySystemComponent* ASC, FYcAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;
	
	// 授予此技能集合时要赋予的游戏技能列表
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FYcAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// 授予此技能集合时要赋予的游戏效果列表
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FYcAbilitySet_GameplayEffect> GrantedGameplayEffects;

	// 授予此技能集合时要赋予的属性集列表
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
	TArray<FYcAbilitySet_AttributeSet> GrantedAttributes;
};
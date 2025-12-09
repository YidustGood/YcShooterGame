// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcGameFeatureAction_WorldActionBase.h"
#include "YcGameFeatureAction_AddAbilities.generated.h"

class UGameplayAbility;
class UAttributeSet;
class UYcAbilitySet;
struct FGameplayAbilitySpecHandle;
struct FYcAbilitySet_GrantedHandles;
struct FComponentRequestHandle;

/** 要授予的技能结构体 */
USTRUCT(BlueprintType)
struct FYcAbilityGrant
{
	GENERATED_BODY()

	/** 要授予的技能类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AssetBundles="Client, Server"))
	TSoftClassPtr<UGameplayAbility> AbilityType;
};

USTRUCT(BlueprintType)
struct FYcAttributeSetGrant
{
	GENERATED_BODY()

	/** 要授予的属性集类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AssetBundles="Client,Server"))
	TSoftClassPtr<UAttributeSet> AttributeSetType;

	/** 用于初始化属性的数据表（可选） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AssetBundles="Client,Server"))
	TSoftObjectPtr<UDataTable> InitializationData;
};


/** GameFeature 中要添加技能的配置条目 */
USTRUCT()
struct FGameFeatureAddAbilitiesEntry
{
	GENERATED_BODY()

	/** 目标 Actor 类型 */
	UPROPERTY(EditAnywhere, Category="Abilities")
	TSoftClassPtr<AActor> ActorClass;

	/** 要授予的技能列表 */
	UPROPERTY(EditAnywhere, Category="Abilities")
	TArray<FYcAbilityGrant> GrantedAbilities;

	/** 要授予的属性集列表 */
	UPROPERTY(EditAnywhere, Category="Attributes")
	TArray<FYcAttributeSetGrant> GrantedAttributes;

	/** 要授予的技能集合列表 */
	UPROPERTY(EditAnywhere, Category="Attributes", meta=(AssetBundles="Client,Server"))
	TArray<TSoftObjectPtr<const UYcAbilitySet>> GrantedAbilitySets;
};

/**
 * GameFeatureAction，负责为指定类型的 Actor 授予技能、属性集和技能集合。
 * 
 * 该类通过 GameFrameworkComponentManager 监听 Actor 的创建和销毁事件，
 * 在 Actor 创建时自动授予配置的技能和属性，在 Actor 销毁时清理已授予的内容。
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Abilities"))
class UYcGameFeatureAction_AddAbilities : public UYcGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
//~ Begin UGameFeatureAction interface
	/** 当 GameFeature 激活时调用，初始化技能授予系统 */
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	
	/** 当 GameFeature 停用时调用，清理所有已授予的技能和属性 */
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeatureAction interface

	//~ Begin UObject interface
#if WITH_EDITOR
	/** 编辑器中验证配置数据的有效性 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~ End UObject interface

	/** 要添加的技能配置条目列表 */
	UPROPERTY(EditAnywhere, Category="Abilities", meta=(TitleProperty="ActorClass", ShowOnlyInnerProperties))
	TArray<FGameFeatureAddAbilitiesEntry> AbilitiesList;

private:
	/** 记录为单个 Actor 授予的技能、属性集和技能集合的句柄 */
	struct FActorExtensions
	{
		/** 已授予的技能句柄列表 */
		TArray<FGameplayAbilitySpecHandle> Abilities;
		
		/** 已授予的属性集实例列表 */
		TArray<UAttributeSet*> Attributes;
		
		/** 已授予的技能集合句柄列表 */
		TArray<FYcAbilitySet_GrantedHandles> AbilitySetHandles;
	};

	/** 单个 GameFeature 上下文的数据 */
	struct FPerContextData
	{
		/** 已添加技能的 Actor 及其扩展信息的映射 */
		TMap<AActor*, FActorExtensions> ActiveExtensions;
		
		/** 待处理的组件请求句柄列表 */
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
	};
	
	/** 按 GameFeature 上下文存储的数据映射 */
	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;	

	//~ Begin UGameFeatureAction_WorldActionBase interface
	/** 当 GameFeature 添加到世界时调用，注册 Actor 扩展处理器 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~ End UGameFeatureAction_WorldActionBase interface

	/** 清理指定上下文的所有已授予的技能和属性 */
	void Reset(FPerContextData& ActiveData);
	
	/** 处理 Actor 扩展事件（创建或销毁），根据事件类型添加或移除技能 */
	void HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex, FGameFeatureStateChangeContext ChangeContext);
	
	/** 为指定 Actor 授予配置中的所有技能、属性集和技能集合 */
	void AddActorAbilities(AActor* Actor, const FGameFeatureAddAbilitiesEntry& AbilitiesEntry, FPerContextData& ActiveData);
	
	/** 移除之前为 Actor 授予的所有技能、属性集和技能集合 */
	void RemoveActorAbilities(const AActor* Actor, FPerContextData& ActiveData);

	/**
	 * 为 Actor 查找或添加指定类型的组件（模板版本）。
	 * 
	 * @tparam ComponentType 要查找或添加的组件类型
	 * @param Actor 目标 Actor
	 * @param AbilitiesEntry 技能配置条目
	 * @param ActiveData 当前上下文的活跃数据
	 * @return 找到或新添加的组件指针，如果失败则返回 nullptr
	 * 
	 * @note 如果 Actor 上不存在该类型的组件，会通过 GameFrameworkComponentManager 请求添加
	 * @todo 考虑添加配置选项以控制是否自动添加缺失的组件
	 */
	template<class ComponentType>
	ComponentType* FindOrAddComponentForActor(AActor* Actor, const FGameFeatureAddAbilitiesEntry& AbilitiesEntry, FPerContextData& ActiveData)
	{
		//@TODO: 目前的设计是找不到ASC会自动添加ASC组件, 这里也许可用做成只找不加, 或者在增加一个变量来控制是否要自动添加
		return Cast<ComponentType>(FindOrAddComponentForActor(ComponentType::StaticClass(), Actor, AbilitiesEntry, ActiveData));
	}
	
	/**
	 * 为 Actor 查找或添加指定类型的组件（实现版本）。
	 * 
	 * @param ComponentType 要查找或添加的组件类型
	 * @param Actor 目标 Actor
	 * @param AbilitiesEntry 技能配置条目
	 * @param ActiveData 当前上下文的活跃数据
	 * @return 找到或新添加的组件指针，如果失败则返回 nullptr
	 */
	UActorComponent* FindOrAddComponentForActor(UClass* ComponentType, AActor* Actor, const FGameFeatureAddAbilitiesEntry& AbilitiesEntry, FPerContextData& ActiveData);
};

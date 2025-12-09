// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/Actions/YcGameFeatureAction_AddAbilities.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "YcAbilitySet.h"
#include "YcAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Player/YcPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_AddAbilities)

#define LOCTEXT_NAMESPACE "YichenGameFeatures"

void UYcGameFeatureAction_AddAbilities::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FPerContextData& ActiveData = ContextData.FindOrAdd(Context);

	if (!ensureAlways(ActiveData.ActiveExtensions.IsEmpty()) ||
		!ensureAlways(ActiveData.ComponentRequests.IsEmpty()))
	{
		Reset(ActiveData);
	}
	Super::OnGameFeatureActivating(Context);
}

void UYcGameFeatureAction_AddAbilities::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	FPerContextData* ActiveData = ContextData.Find(Context);

	if (ensure(ActiveData))
	{
		Reset(*ActiveData);
	}
}

#if WITH_EDITOR
EDataValidationResult UYcGameFeatureAction_AddAbilities::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	// 验证配置列表中的每个条目
	int32 EntryIndex = 0;
	for (const FGameFeatureAddAbilitiesEntry& Entry : AbilitiesList)
	{
		// 检查目标 Actor 类型是否为空
		if (Entry.ActorClass.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("EntryHasNullActor", "Null ActorClass at index {0} in AbilitiesList"), FText::AsNumber(EntryIndex)));
		}

		// 检查是否至少配置了一种要授予的内容
		if (Entry.GrantedAbilities.IsEmpty() && Entry.GrantedAttributes.IsEmpty() && Entry.GrantedAbilitySets.IsEmpty())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("EntryHasNoAddOns", "Index {0} in AbilitiesList will do nothing (no granted abilities, attributes, or ability sets)"), FText::AsNumber(EntryIndex)));
		}

		// 验证技能列表中的每个技能
		int32 AbilityIndex = 0;
		for (const FYcAbilityGrant& Ability : Entry.GrantedAbilities)
		{
			if (Ability.AbilityType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAbility", "Null AbilityType at index {0} in AbilitiesList[{1}].GrantedAbilities"), FText::AsNumber(AbilityIndex), FText::AsNumber(EntryIndex)));
			}
			++AbilityIndex;
		}

		// 验证属性集列表中的每个属性集
		int32 AttributesIndex = 0;
		for (const FYcAttributeSetGrant& Attributes : Entry.GrantedAttributes)
		{
			if (Attributes.AttributeSetType.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAttributeSet", "Null AttributeSetType at index {0} in AbilitiesList[{1}].GrantedAttributes"), FText::AsNumber(AttributesIndex), FText::AsNumber(EntryIndex)));
			}
			++AttributesIndex;
		}

		// 验证技能集列表中的每个技能集
		int32 AttributeSetIndex = 0;
		for (const TSoftObjectPtr<const UYcAbilitySet>& AttributeSetPtr : Entry.GrantedAbilitySets)
		{
			if (AttributeSetPtr.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullAttributeSet", "Null AbilitySet at index {0} in AbilitiesList[{1}].GrantedAbilitySets"), FText::AsNumber(AttributeSetIndex), FText::AsNumber(EntryIndex)));
			}
			++AttributeSetIndex;
		}
		++EntryIndex;
	}

	return Result;
}
#endif

void UYcGameFeatureAction_AddAbilities::Reset(FPerContextData& ActiveData)
{
	// 移除所有已添加技能的 Actor 的技能和属性
	while (!ActiveData.ActiveExtensions.IsEmpty())
	{
		auto ExtensionIt = ActiveData.ActiveExtensions.CreateIterator();
		RemoveActorAbilities(ExtensionIt->Key, ActiveData);
	}

	// 清空所有待处理的组件请求
	ActiveData.ComponentRequests.Empty();
}

void UYcGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	// 验证世界和游戏实例的有效性
	if ((GameInstance == nullptr) || (World == nullptr) || !World->IsGameWorld()) return;
	UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (ComponentMan == nullptr) return;
	
	// 为配置中的每个 Actor 类型注册扩展处理器
	int32 EntryIndex = 0;
	for (const FGameFeatureAddAbilitiesEntry& Entry : AbilitiesList)
	{
		if (Entry.ActorClass.IsNull()) continue;
		
		// 创建委托，当指定类型的 Actor 创建或销毁时调用 HandleActorExtension
		auto AddAbilitiesDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
				this, &UYcGameFeatureAction_AddAbilities::HandleActorExtension, EntryIndex, ChangeContext);
		auto ExtensionRequestHandle = ComponentMan->AddExtensionHandler(Entry.ActorClass, AddAbilitiesDelegate);

		ActiveData.ComponentRequests.Add(ExtensionRequestHandle);
		EntryIndex++;
	}
}

void UYcGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex, FGameFeatureStateChangeContext ChangeContext)
{
	FPerContextData* ActiveData = ContextData.Find(ChangeContext);
	if (AbilitiesList.IsValidIndex(EntryIndex) && ActiveData)
	{
		const FGameFeatureAddAbilitiesEntry& Entry = AbilitiesList[EntryIndex];
		
		// 当 Actor 被移除或接收者被移除时，清理已授予的技能
		if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved) || (EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved))
		{
			RemoveActorAbilities(Actor, *ActiveData);
		}
		// 当 Actor 扩展被添加或技能系统准备就绪时，授予技能
		else if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded) || (EventName == AYcPlayerState::NAME_YcAbilityReady))
		{
			AddActorAbilities(Actor, Entry, *ActiveData);
		}
	}
}

void UYcGameFeatureAction_AddAbilities::AddActorAbilities(AActor* Actor, const FGameFeatureAddAbilitiesEntry& AbilitiesEntry, FPerContextData& ActiveData)
{
	check(Actor);
	if (!Actor->HasAuthority()) return;

	// 如果 Actor 已经被添加过技能，则提前返回以避免重复添加
	if (ActiveData.ActiveExtensions.Find(Actor) != nullptr) return;	

	// 查找或创建 ASC 组件，如果不存在则通过 GameFrameworkComponentManager 请求添加
	UAbilitySystemComponent* AbilitySystemComponent = FindOrAddComponentForActor<UAbilitySystemComponent>(Actor, AbilitiesEntry, ActiveData);
	if (AbilitySystemComponent == nullptr)
	{
		UE_LOG(LogGameFeatures, Error, TEXT("Failed to find/add an ability component to '%s'. Abilities will not be granted."), *Actor->GetPathName());
		return;
	}
	
	FActorExtensions AddedExtensions;
	AddedExtensions.Abilities.Reserve(AbilitiesEntry.GrantedAbilities.Num());
	AddedExtensions.Attributes.Reserve(AbilitiesEntry.GrantedAttributes.Num());
	AddedExtensions.AbilitySetHandles.Reserve(AbilitiesEntry.GrantedAbilitySets.Num());

	// 遍历技能列表，为 ASC 授予每个技能
	// 注：目前使用同步加载，如果技能硬引用了大量资产可能会阻塞游戏，后续可考虑优化为异步加载
	for (const auto& [AbilityType] : AbilitiesEntry.GrantedAbilities)
	{
		if (AbilityType.IsNull()) continue;
		FGameplayAbilitySpec NewAbilitySpec(AbilityType.LoadSynchronous()); 
		FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(NewAbilitySpec);
		AddedExtensions.Abilities.Add(AbilityHandle);
	}

	// 遍历属性集列表，为 ASC 添加每个属性集实例
	for (const FYcAttributeSetGrant& Attributes : AbilitiesEntry.GrantedAttributes)
	{
		if (Attributes.AttributeSetType.IsNull()) continue;
		TSubclassOf<UAttributeSet> SetType = Attributes.AttributeSetType.LoadSynchronous();
		if (!SetType) continue;
		UAttributeSet* NewSet = NewObject<UAttributeSet>(AbilitySystemComponent->GetOwner(), SetType);
		
		// 如果指定了初始化数据表，则使用其数据初始化属性
		if (!Attributes.InitializationData.IsNull())
		{
			if (UDataTable* InitData = Attributes.InitializationData.LoadSynchronous())
			{
				NewSet->InitFromMetaDataTable(InitData);
			}
		}

		AddedExtensions.Attributes.Add(NewSet);
		AbilitySystemComponent->AddAttributeSetSubobject(NewSet);
	}

	// 遍历技能集列表，为 ASC 授予每个技能集合中的所有技能
	UYcAbilitySystemComponent* ASC = CastChecked<UYcAbilitySystemComponent>(AbilitySystemComponent);
	for (const TSoftObjectPtr<const UYcAbilitySet>& SetPtr : AbilitiesEntry.GrantedAbilitySets)
	{
		if (const UYcAbilitySet* Set = SetPtr.Get())
		{
			// 调用技能集的授予函数，将其包含的所有技能赋予目标 ASC
			Set->GiveToAbilitySystem(ASC, &AddedExtensions.AbilitySetHandles.AddDefaulted_GetRef());
		}
	}

	// 记录此 Actor 的所有已授予的技能和属性，以便后续清理
	ActiveData.ActiveExtensions.Add(Actor, AddedExtensions);
}

void UYcGameFeatureAction_AddAbilities::RemoveActorAbilities(const AActor* Actor, FPerContextData& ActiveData)
{
	FActorExtensions* ActorExtensions = ActiveData.ActiveExtensions.Find(Actor);
	if (ActorExtensions == nullptr) return;
	if (UAbilitySystemComponent* AbilitySystemComponent = Actor->FindComponentByClass<UAbilitySystemComponent>())
	{
		// 移除所有已授予的属性集
		for (UAttributeSet* AttribSetInstance : ActorExtensions->Attributes)
		{
			AbilitySystemComponent->RemoveSpawnedAttribute(AttribSetInstance);
		}

		// 移除所有已授予的技能
		for (FGameplayAbilitySpecHandle AbilityHandle : ActorExtensions->Abilities)
		{
			AbilitySystemComponent->SetRemoveAbilityOnEnd(AbilityHandle);
		}

		// 移除所有已授予的技能集合
		UYcAbilitySystemComponent* ASC = CastChecked<UYcAbilitySystemComponent>(AbilitySystemComponent);
		for (FYcAbilitySet_GrantedHandles& SetHandle : ActorExtensions->AbilitySetHandles)
		{
			SetHandle.RemoveFromAbilitySystem(ASC);
		}
	}
	// 从活跃扩展映射中移除此 Actor 的记录
	ActiveData.ActiveExtensions.Remove(Actor);
}

UActorComponent* UYcGameFeatureAction_AddAbilities::FindOrAddComponentForActor(UClass* ComponentType, AActor* Actor, const FGameFeatureAddAbilitiesEntry& AbilitiesEntry, FPerContextData& ActiveData)
{
	UActorComponent* Component = Actor->FindComponentByClass(ComponentType);
	
	bool bMakeComponentRequest = (Component == nullptr);
	if (Component)
	{
		// 检查此组件是否来自 GameFrameworkComponentManager 的请求
		// 对于动态添加的组件，CreationMethod 的默认值为 Native
		if (Component->CreationMethod == EComponentCreationMethod::Native)
		{
			// 区分真正的原生组件和由 GameFrameworkComponentManager 创建的组件
			// 如果组件的原型是 CDO（Class Default Object），则说明它来自 GameFrameworkComponentManager，需要重新发出请求
			// 这是因为 GameFrameworkComponentManager 的请求采用引用计数机制
			UObject* ComponentArchetype = Component->GetArchetype();
			bMakeComponentRequest = ComponentArchetype->HasAnyFlags(RF_ClassDefaultObject);
		}
	}

	if (bMakeComponentRequest)
	{
		UWorld* World = Actor->GetWorld();
		UGameInstance* GameInstance = World->GetGameInstance();

		// 通过 GameFrameworkComponentManager 请求添加组件
		if (UGameFrameworkComponentManager* ComponentMan = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance))
		{
			TSharedPtr<FComponentRequestHandle> RequestHandle = ComponentMan->AddComponentRequest(AbilitiesEntry.ActorClass, ComponentType);
			ActiveData.ComponentRequests.Add(RequestHandle);
		}

		// 如果之前没有找到组件，则在请求后再次查找
		if (!Component)
		{
			Component = Actor->FindComponentByClass(ComponentType);
			ensureAlways(Component);
		}
	}

	return Component;
}

#undef LOCTEXT_NAMESPACE
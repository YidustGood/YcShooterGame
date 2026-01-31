// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayAbilitySpecHandle.h"
#include "Templates/SubclassOf.h"
#include "YcGlobalAbilitySystem.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UYcAbilitySystemComponent;

USTRUCT()
struct FGlobalAppliedAbilityList
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TObjectPtr<UYcAbilitySystemComponent>, FGameplayAbilitySpecHandle> Handles;

	void AddToASC(TSubclassOf<UGameplayAbility> Ability, UYcAbilitySystemComponent* ASC);
	void RemoveFromASC(UYcAbilitySystemComponent* ASC);
	void RemoveFromAll();
};

USTRUCT()
struct FGlobalAppliedEffectList
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<TObjectPtr<UYcAbilitySystemComponent>, FActiveGameplayEffectHandle> Handles;

	void AddToASC(TSubclassOf<UGameplayEffect> Effect, UYcAbilitySystemComponent* ASC, float Level = 1);
	void RemoveFromASC(UYcAbilitySystemComponent* ASC);
	void RemoveFromAll();
};

/**
 * // @todo 暂时还未对接全局技能系统, 目前还用不上
 */
UCLASS()
class YICHENABILITY_API UYcGlobalAbilitySystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UYcGlobalAbilitySystem();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="YiChenGameCore")
	void ApplyAbilityToAll(TSubclassOf<UGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="YiChenGameCore")
	void ApplyEffectToAll(TSubclassOf<UGameplayEffect> Effect);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "YiChenGameCore")
	void RemoveAbilityFromAll(TSubclassOf<UGameplayAbility> Ability);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "YiChenGameCore")
	void RemoveEffectFromAll(TSubclassOf<UGameplayEffect> Effect);
	
	/** Register an ASC with global system and apply any active global effects/abilities. */
	void RegisterASC(UYcAbilitySystemComponent* ASC);

	/** Removes an ASC from the global system, along with any active global effects/abilities. */
	void UnregisterASC(UYcAbilitySystemComponent* ASC);
	
private:
	UPROPERTY()
	TMap<TSubclassOf<UGameplayAbility>, FGlobalAppliedAbilityList> AppliedAbilities;

	UPROPERTY()
	TMap<TSubclassOf<UGameplayEffect>, FGlobalAppliedEffectList> AppliedEffects;

	UPROPERTY()
	TArray<TObjectPtr<UYcAbilitySystemComponent>> RegisteredASCs;
};

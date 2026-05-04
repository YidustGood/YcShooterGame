// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "YcContextEffectsSubsystem.generated.h"

#define UE_API YICHENGAMEPLAY_API

class AActor;
class UAudioComponent;
class UYcContextEffectsLibrary;
class UNiagaraComponent;
class USceneComponent;

/** 上下文反馈系统配置。 */
UCLASS(MinimalAPI, config = Game, defaultconfig, meta = (DisplayName = "YcContextEffects"))
class UYcContextEffectsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 物理表面类型到上下文标签的映射配置。 */
	UPROPERTY(config, EditAnywhere, Category = "YcGameCore|Feedback")
	TMap<TEnumAsByte<EPhysicalSurface>, FGameplayTag> SurfaceTypeToContextMap;
};

/** 每个 Actor 持有的一组已激活反馈资源库。 */
UCLASS(MinimalAPI)
class UYcContextEffectsSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TSet<TObjectPtr<UYcContextEffectsLibrary>> ContextEffectsLibraries;
};

/** 世界级上下文反馈子系统，负责缓存与实际生成反馈对象。 */
UCLASS(MinimalAPI)
class UYcContextEffectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 根据标签和上下文生成反馈效果。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	UE_API void SpawnContextEffects(
		const AActor* SpawningActor
		, USceneComponent* AttachToComponent
		, const FName AttachPoint
		, const FVector LocationOffset
		, const FRotator RotationOffset
		, FGameplayTag Effect
		, FGameplayTagContainer Contexts
		, TArray<UAudioComponent*>& AudioOut
		, TArray<UNiagaraComponent*>& NiagaraOut
		, FVector VFXScale = FVector(1)
		, float AudioVolume = 1
		, float AudioPitch = 1);

	/** 把物理表面类型映射为上下文标签。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	UE_API bool GetContextFromSurfaceType(TEnumAsByte<EPhysicalSurface> PhysicalSurface, FGameplayTag& Context);

	/** 为指定 Actor 加载并注册反馈资源库。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	UE_API void LoadAndAddContextEffectsLibraries(AActor* OwningActor, TSet<TSoftObjectPtr<UYcContextEffectsLibrary>> ContextEffectsLibraries);

	/** 移除指定 Actor 的反馈资源库引用。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	UE_API void UnloadAndRemoveContextEffectsLibraries(AActor* OwningActor);

private:

	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, TObjectPtr<UYcContextEffectsSet>> ActiveActorEffectsMap;
};

#undef UE_API


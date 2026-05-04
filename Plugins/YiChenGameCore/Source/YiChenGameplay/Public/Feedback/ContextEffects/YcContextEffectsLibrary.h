// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "YcContextEffectsLibrary.generated.h"

#define UE_API YICHENGAMEPLAY_API

class UNiagaraSystem;
class USoundBase;

/** 上下文反馈资源库加载状态。 */
UENUM()
enum class EContextEffectsLibraryLoadState : uint8 {
	Unloaded = 0,
	Loading = 1,
	Loaded = 2
};

/** 单条上下文反馈配置。 */
USTRUCT(BlueprintType)
struct FYcContextEffects
{
	GENERATED_BODY()

	/** 效果主标签，例如 Footstep、Impact。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "YcGameCore|Feedback")
	FGameplayTag EffectTag;

	/** 触发该效果所需的上下文标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "YcGameCore|Feedback")
	FGameplayTagContainer Context;

	/** 实际资源列表，支持 SoundBase 与 NiagaraSystem。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "YcGameCore|Feedback", meta = (AllowedClasses = "/Script/Engine.SoundBase, /Script/Niagara.NiagaraSystem"))
	TArray<FSoftObjectPath> Effects;

};

/** 运行时缓存的已加载反馈资源。 */
UCLASS(MinimalAPI)
class UYcActiveContextEffects : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FGameplayTag EffectTag;

	UPROPERTY(VisibleAnywhere)
	FGameplayTagContainer Context;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<USoundBase>> Sounds;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UNiagaraSystem>> NiagaraSystems;
};

/** 上下文反馈资源库。 */
UCLASS(MinimalAPI, BlueprintType)
class UYcContextEffectsLibrary : public UObject
{
	GENERATED_BODY()
public:
	/** 资源库中配置的全部反馈条目。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "YcGameCore|Feedback")
	TArray<FYcContextEffects> ContextEffects;

	/** 根据效果标签和上下文查询可播放的音效与特效。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	UE_API void GetEffects(const FGameplayTag Effect, const FGameplayTagContainer Context, TArray<USoundBase*>& Sounds, TArray<UNiagaraSystem*>& NiagaraSystems);

	/** 同步加载资源库中的全部反馈资源。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	UE_API void LoadEffects();

	/** 获取当前资源库加载状态。 */
	UE_API EContextEffectsLibraryLoadState GetContextEffectsLibraryLoadState();

private:
	void LoadEffectsInternal();

	void YcContextEffectLibraryLoadingComplete(TArray<UYcActiveContextEffects*> YcActiveContextEffects);

	UPROPERTY(Transient)
	TArray< TObjectPtr<UYcActiveContextEffects>> ActiveContextEffects;

	UPROPERTY(Transient)
	EContextEffectsLibraryLoadState EffectsLoadState = EContextEffectsLibraryLoadState::Unloaded;
};


#undef UE_API

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcContextEffectsInterface.h"
#include "Components/ActorComponent.h"
#include "YcContextEffectComponent.generated.h"


class UAnimSequenceBase;
class UAudioComponent;
class UYcContextEffectsLibrary;
class UNiagaraComponent;
class USceneComponent;
struct FHitResult;

/** 负责为 Actor 聚合上下文标签与资源库，并在动画通知触发时生成反馈效果。 */
UCLASS(ClassGroup=(YiChenGameCore), hidecategories = (Variable, Tags, ComponentTick, ComponentReplication, Activation, Cooking, AssetUserData, Collision), CollapseCategories, meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcContextEffectComponent : public UActorComponent, public IYcContextEffectsInterface
{
	GENERATED_BODY()

public:
	UYcContextEffectComponent();
protected:

	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	/** IYcContextEffectsInterface 实现。 */
	UFUNCTION(BlueprintCallable)
	virtual void AnimMotionEffect_Implementation(const FName Bone, const FGameplayTag MotionEffect, USceneComponent* StaticMeshComponent,
		const FVector LocationOffset, const FRotator RotationOffset, const UAnimSequenceBase* AnimationSequence,
		const bool bHitSuccess, const FHitResult HitResult, FGameplayTagContainer Contexts,
		FVector VFXScale = FVector(1), float AudioVolume = 1, float AudioPitch = 1) override;

	/** 是否自动把命中的物理表面类型转换为上下文标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "YcGameCore|Feedback")
	bool bConvertPhysicalSurfaceToContext = true;

	/** 默认上下文标签。 */
	UPROPERTY(EditAnywhere, Category = "YcGameCore|Feedback")
	FGameplayTagContainer DefaultEffectContexts;

	/** 默认反馈资源库。 */
	UPROPERTY(EditAnywhere, Category = "YcGameCore|Feedback")
	TSet<TSoftObjectPtr<UYcContextEffectsLibrary>> DefaultContextEffectsLibraries;

	/** 更新当前附加的上下文标签。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	void UpdateEffectContexts(FGameplayTagContainer NewEffectContexts);

	/** 更新当前使用的反馈资源库。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Feedback")
	void UpdateLibraries(TSet<TSoftObjectPtr<UYcContextEffectsLibrary>> NewContextEffectsLibraries);

private:
	UPROPERTY(Transient)
	FGameplayTagContainer CurrentContexts;

	UPROPERTY(Transient)
	TSet<TSoftObjectPtr<UYcContextEffectsLibrary>> CurrentContextEffectsLibraries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveAudioComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveNiagaraComponents;
};

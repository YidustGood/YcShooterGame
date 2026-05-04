// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "YcContextEffectsInterface.generated.h"

class UAnimSequenceBase;
class USceneComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UYcContextEffectsInterface : public UInterface
{
	GENERATED_BODY()
};

/** 上下文反馈接口，用于接收动画通知传递的反馈请求。 */
class YICHENGAMEPLAY_API IYcContextEffectsInterface
{
	GENERATED_BODY()

public:

	/** 处理一次动画反馈请求。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "YcGameCore|Feedback")
	void AnimMotionEffect(const FName Bone
		, const FGameplayTag MotionEffect
		, USceneComponent* StaticMeshComponent
		, const FVector LocationOffset
		, const FRotator RotationOffset
		, const UAnimSequenceBase* AnimationSequence
		, const bool bHitSuccess
		, const FHitResult HitResult
		, FGameplayTagContainer Contexts
		, FVector VFXScale = FVector(1)
		, float AudioVolume = 1
		, float AudioPitch = 1);
};

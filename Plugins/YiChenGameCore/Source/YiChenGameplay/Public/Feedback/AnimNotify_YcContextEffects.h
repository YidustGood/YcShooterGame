// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_YcContextEffects.generated.h"

/** 动画通知射线检测配置。 */
USTRUCT(BlueprintType)
struct FYcContextEffectAnimNotifyTraceSettings
{
	GENERATED_BODY()

	/** 射线检测通道。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	/** 射线终点相对起点的偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	FVector EndTraceLocationOffset = FVector::ZeroVector;

	/** 是否忽略拥有该通知的 Actor。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	bool bIgnoreActor = true;

	/** 是否绘制射线检测调试图形。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	bool bDrawDebugTrace = false;

	/** 调试图形在场景中的保留时间，单位秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace, meta = (EditCondition = "bDrawDebugTrace", ClampMin = "0.0"))
	float DebugDrawTime = 1.0f;
};

/** 动画编辑器中的预览配置。 */
USTRUCT(BlueprintType)
struct FYcContextEffectAnimNotifyPreviewSettings
{
	GENERATED_BODY()

	/** 是否根据物理表面类型自动补充上下文标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	bool bPreviewPhysicalSurfaceAsContext = true;

	/** 预览使用的物理表面类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview, meta=(EditCondition="bPreviewPhysicalSurfaceAsContext"))
	TEnumAsByte<EPhysicalSurface> PreviewPhysicalSurface = EPhysicalSurface::SurfaceType_Default;

	/** 预览使用的反馈资源库。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview, meta = (AllowedClasses = "/Script/YiChenGameplay.YcContextEffectsLibrary"))
	FSoftObjectPath PreviewContextEffectsLibrary;

	/** 额外追加的预览上下文标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	FGameplayTagContainer PreviewContexts;
};

/** 粒子特效参数。 */
USTRUCT(BlueprintType)
struct FYcContextEffectAnimNotifyVFXSettings
{
	GENERATED_BODY()

	/** 特效缩放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FX)
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
};

/** 音频参数。 */
USTRUCT(BlueprintType)
struct FYcContextEffectAnimNotifyAudioSettings
{
	GENERATED_BODY()

	/** 音量倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	float VolumeMultiplier = 1.0f;

	/** 音高倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	float PitchMultiplier = 1.0f;
};

/** 根据动画通知触发上下文反馈效果。 */
UCLASS(const, hidecategories=Object, CollapseCategories, Config = Game, meta=(DisplayName="Play Context Effects"))
class YICHENGAMEPLAY_API UAnimNotify_YcContextEffects : public UAnimNotify
{
	GENERATED_BODY()
public:
	UAnimNotify_YcContextEffects();

	// Begin UObject interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject interface

	// Begin UAnimNotify interface
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
#if WITH_EDITOR
	virtual void ValidateAssociatedAssets() override;
#endif
	// End UAnimNotify interface

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable)
	void SetParameters(FGameplayTag EffectIn, FVector LocationOffsetIn, FRotator RotationOffsetIn, 
		FYcContextEffectAnimNotifyVFXSettings VFXPropertiesIn, FYcContextEffectAnimNotifyAudioSettings AudioPropertiesIn,
		bool bAttachedIn, FName SocketNameIn, bool bPerformTraceIn, FYcContextEffectAnimNotifyTraceSettings TracePropertiesIn);
#endif

	/** 要播放的反馈效果标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (DisplayName = "Effect", ExposeOnSpawn = true))
	FGameplayTag Effect;

	/** 相对挂点的位置偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (ExposeOnSpawn = true))
	FVector LocationOffset = FVector::ZeroVector;

	/** 相对挂点的旋转偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (ExposeOnSpawn = true))
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** 视觉特效参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (ExposeOnSpawn = true))
	FYcContextEffectAnimNotifyVFXSettings VFXProperties;

	/** 音频参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (ExposeOnSpawn = true))
	FYcContextEffectAnimNotifyAudioSettings AudioProperties;

	/** 是否附着到骨骼或插槽，开启后射线起点也会使用该位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback|Attachment", meta = (ExposeOnSpawn = true))
	uint32 bAttached : 1; 	//~ Does not follow coding standard due to redirection from BP

	/** 附着使用的骨骼或插槽名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback|Attachment", meta = (ExposeOnSpawn = true, EditCondition = "bAttached"))
	FName SocketName;

	/** 是否进行射线检测，用于命中物理表面并补充上下文。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (ExposeOnSpawn = true))
	uint32 bPerformTrace : 1; 	
	
	/** 射线检测配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YcGameCore|Feedback", meta = (ExposeOnSpawn = true, EditCondition = "bPerformTrace"))
	FYcContextEffectAnimNotifyTraceSettings TraceProperties;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "YcGameCore|Feedback|Preview")
	uint32 bPreviewInEditor : 1;

	UPROPERTY(EditAnywhere, Category = "YcGameCore|Feedback|Preview", meta = (EditCondition = "bPreviewInEditor"))
	FYcContextEffectAnimNotifyPreviewSettings PreviewProperties;
#endif
};

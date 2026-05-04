// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Feedback/AnimNotify_YcContextEffects.h"
#include "YcFootstepNotifyGenerationService.generated.h"

class UAnimSequence;
class UWorld;
class UDebugSkelMeshComponent;

/** 单帧足部采样结果。 */
USTRUCT()
struct FYcFootstepSample
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Frame = INDEX_NONE;

	UPROPERTY()
	float Time = 0.0f;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/** 参考骨骼位置，通常是 pelvis 或 root。 */
	UPROPERTY()
	FVector ReferenceLocation = FVector::ZeroVector;

	/** 足部相对参考骨骼的位置。 */
	UPROPERTY()
	FVector RelativeLocation = FVector::ZeroVector;

	/** 射线是否命中预览场景地板或其他阻挡。 */
	UPROPERTY()
	bool bHasGroundHit = false;

	/** 射线命中点到足部采样点的垂直距离，未命中时为一个很大的值。 */
	UPROPERTY()
	float GroundDistance = TNumericLimits<float>::Max();
};

/** 编辑器侧分析上下文，用于接入动画预览场景。 */
struct FYcFootstepGenerationContext
{
	/** 预览世界，供脚步分析时做地板射线检测。 */
	UWorld* PreviewWorld = nullptr;

	/** 预览骨骼组件，用于忽略自身碰撞，并尽量对齐预览场景状态。 */
	UDebugSkelMeshComponent* PreviewMeshComponent = nullptr;

	bool IsValid() const
	{
		return PreviewWorld != nullptr && PreviewMeshComponent != nullptr;
	}
};

/** 生成后的脚步通知候选。 */
USTRUCT()
struct FYcFootstepNotifyCandidate
{
	GENERATED_BODY()

	UPROPERTY()
	float Time = 0.0f;

	UPROPERTY()
	FName SocketName = NAME_None;

	UPROPERTY()
	int32 Frame = INDEX_NONE;

	UPROPERTY()
	float Height = 0.0f;
};

/**
 * 脚步通知生成设置。
 * 这里统一承载分析参数和通知写入参数，便于不同入口共用同一套配置界面。
 */
UCLASS()
class YICHENGAMECOREEDITOR_API UYcFootstepNotifyGeneratorSettings : public UObject
{
	GENERATED_BODY()

public:
	/** 左脚骨骼名称。通常填写角色骨架中的左脚骨，例如 foot_l。若名称错误，将无法生成左脚落点。 */
	UPROPERTY(EditAnywhere, Category = "骨骼")
	FName LeftFootBone = TEXT("foot_l");

	/** 右脚骨骼名称。通常填写角色骨架中的右脚骨，例如 foot_r。若名称错误，将无法生成右脚落点。 */
	UPROPERTY(EditAnywhere, Category = "骨骼")
	FName RightFootBone = TEXT("foot_r");

	/** 写入动画通知时使用的轨道名称。建议单独使用 Footsteps 之类的名称，方便集中查看和人工微调。 */
	UPROPERTY(EditAnywhere, Category = "输出")
	FName NotifyTrackName = TEXT("Footsteps");

	/** 写入动画通知时使用的反馈标签。生成后的 UAnimNotify_YcContextEffects 会用它来匹配脚步反馈资源。 */
	UPROPERTY(EditAnywhere, Category = "输出")
	FGameplayTag FootstepEffectTag;

	/** 生成前是否清空目标轨道。开启适合反复重生成；关闭适合在已有手工通知基础上继续追加。 */
	UPROPERTY(EditAnywhere, Category = "输出")
	bool bClearExistingTrack = true;

	/** 是否尽量限制左右脚交替落地。普通双足角色建议开启；若是特殊动作、怪物步态或非标准移动，可考虑关闭。 */
	UPROPERTY(EditAnywhere, Category = "分析")
	bool bPreferAlternatingFeet = true;

	/** 两次同脚触发之间的最小时间间隔。值越大，越能避免同一只脚短时间内生成多个通知；若漏掉真实步点，可适当调小。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.01"))
	float MinIntervalBetweenSameFootsteps = 0.16f;

	/** 判定足部“接近地面”的高度容差，单位厘米。值越大越宽松，更容易出候选；误检多时调小，漏检多时调大。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.0"))
	float ContactHeightTolerance = 4.0f;

	/** 识别低点时，前一帧至少需要下降的高度。当前通用方案中影响较弱，可理解为“低点前最好有一点下落趋势”；误检多时可略微调大。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.0"))
	float MinDescendingDelta = 0.1f;

	/** 识别低点时，后一帧至少需要回升的高度。当前通用方案中影响较弱，可理解为“低点后最好别完全贴平”；平台帧误检多时可略微调大。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.0"))
	float MinAscendingDelta = 0.05f;

	/** 每隔多少帧采样一次。1 表示逐帧采样，结果最稳定；值越大分析越快，但越容易跳过短促步点。普通移动动画建议保持 1。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "1", UIMin = "1"))
	int32 FrameStep = 1;

	/** 支撑脚在动画参考系中的最大水平速度。值越大越容易把“脚移动不太快”的帧算成候选，适合先求不漏；若候选过多可调小。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.0"))
	float MaxSupportingFootSpeed = 16.0f;

	/** 支撑状态最少持续多久才认为是一次有效落脚，单位秒。值越大越偏向保留较稳定的踩地阶段；若短促动作经常漏掉，可调小。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.0"))
	float MinSupportDuration = 0.08f;

	/** 是否在动画编辑器中优先使用预览场景地板射线辅助分析。适合预览地板摆放正确时使用；若不同动画方向表现不稳定，可关闭只走纯动画分析。 */
	UPROPERTY(EditAnywhere, Category = "分析")
	bool bUsePreviewSceneFloorTrace = true;

	/** 判定脚已经接近预览地板的最大距离，单位厘米。值越大越容易认为“脚已接地”；若误命中过多可调小，若明显踩地却不算可调大。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "0.0", EditCondition = "bUsePreviewSceneFloorTrace"))
	float PreviewFloorContactDistance = 6.0f;

	/** 分析时向下射线的最大长度，单位厘米。若角色脚离预览地板较远仍想命中，可调大；若只想检测脚底附近，保持较小更稳。 */
	UPROPERTY(EditAnywhere, Category = "分析", meta = (ClampMin = "1.0", EditCondition = "bUsePreviewSceneFloorTrace"))
	float PreviewFloorTraceDistance = 40.0f;

	/** 是否为生成出的通知启用运行时射线检测。这个影响游戏里通知播放时的地面判定，不影响编辑器里“生成步点”的核心逻辑。 */
	UPROPERTY(EditAnywhere, Category = "通知")
	bool bEnableTrace = true;

	/** 运行时射线检测通道。通常设置为项目里地面常用的碰撞通道，例如 Visibility。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (EditCondition = "bEnableTrace"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	/** 运行时射线终点相对起点的向下偏移距离，单位厘米。角色脚底离地较高、地形起伏较大时可调大。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (EditCondition = "bEnableTrace", ClampMin = "0.0"))
	float TraceDistance = 60.0f;

	/** 运行时射线是否忽略拥有该通知的 Actor。一般建议开启，避免射线先打到角色自己。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (EditCondition = "bEnableTrace"))
	bool bIgnoreOwner = true;

	/** 是否为生成的通知开启运行时射线调试显示。适合联调脚步反馈上下文，正式使用时通常关闭。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (EditCondition = "bEnableTrace"))
	bool bDrawDebugTrace = false;

	/** 运行时射线调试线的保留时间。若调试时看不清可适当调大。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (EditCondition = "bEnableTrace && bDrawDebugTrace", ClampMin = "0.0"))
	float DebugDrawTime = 1.0f;

	/** 通知相对骨骼挂点的位置偏移。适合把脚步特效或声音触发点往前、往下微调到更接近实际脚底接触位置。 */
	UPROPERTY(EditAnywhere, Category = "通知")
	FVector NotifyLocationOffset = FVector::ZeroVector;

	/** 通知相对骨骼挂点的旋转偏移。适合修正脚印、粒子朝向等效果；纯声音场景通常保持默认即可。 */
	UPROPERTY(EditAnywhere, Category = "通知")
	FRotator NotifyRotationOffset = FRotator::ZeroRotator;

	/** 特效缩放。用于统一调整生成通知默认特效的大小。 */
	UPROPERTY(EditAnywhere, Category = "通知")
	FVector VfxScale = FVector(1.0f, 1.0f, 1.0f);

	/** 音量倍率。用于统一调整该批脚步通知的默认音量。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	/** 音高倍率。用于统一调整该批脚步通知的默认音高。 */
	UPROPERTY(EditAnywhere, Category = "通知", meta = (ClampMin = "0.0"))
	float PitchMultiplier = 1.0f;
};

/** 脚步通知生成核心服务。 */
class YICHENGAMECOREEDITOR_API FYcFootstepNotifyGenerationService
{
public:
	static bool GenerateForSequences(const TArray<UAnimSequence*>& AnimSequences, const UYcFootstepNotifyGeneratorSettings* Settings, FText& OutSummary, const FYcFootstepGenerationContext* Context = nullptr);
	static bool GenerateForSequence(UAnimSequence* AnimSequence, const UYcFootstepNotifyGeneratorSettings* Settings, int32& OutGeneratedNotifyCount, FText& OutMessage, const FYcFootstepGenerationContext* Context = nullptr);

private:
	static bool ValidateInputs(UAnimSequence* AnimSequence, const UYcFootstepNotifyGeneratorSettings* Settings, FText& OutMessage);
	static FName GetAnalysisReferenceBoneName(const UAnimSequence* AnimSequence);
	static void BuildSamplesForBone(UAnimSequence* AnimSequence, FName BoneName, FName ReferenceBoneName, int32 NumFrames, float SequenceLength, int32 FrameStep, const UYcFootstepNotifyGeneratorSettings* Settings, const FYcFootstepGenerationContext* Context, TArray<FYcFootstepSample>& OutSamples);
	static void CollectCandidatesForFoot(const TArray<FYcFootstepSample>& Samples, const TArray<FYcFootstepSample>& OtherFootSamples, FName FootBoneName, const UYcFootstepNotifyGeneratorSettings* Settings, TArray<FYcFootstepNotifyCandidate>& InOutCandidates);
	static void FilterCandidates(const UYcFootstepNotifyGeneratorSettings* Settings, TArray<FYcFootstepNotifyCandidate>& InOutCandidates);
	static void ApplyNotifies(UAnimSequence* AnimSequence, const UYcFootstepNotifyGeneratorSettings* Settings, const TArray<FYcFootstepNotifyCandidate>& Candidates, int32& OutGeneratedNotifyCount);
};

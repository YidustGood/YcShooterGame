// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Feedback/FootstepNotifyGenerator/YcFootstepNotifyGenerationService.h"

#include "Animation/AnimSequence.h"
#include "AnimPose.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Misc/ScopedSlowTask.h"
#include "ScopedTransaction.h"
#include "YcGameCoreEditor.h"

#define LOCTEXT_NAMESPACE "YcFootstepNotifyGenerationService"

namespace
{
	struct FYcFootstepAnalysisMetrics
	{
		TArray<float> HorizontalSpeeds;
		TArray<float> VerticalSpeeds;
		TArray<float> LocalMinHeights;
		float EffectiveSupportSpeed = 0.0f;
		float EffectiveVerticalSpeed = 0.0f;
		int32 LocalWindowRadius = 1;
	};

	static float SampleSortedPercentile(const TArray<float>& SortedValues, float Percentile)
	{
		if (SortedValues.IsEmpty())
		{
			return 0.0f;
		}

		const float ClampedPercentile = FMath::Clamp(Percentile, 0.0f, 1.0f);
		const float FloatIndex = ClampedPercentile * static_cast<float>(SortedValues.Num() - 1);
		const int32 LowerIndex = FMath::FloorToInt(FloatIndex);
		const int32 UpperIndex = FMath::Min(LowerIndex + 1, SortedValues.Num() - 1);
		const float Alpha = FloatIndex - static_cast<float>(LowerIndex);
		return FMath::Lerp(SortedValues[LowerIndex], SortedValues[UpperIndex], Alpha);
	}

	static FYcFootstepAnalysisMetrics BuildAnalysisMetrics(const TArray<FYcFootstepSample>& Samples, const UYcFootstepNotifyGeneratorSettings* Settings)
	{
		FYcFootstepAnalysisMetrics Metrics;
		Metrics.HorizontalSpeeds.Init(0.0f, Samples.Num());
		Metrics.VerticalSpeeds.Init(0.0f, Samples.Num());
		Metrics.LocalMinHeights.Init(0.0f, Samples.Num());

		if (Samples.IsEmpty())
		{
			return Metrics;
		}

		TArray<float> SortedHorizontalSpeeds;
		SortedHorizontalSpeeds.Reserve(FMath::Max(Samples.Num() - 1, 0));

		for (int32 SampleIndex = 1; SampleIndex < Samples.Num(); ++SampleIndex)
		{
			const float DeltaTime = FMath::Max(Samples[SampleIndex].Time - Samples[SampleIndex - 1].Time, KINDA_SMALL_NUMBER);
			const FVector2D PreviousPosition(Samples[SampleIndex - 1].RelativeLocation.X, Samples[SampleIndex - 1].RelativeLocation.Y);
			const FVector2D CurrentPosition(Samples[SampleIndex].RelativeLocation.X, Samples[SampleIndex].RelativeLocation.Y);

			const float HorizontalSpeed = FVector2D::Distance(CurrentPosition, PreviousPosition) / DeltaTime;
			const float VerticalSpeed = (Samples[SampleIndex].RelativeLocation.Z - Samples[SampleIndex - 1].RelativeLocation.Z) / DeltaTime;

			Metrics.HorizontalSpeeds[SampleIndex] = HorizontalSpeed;
			Metrics.VerticalSpeeds[SampleIndex] = VerticalSpeed;
			SortedHorizontalSpeeds.Add(HorizontalSpeed);
		}

		if (Samples.Num() >= 2)
		{
			Metrics.HorizontalSpeeds[0] = Metrics.HorizontalSpeeds[1];
			Metrics.VerticalSpeeds[0] = Metrics.VerticalSpeeds[1];
		}

		SortedHorizontalSpeeds.Sort();
		const float P35Speed = SampleSortedPercentile(SortedHorizontalSpeeds, 0.35f);
		const float P65Speed = SampleSortedPercentile(SortedHorizontalSpeeds, 0.65f);

		// 支撑脚的速度阈值既参考用户配置，也允许根据动画自身节奏自适应放宽一点，
		// 这样不同角色尺度、不同根骨移动方式下都不至于过早漏检。
		Metrics.EffectiveSupportSpeed = FMath::Max(Settings->MaxSupportingFootSpeed, P35Speed * 1.15f);
		Metrics.EffectiveVerticalSpeed = FMath::Max(18.0f, FMath::Max(Settings->MaxSupportingFootSpeed, P65Speed) * 1.1f);

		const float AverageDeltaTime = Samples.Num() >= 2
			? FMath::Max((Samples.Last().Time - Samples[0].Time) / static_cast<float>(Samples.Num() - 1), KINDA_SMALL_NUMBER)
			: (1.0f / 30.0f);
		const int32 RadiusByDuration = FMath::CeilToInt(Settings->MinSupportDuration / AverageDeltaTime);
		Metrics.LocalWindowRadius = FMath::Clamp(RadiusByDuration, 2, 6);

		for (int32 SampleIndex = 0; SampleIndex < Samples.Num(); ++SampleIndex)
		{
			float LocalMinHeight = Samples[SampleIndex].RelativeLocation.Z;
			const int32 WindowStart = FMath::Max(0, SampleIndex - Metrics.LocalWindowRadius);
			const int32 WindowEnd = FMath::Min(Samples.Num() - 1, SampleIndex + Metrics.LocalWindowRadius);
			for (int32 NeighborIndex = WindowStart; NeighborIndex <= WindowEnd; ++NeighborIndex)
			{
				LocalMinHeight = FMath::Min(LocalMinHeight, Samples[NeighborIndex].RelativeLocation.Z);
			}
			Metrics.LocalMinHeights[SampleIndex] = LocalMinHeight;
		}

		return Metrics;
	}

	struct FYcSupportEdgeExclusion
	{
		float StartTime = 0.0f;
		float EndTime = 0.0f;
	};

	static FYcSupportEdgeExclusion GetSupportEdgeExclusion(const TArray<FYcFootstepSample>& Samples)
	{
		FYcSupportEdgeExclusion Result;
		if (Samples.Num() < 2)
		{
			return Result;
		}

		const float AverageDeltaTime = FMath::Max(
			(Samples.Last().Time - Samples[0].Time) / static_cast<float>(Samples.Num() - 1),
			KINDA_SMALL_NUMBER);

		// 动画开头通常更不可靠，因此起始段多排一点；
		// 结尾则适当放宽，避免最后一次真实落脚被误排除。
		Result.StartTime = AverageDeltaTime * 2.25f;
		Result.EndTime = AverageDeltaTime * 1.0f;
		return Result;
	}
}

bool FYcFootstepNotifyGenerationService::GenerateForSequences(const TArray<UAnimSequence*>& AnimSequences,
	const UYcFootstepNotifyGeneratorSettings* Settings, FText& OutSummary, const FYcFootstepGenerationContext* Context)
{
	if (Settings == nullptr)
	{
		OutSummary = LOCTEXT("MissingSettings", "脚步通知生成失败：缺少生成设置。");
		return false;
	}

	TArray<UAnimSequence*> ValidSequences;
	for (UAnimSequence* AnimSequence : AnimSequences)
	{
		if (AnimSequence != nullptr)
		{
			ValidSequences.Add(AnimSequence);
		}
	}

	if (ValidSequences.IsEmpty())
	{
		OutSummary = LOCTEXT("NoAnimSequence", "没有可处理的动画序列。");
		return false;
	}

	FScopedTransaction Transaction(LOCTEXT("GenerateFootstepNotifyTransaction", "生成脚步上下文通知"));
	FScopedSlowTask SlowTask(static_cast<float>(ValidSequences.Num()), LOCTEXT("GeneratingFootstepNotify", "正在生成脚步上下文通知..."));
	SlowTask.MakeDialog(true);

	int32 SuccessCount = 0;
	int32 TotalNotifyCount = 0;
	TArray<FString> FailureMessages;

	for (UAnimSequence* AnimSequence : ValidSequences)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("GeneratingForAsset", "正在处理：{0}"), FText::FromString(AnimSequence->GetName())));

		int32 GeneratedNotifyCount = 0;
		FText Message;
		if (GenerateForSequence(AnimSequence, Settings, GeneratedNotifyCount, Message, Context))
		{
			++SuccessCount;
			TotalNotifyCount += GeneratedNotifyCount;
		}
		else
		{
			FailureMessages.Add(FString::Printf(TEXT("%s：%s"), *AnimSequence->GetName(), *Message.ToString()));
		}
	}

	if (SuccessCount <= 0)
	{
		OutSummary = FailureMessages.IsEmpty()
			? LOCTEXT("GenerateFailed", "脚步通知生成失败。")
			: FText::FromString(FString::Join(FailureMessages, TEXT("\n")));
		UE_LOG(LogYcGameCoreEditor, Warning, TEXT("脚步通知生成失败：%s"), *OutSummary.ToString());
		return false;
	}

	if (FailureMessages.IsEmpty())
	{
		OutSummary = FText::Format(
			LOCTEXT("GenerateSucceeded", "已处理 {0} 个动画，共生成 {1} 个脚步通知。"),
			SuccessCount,
			TotalNotifyCount);
	}
	else
	{
		OutSummary = FText::Format(
			LOCTEXT("GeneratePartialSucceeded", "已处理 {0} 个动画，共生成 {1} 个脚步通知。\n以下动画未成功处理：\n{2}"),
			SuccessCount,
			TotalNotifyCount,
			FText::FromString(FString::Join(FailureMessages, TEXT("\n"))));
	}

	UE_LOG(LogYcGameCoreEditor, Log, TEXT("脚步通知生成完成：%s"), *OutSummary.ToString());
	return true;
}

bool FYcFootstepNotifyGenerationService::GenerateForSequence(UAnimSequence* AnimSequence,
	const UYcFootstepNotifyGeneratorSettings* Settings, int32& OutGeneratedNotifyCount, FText& OutMessage, const FYcFootstepGenerationContext* Context)
{
	OutGeneratedNotifyCount = 0;

	if (!ValidateInputs(AnimSequence, Settings, OutMessage))
	{
		return false;
	}

	int32 NumFrames = 0;
	float SequenceLength = 0.0f;
	UAnimationBlueprintLibrary::GetNumFrames(AnimSequence, NumFrames);
	UAnimationBlueprintLibrary::GetSequenceLength(AnimSequence, SequenceLength);

	if (NumFrames < 3 || SequenceLength <= 0.0f)
	{
		OutMessage = LOCTEXT("InvalidSequenceFrames", "动画帧数不足，无法自动分析脚步落点。");
		return false;
	}

	TArray<FYcFootstepSample> LeftSamples;
	TArray<FYcFootstepSample> RightSamples;
	const FName ReferenceBoneName = GetAnalysisReferenceBoneName(AnimSequence);
	BuildSamplesForBone(AnimSequence, Settings->LeftFootBone, ReferenceBoneName, NumFrames, SequenceLength, Settings->FrameStep, Settings, Context, LeftSamples);
	BuildSamplesForBone(AnimSequence, Settings->RightFootBone, ReferenceBoneName, NumFrames, SequenceLength, Settings->FrameStep, Settings, Context, RightSamples);

	TArray<FYcFootstepNotifyCandidate> Candidates;
	CollectCandidatesForFoot(LeftSamples, RightSamples, Settings->LeftFootBone, Settings, Candidates);
	CollectCandidatesForFoot(RightSamples, LeftSamples, Settings->RightFootBone, Settings, Candidates);
	FilterCandidates(Settings, Candidates);

	if (Candidates.IsEmpty())
	{
		OutMessage = LOCTEXT("NoCandidateFound", "没有识别到合适的脚步落地帧，请调整分析参数后重试。");
		UE_LOG(LogYcGameCoreEditor, Warning, TEXT("动画 %s 未识别到脚步落地点。LeftBone=%s RightBone=%s"),
			*AnimSequence->GetName(),
			*Settings->LeftFootBone.ToString(),
			*Settings->RightFootBone.ToString());
		return false;
	}

	ApplyNotifies(AnimSequence, Settings, Candidates, OutGeneratedNotifyCount);

	OutMessage = FText::Format(
		LOCTEXT("GenerateSingleSucceeded", "已在 {0} 中生成 {1} 个脚步通知。"),
		FText::FromString(AnimSequence->GetName()),
		OutGeneratedNotifyCount);
	UE_LOG(LogYcGameCoreEditor, Log, TEXT("动画 %s 生成了 %d 个脚步通知。"), *AnimSequence->GetName(), OutGeneratedNotifyCount);
	return true;
}

bool FYcFootstepNotifyGenerationService::ValidateInputs(UAnimSequence* AnimSequence,
	const UYcFootstepNotifyGeneratorSettings* Settings, FText& OutMessage)
{
	if (AnimSequence == nullptr || Settings == nullptr)
	{
		OutMessage = LOCTEXT("MissingInput", "缺少动画序列或生成设置。");
		return false;
	}

	if (Settings->LeftFootBone.IsNone() || Settings->RightFootBone.IsNone())
	{
		OutMessage = LOCTEXT("MissingBoneName", "左右脚骨骼名称不能为空。");
		return false;
	}

	if (Settings->NotifyTrackName.IsNone())
	{
		OutMessage = LOCTEXT("MissingTrackName", "通知轨道名称不能为空。");
		return false;
	}

	const USkeleton* Skeleton = AnimSequence->GetSkeleton();
	if (Skeleton == nullptr)
	{
		OutMessage = LOCTEXT("MissingSkeleton", "当前动画缺少骨架信息。");
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	if (ReferenceSkeleton.FindBoneIndex(Settings->LeftFootBone) == INDEX_NONE)
	{
		OutMessage = FText::Format(LOCTEXT("LeftBoneNotFound", "未在骨架中找到左脚骨骼：{0}"), FText::FromName(Settings->LeftFootBone));
		return false;
	}

	if (ReferenceSkeleton.FindBoneIndex(Settings->RightFootBone) == INDEX_NONE)
	{
		OutMessage = FText::Format(LOCTEXT("RightBoneNotFound", "未在骨架中找到右脚骨骼：{0}"), FText::FromName(Settings->RightFootBone));
		return false;
	}

	return true;
}

FName FYcFootstepNotifyGenerationService::GetAnalysisReferenceBoneName(const UAnimSequence* AnimSequence)
{
	const USkeleton* Skeleton = AnimSequence ? AnimSequence->GetSkeleton() : nullptr;
	if (Skeleton == nullptr)
	{
		return NAME_None;
	}

	const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	constexpr const TCHAR* PreferredReferenceBones[] =
	{
		TEXT("pelvis"),
		TEXT("hips"),
		TEXT("root")
	};

	for (const TCHAR* PreferredBoneName : PreferredReferenceBones)
	{
		const FName BoneName(PreferredBoneName);
		if (ReferenceSkeleton.FindBoneIndex(BoneName) != INDEX_NONE)
		{
			return BoneName;
		}
	}

	return ReferenceSkeleton.GetNum() > 0 ? ReferenceSkeleton.GetBoneName(0) : NAME_None;
}

void FYcFootstepNotifyGenerationService::BuildSamplesForBone(UAnimSequence* AnimSequence, FName BoneName, FName ReferenceBoneName, int32 NumFrames,
	float SequenceLength, int32 FrameStep, const UYcFootstepNotifyGeneratorSettings* Settings, const FYcFootstepGenerationContext* Context, TArray<FYcFootstepSample>& OutSamples)
{
	OutSamples.Reset();

	const int32 SafeFrameStep = FMath::Max(FrameStep, 1);
	const int32 MaxFrameIndex = NumFrames;
	FAnimPoseEvaluationOptions EvaluationOptions;
	const bool bUsePreviewFloorTrace = Settings != nullptr && Settings->bUsePreviewSceneFloorTrace && Context != nullptr && Context->IsValid();
	UWorld* PreviewWorld = bUsePreviewFloorTrace ? Context->PreviewWorld : nullptr;
	UDebugSkelMeshComponent* PreviewMeshComponent = bUsePreviewFloorTrace ? Context->PreviewMeshComponent : nullptr;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(YcFootstepPreviewFloorTrace), false);
	if (PreviewMeshComponent != nullptr)
	{
		QueryParams.AddIgnoredActor(PreviewMeshComponent->GetOwner());
		QueryParams.AddIgnoredComponent(PreviewMeshComponent);
	}

	auto PopulateGroundTraceData = [&](FYcFootstepSample& Sample)
	{
		Sample.bHasGroundHit = false;
		Sample.GroundDistance = TNumericLimits<float>::Max();

		if (!bUsePreviewFloorTrace || PreviewWorld == nullptr)
		{
			return;
		}

		const FVector TraceStart = Sample.Location + FVector(0.0f, 0.0f, 2.0f);
		const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, Settings->PreviewFloorTraceDistance);
		FHitResult HitResult;
		if (PreviewWorld->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			Sample.bHasGroundHit = true;
			Sample.GroundDistance = FMath::Max(TraceStart.Z - HitResult.ImpactPoint.Z, 0.0f);
		}
	};

	for (int32 FrameIndex = 0; FrameIndex <= MaxFrameIndex; FrameIndex += SafeFrameStep)
	{
		FAnimPose AnimPose;
		UAnimPoseExtensions::GetAnimPoseAtFrame(AnimSequence, FrameIndex, EvaluationOptions, AnimPose);
		const FTransform BonePose = UAnimPoseExtensions::GetBonePose(AnimPose, BoneName, EAnimPoseSpaces::World);
		const FTransform ReferenceBonePose = ReferenceBoneName.IsNone()
			? FTransform::Identity
			: UAnimPoseExtensions::GetBonePose(AnimPose, ReferenceBoneName, EAnimPoseSpaces::World);

		FYcFootstepSample& Sample = OutSamples.AddDefaulted_GetRef();
		Sample.Frame = FrameIndex;
		Sample.Time = (MaxFrameIndex > 0) ? (SequenceLength * static_cast<float>(FrameIndex) / static_cast<float>(MaxFrameIndex)) : 0.0f;
		Sample.Location = BonePose.GetLocation();
		Sample.ReferenceLocation = ReferenceBonePose.GetLocation();
		Sample.RelativeLocation = Sample.Location - Sample.ReferenceLocation;
		PopulateGroundTraceData(Sample);
	}

	if (!OutSamples.IsEmpty() && OutSamples.Last().Frame != MaxFrameIndex)
	{
		FAnimPose AnimPose;
		UAnimPoseExtensions::GetAnimPoseAtFrame(AnimSequence, MaxFrameIndex, EvaluationOptions, AnimPose);
		const FTransform BonePose = UAnimPoseExtensions::GetBonePose(AnimPose, BoneName, EAnimPoseSpaces::World);
		const FTransform ReferenceBonePose = ReferenceBoneName.IsNone()
			? FTransform::Identity
			: UAnimPoseExtensions::GetBonePose(AnimPose, ReferenceBoneName, EAnimPoseSpaces::World);

		FYcFootstepSample& Sample = OutSamples.AddDefaulted_GetRef();
		Sample.Frame = MaxFrameIndex;
		Sample.Time = SequenceLength;
		Sample.Location = BonePose.GetLocation();
		Sample.ReferenceLocation = ReferenceBonePose.GetLocation();
		Sample.RelativeLocation = Sample.Location - Sample.ReferenceLocation;
		PopulateGroundTraceData(Sample);
	}
}

void FYcFootstepNotifyGenerationService::CollectCandidatesForFoot(const TArray<FYcFootstepSample>& Samples, const TArray<FYcFootstepSample>& OtherFootSamples, FName FootBoneName,
	const UYcFootstepNotifyGeneratorSettings* Settings, TArray<FYcFootstepNotifyCandidate>& InOutCandidates)
{
	if (Samples.Num() < 3)
	{
		return;
	}

	const FYcFootstepAnalysisMetrics SelfMetrics = BuildAnalysisMetrics(Samples, Settings);
	const FYcFootstepAnalysisMetrics OtherMetrics = BuildAnalysisMetrics(OtherFootSamples, Settings);
	const FYcSupportEdgeExclusion EdgeExclusion = GetSupportEdgeExclusion(Samples);
	const float SequenceStartTime = Samples[0].Time + EdgeExclusion.StartTime;
	const float SequenceEndTime = Samples.Last().Time - EdgeExclusion.EndTime;
	float LastAcceptedTime = -FLT_MAX;
	const float ValleyTolerance = FMath::Max(Settings->ContactHeightTolerance * 0.35f, 0.5f);
	const float MinValleyDepth = FMath::Max(Settings->ContactHeightTolerance * 0.08f, 0.3f);

	// 采用更通用、更容易人工微调的近似方案：
	// 沿整段动画寻找“局部低点 + 速度较小”的帧。
	// 如果有预览地板射线，则把接地命中当作加分项，而不是硬性约束。
	for (int32 SampleIndex = 1; SampleIndex < Samples.Num() - 1; ++SampleIndex)
	{
		const FYcFootstepSample& PreviousSample = Samples[SampleIndex - 1];
		const FYcFootstepSample& CurrentSample = Samples[SampleIndex];
		const FYcFootstepSample& NextSample = Samples[SampleIndex + 1];

		if (CurrentSample.Time <= SequenceStartTime || CurrentSample.Time >= SequenceEndTime)
		{
			continue;
		}

		if ((CurrentSample.Time - LastAcceptedTime) < Settings->MinIntervalBetweenSameFootsteps)
		{
			continue;
		}

		const float LocalHeightOffset = CurrentSample.RelativeLocation.Z - SelfMetrics.LocalMinHeights[SampleIndex];
		const bool bNearLocalGround = LocalHeightOffset <= Settings->ContactHeightTolerance;
		const bool bLocalValley =
			(CurrentSample.RelativeLocation.Z <= PreviousSample.RelativeLocation.Z + ValleyTolerance) &&
			(CurrentSample.RelativeLocation.Z <= NextSample.RelativeLocation.Z + ValleyTolerance);
		const bool bLowSpeed = SelfMetrics.HorizontalSpeeds[SampleIndex] <= (SelfMetrics.EffectiveSupportSpeed * 1.15f);
		const bool bStableVerticalMotion = FMath::Abs(SelfMetrics.VerticalSpeeds[SampleIndex]) <= (SelfMetrics.EffectiveVerticalSpeed * 1.25f);

		if (!bNearLocalGround || !bLocalValley || !bLowSpeed || !bStableVerticalMotion)
		{
			continue;
		}

		const float ValleyDepth = ((PreviousSample.RelativeLocation.Z - CurrentSample.RelativeLocation.Z) + (NextSample.RelativeLocation.Z - CurrentSample.RelativeLocation.Z)) * 0.5f;
		const float SpeedScore = FMath::Max((SelfMetrics.EffectiveSupportSpeed * 1.15f) - SelfMetrics.HorizontalSpeeds[SampleIndex], 0.0f);
		const float HeightScore = FMath::Max(Settings->ContactHeightTolerance - LocalHeightOffset, 0.0f);
		const float GroundScore = (CurrentSample.bHasGroundHit && CurrentSample.GroundDistance <= (Settings->PreviewFloorContactDistance * 1.5f))
			? FMath::Max((Settings->PreviewFloorContactDistance * 1.5f) - CurrentSample.GroundDistance, 0.0f)
			: 0.0f;

		float OtherFootActivityScore = 0.0f;
		if (OtherFootSamples.Num() == Samples.Num() && OtherMetrics.HorizontalSpeeds.IsValidIndex(SampleIndex))
		{
			OtherFootActivityScore = FMath::Max(OtherMetrics.HorizontalSpeeds[SampleIndex] - SelfMetrics.HorizontalSpeeds[SampleIndex], 0.0f);
		}

		const bool bGroundHelpful = CurrentSample.bHasGroundHit && CurrentSample.GroundDistance <= Settings->PreviewFloorContactDistance;
		if (ValleyDepth < MinValleyDepth && !bGroundHelpful)
		{
			continue;
		}

		const float CandidateScore =
			(ValleyDepth * 1.6f) +
			SpeedScore +
			(HeightScore * 1.25f) +
			(GroundScore * 0.75f) +
			(OtherFootActivityScore * 0.25f);

		const bool bStrongEnoughCandidate =
			CandidateScore >= 1.4f ||
			bGroundHelpful;
		if (!bStrongEnoughCandidate)
		{
			continue;
		}

		FYcFootstepNotifyCandidate& Candidate = InOutCandidates.AddDefaulted_GetRef();
		Candidate.Time = CurrentSample.Time;
		Candidate.SocketName = FootBoneName;
		Candidate.Frame = CurrentSample.Frame;
		Candidate.Height = CurrentSample.RelativeLocation.Z;

		UE_LOG(
			LogYcGameCoreEditor,
			Log,
			TEXT("脚步候选 %s: 帧=%d 时间=%.3f 高度=%.2f 速度=%.2f ValleyDepth=%.2f GroundDistance=%.2f"),
			*FootBoneName.ToString(),
			CurrentSample.Frame,
			CurrentSample.Time,
			CurrentSample.RelativeLocation.Z,
			SelfMetrics.HorizontalSpeeds[SampleIndex],
			ValleyDepth,
			CurrentSample.GroundDistance);

		LastAcceptedTime = CurrentSample.Time;
	}
}

void FYcFootstepNotifyGenerationService::FilterCandidates(const UYcFootstepNotifyGeneratorSettings* Settings,
	TArray<FYcFootstepNotifyCandidate>& InOutCandidates)
{
	InOutCandidates.Sort([](const FYcFootstepNotifyCandidate& A, const FYcFootstepNotifyCandidate& B)
	{
		return A.Time < B.Time;
	});

	if (InOutCandidates.Num() < 2)
	{
		return;
	}

	TArray<FYcFootstepNotifyCandidate> FilteredCandidates;
	FilteredCandidates.Reserve(InOutCandidates.Num());
	const float MinIntervalBetweenAnyFootsteps = FMath::Max(Settings->MinIntervalBetweenSameFootsteps * 0.75f, 0.22f);

	for (const FYcFootstepNotifyCandidate& Candidate : InOutCandidates)
	{
		if (FilteredCandidates.IsEmpty())
		{
			FilteredCandidates.Add(Candidate);
			continue;
		}

		FYcFootstepNotifyCandidate& LastCandidate = FilteredCandidates.Last();
		const bool bTooCloseToPreviousAnyFoot = (Candidate.Time - LastCandidate.Time) < MinIntervalBetweenAnyFootsteps;
		const bool bSameFoot = LastCandidate.SocketName == Candidate.SocketName;
		const bool bCloseInTime = (Candidate.Time - LastCandidate.Time) < Settings->MinIntervalBetweenSameFootsteps;

		if (bTooCloseToPreviousAnyFoot)
		{
			if (Candidate.Height < LastCandidate.Height)
			{
				LastCandidate = Candidate;
			}
			continue;
		}

		if (!Settings->bPreferAlternatingFeet)
		{
			FilteredCandidates.Add(Candidate);
			continue;
		}

		if (bSameFoot && bCloseInTime)
		{
			if (Candidate.Height < LastCandidate.Height)
			{
				LastCandidate = Candidate;
			}
			continue;
		}

		FilteredCandidates.Add(Candidate);
	}

	InOutCandidates = MoveTemp(FilteredCandidates);
}

void FYcFootstepNotifyGenerationService::ApplyNotifies(UAnimSequence* AnimSequence, const UYcFootstepNotifyGeneratorSettings* Settings,
	const TArray<FYcFootstepNotifyCandidate>& Candidates, int32& OutGeneratedNotifyCount)
{
	OutGeneratedNotifyCount = 0;

	AnimSequence->Modify();

	if (!UAnimationBlueprintLibrary::IsValidAnimNotifyTrackName(AnimSequence, Settings->NotifyTrackName))
	{
		UAnimationBlueprintLibrary::AddAnimationNotifyTrack(AnimSequence, Settings->NotifyTrackName, FLinearColor(0.12f, 0.62f, 0.95f));
	}

	if (Settings->bClearExistingTrack)
	{
		UAnimationBlueprintLibrary::RemoveAnimationNotifyEventsByTrack(AnimSequence, Settings->NotifyTrackName);
	}

	FYcContextEffectAnimNotifyVFXSettings VfxSettings;
	VfxSettings.Scale = Settings->VfxScale;

	FYcContextEffectAnimNotifyAudioSettings AudioSettings;
	AudioSettings.VolumeMultiplier = Settings->VolumeMultiplier;
	AudioSettings.PitchMultiplier = Settings->PitchMultiplier;

	FYcContextEffectAnimNotifyTraceSettings TraceSettings;
	TraceSettings.TraceChannel = Settings->TraceChannel;
	TraceSettings.EndTraceLocationOffset = FVector(0.0f, 0.0f, -Settings->TraceDistance);
	TraceSettings.bIgnoreActor = Settings->bIgnoreOwner;
	TraceSettings.bDrawDebugTrace = Settings->bDrawDebugTrace;
	TraceSettings.DebugDrawTime = Settings->DebugDrawTime;

	for (const FYcFootstepNotifyCandidate& Candidate : Candidates)
	{
		if (UAnimNotify* CreatedNotify = UAnimationBlueprintLibrary::AddAnimationNotifyEvent(
			AnimSequence,
			Settings->NotifyTrackName,
			Candidate.Time,
			UAnimNotify_YcContextEffects::StaticClass()))
		{
			if (UAnimNotify_YcContextEffects* ContextEffectsNotify = Cast<UAnimNotify_YcContextEffects>(CreatedNotify))
			{
				ContextEffectsNotify->SetParameters(
					Settings->FootstepEffectTag,
					Settings->NotifyLocationOffset,
					Settings->NotifyRotationOffset,
					VfxSettings,
					AudioSettings,
					true,
					Candidate.SocketName,
					Settings->bEnableTrace,
					TraceSettings);
				++OutGeneratedNotifyCount;
			}
		}
	}

	AnimSequence->SortNotifies();
	AnimSequence->MarkPackageDirty();
	AnimSequence->PostEditChange();
}

#undef LOCTEXT_NAMESPACE

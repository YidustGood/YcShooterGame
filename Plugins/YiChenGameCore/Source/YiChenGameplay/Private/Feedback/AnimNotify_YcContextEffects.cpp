// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Feedback/AnimNotify_YcContextEffects.h"

#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Feedback/ContextEffects/YcContextEffectsInterface.h"
#include "Feedback/ContextEffects/YcContextEffectsLibrary.h"
#include "Feedback/ContextEffects/YcContextEffectsSubsystem.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_YcContextEffects)

class UYcContextEffectsSettings;

UAnimNotify_YcContextEffects::UAnimNotify_YcContextEffects()
{
}

void UAnimNotify_YcContextEffects::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR
void UAnimNotify_YcContextEffects::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FString UAnimNotify_YcContextEffects::GetNotifyName_Implementation() const
{
	// 如果效果标签有效，则直接使用标签字符串作为通知名称。
	if (Effect.IsValid())
	{
		return Effect.ToString();
	}

	return Super::GetNotifyName_Implementation();
}

void UAnimNotify_YcContextEffects::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (MeshComp == nullptr) return;
	AActor* OwningActor = MeshComp->GetOwner();
	if (OwningActor == nullptr) return;

	// 准备射线检测相关数据。
	bool bHitSuccess = false;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;

	if (TraceProperties.bIgnoreActor)
	{
		QueryParams.AddIgnoredActor(OwningActor);
	}
	
	QueryParams.bReturnPhysicalMaterial = true;
	
	if (bPerformTrace)
	{
		// 如果启用了射线检测，则根据是否附着来确定起点位置。
		FVector TraceStart = bAttached ? MeshComp->GetSocketLocation(SocketName) : MeshComp->GetComponentLocation();
		const FVector TraceEnd = TraceStart + TraceProperties.EndTraceLocationOffset;

		// 确保世界对象有效。
		if (UWorld* World = OwningActor->GetWorld())
		{
			// 按当前配置执行单次射线检测。
			bHitSuccess = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd,
				TraceProperties.TraceChannel, QueryParams, FCollisionResponseParams::DefaultResponseParam);

			// 根据配置绘制射线检测调试图形，便于观察起点、终点和命中位置。
			if (TraceProperties.bDrawDebugTrace)
			{
				const FColor TraceColor = bHitSuccess ? FColor::Green : FColor::Red;
				const FVector DebugEnd = bHitSuccess ? HitResult.ImpactPoint : TraceEnd;
				DrawDebugLine(World, TraceStart, DebugEnd, TraceColor, false, TraceProperties.DebugDrawTime, 0, 1.5f);

				if (bHitSuccess)
				{
					DrawDebugPoint(World, HitResult.ImpactPoint, 12.0f, FColor::Yellow, false, TraceProperties.DebugDrawTime);
				}
			}
		}
	}

	// 预先准备上下文容器。
	FGameplayTagContainer Contexts;

	// 收集所有实现了上下文反馈接口的对象。
	TArray<UObject*> YcContextEffectImplementingObjects;

	// 先检查拥有者 Actor 自身是否实现了该接口。
	if (OwningActor->Implements<UYcContextEffectsInterface>())
	{
		// 如果实现了接口，则加入待处理列表。
		YcContextEffectImplementingObjects.Add(OwningActor);
	}

	// 遍历拥有者 Actor 的组件，查找实现了该接口的组件。
	for (const auto Component : OwningActor->GetComponents())
	{
		if (Component == nullptr) continue;
		// 如果组件实现了该接口，则加入待处理列表。
		if (Component->Implements<UYcContextEffectsInterface>())
		{
			YcContextEffectImplementingObjects.Add(Component);
		}
	}

	// 逐个向实现接口的对象派发反馈请求。
	for (UObject* YcContextEffectImplementingObject : YcContextEffectImplementingObjects)
	{
		if (YcContextEffectImplementingObject)
		{
			// 如果对象仍然有效，则把本次反馈请求连同相关参数一起派发给它。
			IYcContextEffectsInterface::Execute_AnimMotionEffect(YcContextEffectImplementingObject,
				(bAttached ? SocketName : FName("None")),
				Effect, MeshComp, LocationOffset, RotationOffset,
				Animation, bHitSuccess, HitResult, Contexts, VFXProperties.Scale,
				AudioProperties.VolumeMultiplier, AudioProperties.PitchMultiplier);
		}
	}

#if WITH_EDITORONLY_DATA
	// 动画编辑器预览路径，手动展开接口与子系统逻辑，便于在预览世界直接播放效果。
	if (!bPreviewInEditor) return;
	UWorld* World = OwningActor->GetWorld();

	// 获取世界对象，并确认当前是动画编辑器预览世界。
	if (World && World->WorldType == EWorldType::EditorPreview)
	{
		// 追加预览专用的上下文标签。
		Contexts.AppendTags(PreviewProperties.PreviewContexts);

		// 把预览使用的物理表面类型转换成上下文标签并加入本次预览。
		if (PreviewProperties.bPreviewPhysicalSurfaceAsContext)
		{
			TEnumAsByte<EPhysicalSurface> PhysicalSurfaceType = PreviewProperties.PreviewPhysicalSurface;

			if (const UYcContextEffectsSettings* YcContextEffectsSettings = GetDefault<UYcContextEffectsSettings>())
			{
				if (const FGameplayTag* SurfaceContextPtr = YcContextEffectsSettings->SurfaceTypeToContextMap.Find(PhysicalSurfaceType))
				{
					FGameplayTag SurfaceContext = *SurfaceContextPtr;

					Contexts.AddTag(SurfaceContext);
				}
			}
		}

		// 资源库是软引用，这里直接尝试加载。
		// TODO: 支持异步资源加载。
		if (UObject* EffectsLibrariesObj = PreviewProperties.PreviewContextEffectsLibrary.TryLoad())
		{
			// 确认加载出的对象确实是上下文反馈资源库。
			if (UYcContextEffectsLibrary* EffectLibrary = Cast<UYcContextEffectsLibrary>(EffectsLibrariesObj))
			{
				// 准备累计的音效与 Niagara 特效数组。
				TArray<USoundBase*> TotalSounds;
				TArray<UNiagaraSystem*> TotalNiagaraSystems;

				// 尝试加载资源库内容，加载结果会缓存到资源库对象的运行时数据中。
				EffectLibrary->LoadEffects();

				// 只有资源库成功加载后，才从中提取匹配的反馈资源。
				if (EffectLibrary && EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Loaded)
				{
					// 准备本次资源库查询使用的临时数组。
					TArray<USoundBase*> Sounds;
					TArray<UNiagaraSystem*> NiagaraSystems;

					// 从资源库中获取匹配的反馈资源。
					EffectLibrary->GetEffects(Effect, Contexts, Sounds, NiagaraSystems);

					// 追加到总结果数组中。
					TotalSounds.Append(Sounds);
					TotalNiagaraSystems.Append(NiagaraSystems);
				}

				// 遍历所有音效并按附着方式播放。
				for (USoundBase* Sound : TotalSounds)
				{
					UGameplayStatics::SpawnSoundAttached(Sound, MeshComp, (bAttached ? SocketName : FName("None")), LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset,
						false, AudioProperties.VolumeMultiplier, AudioProperties.PitchMultiplier, 0.0f, nullptr, nullptr, true);
				}

				// 遍历所有 Niagara 特效并按附着方式生成。
				for (UNiagaraSystem* NiagaraSystem : TotalNiagaraSystems)
				{
					UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, MeshComp, (bAttached ? SocketName : FName("None")), LocationOffset,
						RotationOffset, VFXProperties.Scale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, true, true);
				}
			}
		}
	}
#endif
}

#if WITH_EDITOR
void UAnimNotify_YcContextEffects::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();
}

void UAnimNotify_YcContextEffects::SetParameters(FGameplayTag EffectIn, FVector LocationOffsetIn, FRotator RotationOffsetIn,
	FYcContextEffectAnimNotifyVFXSettings VFXPropertiesIn, FYcContextEffectAnimNotifyAudioSettings AudioPropertiesIn,
	bool bAttachedIn, FName SocketNameIn, bool bPerformTraceIn, FYcContextEffectAnimNotifyTraceSettings TracePropertiesIn)
{
	Effect = EffectIn;
	LocationOffset = LocationOffsetIn;
	RotationOffset = RotationOffsetIn;
	VFXProperties.Scale = VFXPropertiesIn.Scale;
	AudioProperties.PitchMultiplier = AudioPropertiesIn.PitchMultiplier;
	AudioProperties.VolumeMultiplier = AudioPropertiesIn.VolumeMultiplier;
	bAttached = bAttachedIn;
	SocketName = SocketNameIn;
	bPerformTrace = bPerformTraceIn;
	TraceProperties.EndTraceLocationOffset = TracePropertiesIn.EndTraceLocationOffset;
	TraceProperties.TraceChannel = TracePropertiesIn.TraceChannel;
	TraceProperties.bIgnoreActor = TracePropertiesIn.bIgnoreActor;
	TraceProperties.bDrawDebugTrace = TracePropertiesIn.bDrawDebugTrace;
	TraceProperties.DebugDrawTime = TracePropertiesIn.DebugDrawTime;
}
#endif

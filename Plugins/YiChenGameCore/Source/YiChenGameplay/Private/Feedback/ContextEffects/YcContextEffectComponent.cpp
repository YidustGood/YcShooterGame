// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Feedback/ContextEffects/YcContextEffectComponent.h"

#include "Feedback/ContextEffects/YcContextEffectsSubsystem.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(YcContextEffectComponent)

UYcContextEffectComponent::UYcContextEffectComponent()
{
	// 该组件只在收到反馈请求时工作，不需要 Tick。
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UYcContextEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentContexts.AppendTags(DefaultEffectContexts);
	CurrentContextEffectsLibraries = DefaultContextEffectsLibraries;

	// BeginPlay 时加载并注册当前组件持有的反馈资源库。
	if (const UWorld* World = GetWorld())
	{
		if (UYcContextEffectsSubsystem* YcContextEffectsSubsystem = World->GetSubsystem<UYcContextEffectsSubsystem>())
		{
			YcContextEffectsSubsystem->LoadAndAddContextEffectsLibraries(GetOwner(), CurrentContextEffectsLibraries);
		}
	}
	
}

void UYcContextEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// EndPlay 时移除当前组件注册的反馈资源库引用。
	if (const UWorld* World = GetWorld())
	{
		if (UYcContextEffectsSubsystem* YcContextEffectsSubsystem = World->GetSubsystem<UYcContextEffectsSubsystem>())
		{
			YcContextEffectsSubsystem->UnloadAndRemoveContextEffectsLibraries(GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UYcContextEffectComponent::AnimMotionEffect_Implementation(const FName Bone, const FGameplayTag MotionEffect,
	USceneComponent* StaticMeshComponent, const FVector LocationOffset, const FRotator RotationOffset,
	const UAnimSequenceBase* AnimationSequence, const bool bHitSuccess, const FHitResult HitResult,
	FGameplayTagContainer Contexts, FVector VFXScale, float AudioVolume, float AudioPitch)
{
	// 准备本次需要维护的音频与 Niagara 组件列表。
	TArray<UAudioComponent*> AudioComponentsToAdd;
	TArray<UNiagaraComponent*> NiagaraComponentsToAdd;

	FGameplayTagContainer TotalContexts;

	// 合并外部传入上下文与组件自身维护的上下文。
	TotalContexts.AppendTags(Contexts);
	TotalContexts.AppendTags(CurrentContexts);

	// 根据需要把命中的物理表面类型转换成上下文标签。
	if (bConvertPhysicalSurfaceToContext)
	{
		TWeakObjectPtr<UPhysicalMaterial> PhysicalSurfaceTypePtr = HitResult.PhysMaterial;

		// 确认物理材质指针有效。
		if (PhysicalSurfaceTypePtr.IsValid())
		{
			TEnumAsByte<EPhysicalSurface> PhysicalSurfaceType = PhysicalSurfaceTypePtr->SurfaceType;

			// 确认配置对象有效。
			if (const UYcContextEffectsSettings* YcContextEffectsSettings = GetDefault<UYcContextEffectsSettings>())
			{
				// 将物理表面类型映射为已配置的上下文标签。
				if (const FGameplayTag* SurfaceContextPtr = YcContextEffectsSettings->SurfaceTypeToContextMap.Find(PhysicalSurfaceType))
				{
					FGameplayTag SurfaceContext = *SurfaceContextPtr;

					TotalContexts.AddTag(SurfaceContext);
				}
			}
		}
	}

	// 缓存仍然有效的音频组件。
	for (UAudioComponent* ActiveAudioComponent : ActiveAudioComponents)
	{
		if (ActiveAudioComponent)
		{
			AudioComponentsToAdd.Add(ActiveAudioComponent);
		}
	}

	// 缓存仍然有效的 Niagara 组件。
	for (UNiagaraComponent* ActiveNiagaraComponent : ActiveNiagaraComponents)
	{
		if (ActiveNiagaraComponent)
		{
			NiagaraComponentsToAdd.Add(ActiveNiagaraComponent);
		}
	}

	// 获取世界对象。
	if (const UWorld* World = GetWorld())
	{
		// 获取上下文反馈子系统。
		if (UYcContextEffectsSubsystem* YcContextEffectsSubsystem = World->GetSubsystem<UYcContextEffectsSubsystem>())
		{
			// 准备接收子系统生成结果的临时数组。
			TArray<UAudioComponent*> AudioComponents;
			TArray<UNiagaraComponent*> NiagaraComponents;

			// 通过子系统生成本次反馈效果。
			YcContextEffectsSubsystem->SpawnContextEffects(GetOwner(), StaticMeshComponent, Bone, 
				LocationOffset, RotationOffset, MotionEffect, TotalContexts,
				AudioComponents, NiagaraComponents, VFXScale, AudioVolume, AudioPitch);

			// 追加本次新生成的组件。
			AudioComponentsToAdd.Append(AudioComponents);
			NiagaraComponentsToAdd.Append(NiagaraComponents);
		}
	}

	// 更新当前激活的音频组件列表。
	ActiveAudioComponents.Empty();
	ActiveAudioComponents.Append(AudioComponentsToAdd);

	// 更新当前激活的 Niagara 组件列表。
	ActiveNiagaraComponents.Empty();
	ActiveNiagaraComponents.Append(NiagaraComponentsToAdd);
}

void UYcContextEffectComponent::UpdateEffectContexts(FGameplayTagContainer NewEffectContexts)
{
	// 重置并更新当前上下文标签。
	CurrentContexts.Reset(NewEffectContexts.Num());
	CurrentContexts.AppendTags(NewEffectContexts);
}

void UYcContextEffectComponent::UpdateLibraries(
	TSet<TSoftObjectPtr<UYcContextEffectsLibrary>> NewContextEffectsLibraries)
{
	// 替换当前使用的反馈资源库集合。
	CurrentContextEffectsLibraries = NewContextEffectsLibraries;

	// 获取世界对象。
	if (const UWorld* World = GetWorld())
	{
		// 获取上下文反馈子系统。
		if (UYcContextEffectsSubsystem* YcContextEffectsSubsystem = World->GetSubsystem<UYcContextEffectsSubsystem>())
		{
			// 重新把资源库加载并注册到子系统中。                  
			YcContextEffectsSubsystem->LoadAndAddContextEffectsLibraries(GetOwner(), CurrentContextEffectsLibraries);
		}
	}
}


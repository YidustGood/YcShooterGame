// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Feedback/ContextEffects/YcContextEffectsSubsystem.h"

#include "GameplayTagContainer.h"
#include "NiagaraFunctionLibrary.h"
#include "Feedback/ContextEffects/YcContextEffectsLibrary.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcContextEffectsSubsystem)

class AActor;
class UAudioComponent;
class UNiagaraSystem;
class USceneComponent;
class USoundBase;

void UYcContextEffectsSubsystem::SpawnContextEffects(const AActor* SpawningActor, USceneComponent* AttachToComponent,
	const FName AttachPoint, const FVector LocationOffset, const FRotator RotationOffset, FGameplayTag Effect,
	FGameplayTagContainer Contexts, TArray<UAudioComponent*>& AudioOut, TArray<UNiagaraComponent*>& NiagaraOut,
	FVector VFXScale, float AudioVolume, float AudioPitch)
{
	TObjectPtr<UYcContextEffectsSet>* EffectsLibrariesSetPtr = ActiveActorEffectsMap.Find(SpawningActor);
	if (!EffectsLibrariesSetPtr) return;
	UYcContextEffectsSet* EffectsLibraries = *EffectsLibrariesSetPtr;
	if (!EffectsLibraries) return;

	// 准备累计的音效与 Niagara 特效数组。
	TArray<USoundBase*> TotalSounds;
	TArray<UNiagaraSystem*> TotalNiagaraSystems;

	// 遍历当前 Actor 注册的所有反馈资源库。
	for (UYcContextEffectsLibrary* EffectLibrary : EffectsLibraries->ContextEffectsLibraries)
	{
		// 资源库有效且已加载完成时，尝试查询匹配的反馈资源。
		if (EffectLibrary && EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Loaded)
		{
			// 准备单个资源库查询使用的临时数组。
			TArray<USoundBase*> Sounds;
			TArray<UNiagaraSystem*> NiagaraSystems;

			// 从当前资源库中提取匹配到的音效和 Niagara 特效。
			EffectLibrary->GetEffects(Effect, Contexts, Sounds, NiagaraSystems);

			// 追加到总结果数组中。
			TotalSounds.Append(Sounds);
			TotalNiagaraSystems.Append(NiagaraSystems);
		}
		else if (EffectLibrary && EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Unloaded)
		{
			// 如果资源库尚未加载，则先触发加载。
			EffectLibrary->LoadEffects();
		}
	}

	// 遍历并生成所有匹配到的音效。
	for (USoundBase* Sound : TotalSounds)
	{
		// 以附着方式播放音效，并把生成的音频组件返回给调用方。
		UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(Sound, AttachToComponent, AttachPoint, LocationOffset, RotationOffset, EAttachLocation::KeepRelativeOffset,
			false, AudioVolume, AudioPitch, 0.0f, nullptr, nullptr, true);

		AudioOut.Add(AudioComponent);
	}

	// 遍历并生成所有匹配到的 Niagara 特效。
	for (UNiagaraSystem* NiagaraSystem : TotalNiagaraSystems)
	{
		// 以附着方式生成 Niagara 特效，并把生成的组件返回给调用方。
		UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, AttachToComponent, AttachPoint, LocationOffset,
			RotationOffset, VFXScale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, true, true);

		NiagaraOut.Add(NiagaraComponent);
	}
}

bool UYcContextEffectsSubsystem::GetContextFromSurfaceType(TEnumAsByte<EPhysicalSurface> PhysicalSurface,
	FGameplayTag& Context)
{
	// 读取项目中的上下文反馈配置。
	if (const UYcContextEffectsSettings* ProjectSettings = GetDefault<UYcContextEffectsSettings>())
	{
		// 查找该物理表面类型映射到的上下文标签。
		if (const FGameplayTag* GameplayTagPtr = ProjectSettings->SurfaceTypeToContextMap.Find(PhysicalSurface))
		{
			Context = *GameplayTagPtr;
		}
	}

	// 只有找到有效上下文标签时才返回 true。
	return Context.IsValid();
}

void UYcContextEffectsSubsystem::LoadAndAddContextEffectsLibraries(AActor* OwningActor,
	TSet<TSoftObjectPtr<UYcContextEffectsLibrary>> ContextEffectsLibraries)
{
	// 如果拥有者 Actor 无效，或者资源库集合为空，则直接返回。
	if (OwningActor == nullptr || ContextEffectsLibraries.Num() <= 0)
	{
		return;
	}

	// 为该 Actor 创建新的反馈资源集合对象。
	UYcContextEffectsSet* EffectsLibrariesSet = NewObject<UYcContextEffectsSet>(this);

	// 遍历所有软引用资源库。
	for (const TSoftObjectPtr<UYcContextEffectsLibrary>& ContextEffectSoftObj : ContextEffectsLibraries)
	{
		// 通过软引用同步加载资源库资产。
		// TODO: 支持异步加载资源数据。
		if (UYcContextEffectsLibrary* EffectsLibrary = ContextEffectSoftObj.LoadSynchronous())
		{
			// 对有效资源库执行内容加载。
			EffectsLibrary->LoadEffects();

			// 把资源库加入当前 Actor 的集合中。
			EffectsLibrariesSet->ContextEffectsLibraries.Add(EffectsLibrary);
		}
	}

	// 更新 Actor 到反馈资源集合的映射。
	ActiveActorEffectsMap.Emplace(OwningActor, EffectsLibrariesSet);
}

void UYcContextEffectsSubsystem::UnloadAndRemoveContextEffectsLibraries(AActor* OwningActor)
{
	// 如果拥有者 Actor 无效，则直接返回。
	if (OwningActor == nullptr)
	{
		return;
	}

	// 从运行时映射中移除该 Actor 的反馈资源集合引用。
	ActiveActorEffectsMap.Remove(OwningActor);
}

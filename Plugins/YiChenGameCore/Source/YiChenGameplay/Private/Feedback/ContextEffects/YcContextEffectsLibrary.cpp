// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Feedback/ContextEffects/YcContextEffectsLibrary.h"

#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcContextEffectsLibrary)

void UYcContextEffectsLibrary::GetEffects(const FGameplayTag Effect, const FGameplayTagContainer Context,
	TArray<USoundBase*>& Sounds, TArray<UNiagaraSystem*>& NiagaraSystems)
{
	// 只有当效果标签有效且资源库已完成加载时才允许查询。
	if (Effect.IsValid() && Context.IsValid() && EffectsLoadState == EContextEffectsLibraryLoadState::Loaded)
	{
		// 遍历所有已激活的上下文反馈条目。
		for (const auto& ActiveContextEffect : ActiveContextEffects)
		{
			// 要求效果标签精确匹配，且当前上下文完整包含配置上下文，同时两边的空上下文状态保持一致。
			if (Effect.MatchesTagExact(ActiveContextEffect->EffectTag)
				&& Context.HasAllExact(ActiveContextEffect->Context)
				&& (ActiveContextEffect->Context.IsEmpty() == Context.IsEmpty()))
			{
				// 收集所有匹配到的音效与 Niagara 特效。
				Sounds.Append(ActiveContextEffect->Sounds);
				NiagaraSystems.Append(ActiveContextEffect->NiagaraSystems);
			}
		}
	}
}

void UYcContextEffectsLibrary::LoadEffects()
{
	// 如果当前不在加载中，则开始把资源同步加载进资源库。
	if (EffectsLoadState != EContextEffectsLibraryLoadState::Loading)
	{
		// 标记为加载中。
		EffectsLoadState = EContextEffectsLibraryLoadState::Loading;

		// 清空旧的运行时缓存。
		ActiveContextEffects.Empty();

		// 调用内部加载逻辑。
		LoadEffectsInternal();
	}
}

EContextEffectsLibraryLoadState UYcContextEffectsLibrary::GetContextEffectsLibraryLoadState()
{
	// 返回当前加载状态。
	return EffectsLoadState;	
}

void UYcContextEffectsLibrary::LoadEffectsInternal()
{
	// TODO: 为资源库加载补充异步流程。

	// 复制一份配置数据，便于后续扩展为异步加载流程。
	TArray<FYcContextEffects> LocalContextEffects = ContextEffects;

	// 准备运行时激活条目数组。
	TArray<UYcActiveContextEffects*> ActiveContextEffectsArray;

	// 遍历配置中的每一条上下文反馈。
	for (const FYcContextEffects& ContextEffect : LocalContextEffects)
	{
		// 确保效果标签和上下文配置有效。
		if (ContextEffect.EffectTag.IsValid() && ContextEffect.Context.IsValid())
		{
			// 创建新的运行时激活条目。
			UYcActiveContextEffects* NewActiveContextEffects = NewObject<UYcActiveContextEffects>(this);

			// 拷贝本条配置的标签数据。
			NewActiveContextEffects->EffectTag = ContextEffect.EffectTag;
			NewActiveContextEffects->Context = ContextEffect.Context;

			// 尝试加载并归类本条配置中的反馈资源。
			for (const FSoftObjectPath& Effect : ContextEffect.Effects)
			{
				if (UObject* Object = Effect.TryLoad())
				{
					if (Object->IsA(USoundBase::StaticClass()))
					{
						if (USoundBase* SoundBase = Cast<USoundBase>(Object))
						{
							NewActiveContextEffects->Sounds.Add(SoundBase);
						}
					}
					else if (Object->IsA(UNiagaraSystem::StaticClass()))
					{
						if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(Object))
						{
							NewActiveContextEffects->NiagaraSystems.Add(NiagaraSystem);
						}
					}
				}
			}

			// 将新的运行时条目加入结果数组。
			ActiveContextEffectsArray.Add(NewActiveContextEffects);
		}
	}

	// TODO: 接入异步加载后，在完成回调里统一触发完成逻辑。
	// 当前同步加载完成后，直接进入完成阶段。
	this->YcContextEffectLibraryLoadingComplete(ActiveContextEffectsArray);
}

void UYcContextEffectsLibrary::YcContextEffectLibraryLoadingComplete(
	TArray<UYcActiveContextEffects*> YcActiveContextEffects)
{
	// 标记资源库已完成加载。
	EffectsLoadState = EContextEffectsLibraryLoadState::Loaded;

	// 把新生成的运行时条目追加到当前激活列表中。
	ActiveContextEffects.Append(YcActiveContextEffects);
}

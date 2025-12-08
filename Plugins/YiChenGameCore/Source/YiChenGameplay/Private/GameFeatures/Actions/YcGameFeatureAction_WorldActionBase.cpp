// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/Actions/YcGameFeatureAction_WorldActionBase.h"

#include "GameFeaturesSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_WorldActionBase)

void UYcGameFeatureAction_WorldActionBase::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	// 绑定GameInstance启动事件，以便为后续创建的World添加Action功能
	GameInstanceStartHandles.FindOrAdd(Context) = FWorldDelegates::OnStartGameInstance.AddUObject(this, 
		&UYcGameFeatureAction_WorldActionBase::HandleGameInstanceStart, FGameFeatureStateChangeContext(Context));

	// 为所有已初始化的World添加Action功能
	// 在PIE模式下GEngine可能包含多个World实例（例如多个PIE实例和编辑器World）
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		// 检查该World是否与当前GameFeatureAction匹配
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddToWorld(WorldContext, Context);
		}
	}
}

void UYcGameFeatureAction_WorldActionBase::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	// 查找并移除GameInstance启动事件的委托绑定
	const FDelegateHandle* FoundHandle = GameInstanceStartHandles.Find(Context);
	if (ensure(FoundHandle))
	{
		FWorldDelegates::OnStartGameInstance.Remove(*FoundHandle);
	}
}

void UYcGameFeatureAction_WorldActionBase::HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext)
{
	// 获取GameInstance关联的World上下文
	if (FWorldContext* WorldContext = GameInstance->GetWorldContext())
	{
		// 检查该World是否与当前GameFeatureAction匹配
		if (ChangeContext.ShouldApplyToWorldContext(*WorldContext))
		{
			// 为新创建的World添加Action功能
			AddToWorld(*WorldContext, ChangeContext);
		}
	}
}
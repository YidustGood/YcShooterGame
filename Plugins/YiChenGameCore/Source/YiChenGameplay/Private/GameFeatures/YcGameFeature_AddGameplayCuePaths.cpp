// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/YcGameFeature_AddGameplayCuePaths.h"

#include "AbilitySystemGlobals.h"
#include "GameFeatureData.h"
#include "GameFeaturesSubsystem.h"
#include "GameplayCueSet.h"
#include "YcGameplayCueManager.h"
#include "YiChenAbility.h"
#include "GameFeatures/Actions/YcGameFeatureAction_AddGameplayCuePath.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeature_AddGameplayCuePaths)

class FName;
struct FPrimaryAssetId;

void UYcGameFeature_AddGameplayCuePaths::OnGameFeatureRegistering(const UGameFeatureData* GameFeatureData,
	const FString& PluginName, const FString& PluginURL)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ULyraGameFeature_AddGameplayCuePaths::OnGameFeatureRegistering);
	
	// 构建插件根路径，例如："/YcShooterExample"
	const FString PluginRootPath = TEXT("/") + PluginName;
	
	// 遍历 GameFeatureData 中的所有 Action
	for (const UGameFeatureAction* Action : GameFeatureData->GetActions())
	{
		// 查找 YcGameFeatureAction_AddGameplayCuePath 类型的 Action
		const UYcGameFeatureAction_AddGameplayCuePath* AddGameplayCueGFA = Cast<UYcGameFeatureAction_AddGameplayCuePath>(Action);
		if (AddGameplayCueGFA == nullptr) continue;
		
		UYcGameplayCueManager* GCM = UYcGameplayCueManager::Get();
		if (GCM == nullptr)
		{
			UE_LOG(LogYcAbilitySystem, Error, TEXT("UYcGameFeature_AddGameplayCuePaths::OnGameFeatureRegistering: 获取UYcGameplayCueManager失败, 请确保在项目设置中设置了使用UYcGameplayCueManager."));
			return;
		}
		
		// 获取GFA配置的GameplayCue扫描路径
		const TArray<FDirectoryPath>& DirsToAdd = AddGameplayCueGFA->GetDirectoryPathsToAdd();
			
		// 记录初始化前的 Cue 数量，用于判断是否需要刷新资产捆绑
		UGameplayCueSet* RuntimeGameplayCueSet = GCM->GetRuntimeCueSet();
		const int32 PreInitializeNumCues = RuntimeGameplayCueSet ? RuntimeGameplayCueSet->GameplayCueData.Num() : 0;

		// 遍历所有需要添加的目录
		for (const FDirectoryPath& Directory : DirsToAdd)
		{
			// 将相对路径转换为完整路径
			// 例如："/GameplayCues" -> "/YcShooterExample/GameplayCues"
			FString MutablePath = Directory.Path;
			UGameFeaturesSubsystem::FixPluginPackagePath(MutablePath, PluginRootPath, false);
					
			// 添加路径到 GameplayCueManager，但不立即扫描（bShouldRescanCueAssets = false）
			// 等所有路径添加完成后统一扫描，提高性能
			GCM->AddGameplayCueNotifyPath(MutablePath, /** bShouldRescanCueAssets = */ false);	
		}
				
		// 所有路径添加完成后，重建运行时对象库，统一扫描所有新路径
		if (!DirsToAdd.IsEmpty())
		{
			GCM->InitializeRuntimeObjectLibrary();	
		}

		// 检查 Cue 数量是否发生变化
		const int32 PostInitializeNumCues = RuntimeGameplayCueSet ? RuntimeGameplayCueSet->GameplayCueData.Num() : 0;
		if (PreInitializeNumCues != PostInitializeNumCues)
		{
			// Cue 数量变化，需要刷新 AssetManager 的资产捆绑数据
			GCM->RefreshGameplayCuePrimaryAsset();
		}
	}
}

void UYcGameFeature_AddGameplayCuePaths::OnGameFeatureUnregistering(const UGameFeatureData* GameFeatureData,
                                                                     const FString& PluginName, const FString& PluginURL)
{
	// 构建插件根路径
	const FString PluginRootPath = TEXT("/") + PluginName;
	
	// 遍历 GameFeatureData 中的所有 Action
	for (const UGameFeatureAction* Action : GameFeatureData->GetActions())
	{
		// 查找 YcGameFeatureAction_AddGameplayCuePath 类型的 Action
		const UYcGameFeatureAction_AddGameplayCuePath* AddGameplayCueGFA = Cast<UYcGameFeatureAction_AddGameplayCuePath>(Action);
		if (AddGameplayCueGFA == nullptr) continue;
		
		UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager();
		
		const TArray<FDirectoryPath>& DirsToAdd = AddGameplayCueGFA->GetDirectoryPathsToAdd();
		
		if (GCM == nullptr) return; // 上面注册时做了体现日志输出这里就不做了
		
		int32 NumRemoved = 0;
				
		// 遍历所有需要移除的目录
		for (const FDirectoryPath& Directory : DirsToAdd)
		{
			// 将相对路径转换为完整路径
			FString MutablePath = Directory.Path;
			UGameFeaturesSubsystem::FixPluginPackagePath(MutablePath, PluginRootPath, false);
					
			// 从 GameplayCueManager 中移除路径，但不立即扫描
			NumRemoved += GCM->RemoveGameplayCueNotifyPath(MutablePath, /** bShouldRescanCueAssets = */ false);
		}

		// 确保移除的路径数量与配置的数量一致
		ensure(NumRemoved == DirsToAdd.Num());
				
		// 如果有路径被移除，重建运行时对象库
		if (NumRemoved > 0)
		{
			GCM->InitializeRuntimeObjectLibrary();	
		}		
	}
}

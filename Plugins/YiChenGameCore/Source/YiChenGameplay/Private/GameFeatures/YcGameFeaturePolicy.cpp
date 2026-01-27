// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/YcGameFeaturePolicy.h"

#include "GameFeatures/YcGameFeature_AddGameplayCuePaths.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeaturePolicy)

UYcGameFeaturePolicy::UYcGameFeaturePolicy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UYcGameFeaturePolicy& UYcGameFeaturePolicy::Get()
{
	return UGameFeaturesSubsystem::Get().GetPolicy<UYcGameFeaturePolicy>();
}

void UYcGameFeaturePolicy::InitGameFeatureManager()
{
	//@TODO HotfixManager
	// Observers.Add(NewObject<UYcGameFeature_HotfixManager>());
	Observers.Add(NewObject<UYcGameFeature_AddGameplayCuePaths>());

	UGameFeaturesSubsystem& Subsystem = UGameFeaturesSubsystem::Get();
	for (UObject* Observer : Observers)
	{
		Subsystem.AddObserver(Observer);
	}

	Super::InitGameFeatureManager();
}

void UYcGameFeaturePolicy::ShutdownGameFeatureManager()
{
	Super::ShutdownGameFeatureManager();

	UGameFeaturesSubsystem& Subsystem = UGameFeaturesSubsystem::Get();
	for (UObject* Observer : Observers)
	{
		Subsystem.RemoveObserver(Observer);
	}
	Observers.Empty();
}

bool UYcGameFeaturePolicy::IsPluginAllowed(const FString& PluginURL, FString* OutReason) const
{
	// @TODO 这里可用添加插件是否允许的筛选机制
	return Super::IsPluginAllowed(PluginURL, OutReason);
}
TArray<FPrimaryAssetId> UYcGameFeaturePolicy::GetPreloadAssetListForGameFeature(const UGameFeatureData* GameFeatureToLoad, bool bIncludeLoadedAssets) const
{
	return Super::GetPreloadAssetListForGameFeature(GameFeatureToLoad, bIncludeLoadedAssets);
}

const TArray<FName> UYcGameFeaturePolicy::GetPreloadBundleStateForGameFeature() const
{
	return Super::GetPreloadBundleStateForGameFeature();
}

void UYcGameFeaturePolicy::GetGameFeatureLoadingMode(bool& bLoadClientData, bool& bLoadServerData) const
{
	// Editor will load both, this can cause hitching as the bundles are set to not preload in editor
	bLoadClientData = !IsRunningDedicatedServer();
	bLoadServerData = !IsRunningClientOnly();
}

bool UYcGameFeaturePolicy::WillPluginBeCooked(const FString& PluginFilename,
	const FGameFeaturePluginDetails& PluginDetails) const
{
	return Super::WillPluginBeCooked(PluginFilename, PluginDetails);
}

bool UYcGameFeaturePolicy::ShouldReadPluginDetails(const FString& PluginDescriptorFilename) const
{
	return Super::ShouldReadPluginDetails(PluginDescriptorFilename);
}
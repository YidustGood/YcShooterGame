// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcExperienceManagerComponent.h"

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "GameFeaturesSubsystemSettings.h"
#include "YiChenGameplay.h"
#include "GameModes/YcExperienceActionSet.h"
#include "GameModes/YcExperienceDefinition.h"
#include "GameModes/YcExperienceManagerSubsystem.h"
#include "GameModes/YcWorldSettings.h"
#include "Net/UnrealNetwork.h"
#include "System/YcAssetManager.h"
#include "Utils/CommonSimpleUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcExperienceManagerComponent)

//@TODO: Async load the experience definition itself
//@TODO: Handle failures explicitly (go into a 'completed but failed' state rather than check()-ing)
//@TODO: Do the action phases at the appropriate times instead of all at once
//@TODO: Support deactivating an experience and do the unloading actions
//@TODO: Think about what deactivation/cleanup means for preloaded assets
//@TODO: Handle deactivating game features, right now we 'leak' them enabled
// (for a client moving from experience to experience we actually want to diff the requirements and only unload some, not unload everything for them to just be immediately reloaded)
//@TODO: Handle both built-in and URL-based plugins (search for colon?)

namespace YcConsoleVariables
{
	static float ExperienceLoadRandomDelayMin = 0.0f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayMin(
		TEXT("Yc.chaos.ExperienceDelayLoad.MinSecs"),
		ExperienceLoadRandomDelayMin,
		TEXT("This value (in seconds) will be added as a delay of load completion of the experience (along with the random value Yc.chaos.ExperienceDelayLoad.RandomSecs)"),
		ECVF_Default);

	static float ExperienceLoadRandomDelayRange = 0.0f;
	static FAutoConsoleVariableRef CVarExperienceLoadRandomDelayRange(
		TEXT("Yc.chaos.ExperienceDelayLoad.RandomSecs"),
		ExperienceLoadRandomDelayRange,
		TEXT(
			"A random amount of time between 0 and this value (in seconds) will be added as a delay of load completion of the experience (along with the fixed value Yc.chaos.ExperienceDelayLoad.MinSecs)"),
		ECVF_Default);

	float GetExperienceLoadDelayDuration()
	{
		return FMath::Max(0.0f, ExperienceLoadRandomDelayMin + FMath::FRand() * ExperienceLoadRandomDelayRange);
	}
}

UYcExperienceManagerComponent::UYcExperienceManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 只有服务器有权指定当前游戏的游戏体验定义类, 所以需要开启网络复制将CurrentExperience同步到客户端
	SetIsReplicatedByDefault(true);
}

void UYcExperienceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CurrentExperience);
}

void UYcExperienceManagerComponent::SetCurrentExperience(const FPrimaryAssetId& ExperienceId)
{
	const UYcAssetManager& AssetManager = UYcAssetManager::Get();
	const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(ExperienceId);
	const TSubclassOf<UYcExperienceDefinition> AssetClass = Cast<UClass>(AssetPath.TryLoad()); // 同步加载
	check(AssetClass);
	// 获取该类的CDO对象, YcExperienceDefinition作为静态的定义配置无需动态修改, 所以直接使用CDO即可
	const UYcExperienceDefinition* Experience = GetDefault<UYcExperienceDefinition>(AssetClass);

	check(Experience != nullptr);
	check(CurrentExperience == nullptr); // 防止重复设置Exp
	CurrentExperience = Experience;
	StartExperienceLoad(); // 开始加载Experience, 客户端的加载时机是在CurrentExperience被复制到客户端时, 也就是OnRep_CurrentExperience中
}

void UYcExperienceManagerComponent::OnRep_CurrentExperience()
{
	StartExperienceLoad(); // CurrentExperience复制到了客户端, 即刻开始加载Experience
}

void UYcExperienceManagerComponent::StartExperienceLoad()
{
	check(CurrentExperience != nullptr);
	check(LoadState == EYcExperienceLoadState::Unloaded); // 避免重复加载问题
	
	UE_LOG(LogYcGameplay, Log, TEXT("EXPERIENCE: StartExperienceLoad(CurrentExperience = %s, %s)"),
	   *CurrentExperience->GetPrimaryAssetId().ToString(),
	   *GetClientServerContextString(this));
	
	LoadState = EYcExperienceLoadState::Loading;
	
	UYcAssetManager& AssetManager = UYcAssetManager::Get();
	
	TSet<FPrimaryAssetId> BundleAssetList; // CurrentExperience里的资产会加入到这个清单里面打包起来加载
	TSet<FSoftObjectPath> RawAssetList;	// 目前还未实际用到
	
	BundleAssetList.Add(CurrentExperience->GetPrimaryAssetId());
	
	// 遍历获取CurrentExperience中的ActionSet的FPrimaryAssetId添加记录
	for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet == nullptr) continue;
		BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
	}
	
	// 遍历获取YcWorldSettings中的ActionSet PrimaryAssetId
	if(AYcWorldSettings* YcWorldSettings = AYcWorldSettings::GetYcWorldSettings(this))
	{
		for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : YcWorldSettings->ActionSets)
		{
			if (ActionSet == nullptr) continue;
			BundleAssetList.Add(ActionSet->GetPrimaryAssetId());
		}
	}
	
	// 加载与体验相关的资源
	
	TArray<FName> BundlesToLoad; // 要加载的资产分组标签
	BundlesToLoad.Add(FYcBundles::Equipped);
	
	//@TODO: Centralize this client/server stuff into the YcAssetManager
	// 根据网络模式添加需要加载的资产分组标签, 避免加载不需要的资源
	const ENetMode OwnerNetMode = GetOwner()->GetNetMode();
	const bool bLoadClient = GIsEditor || (OwnerNetMode != NM_DedicatedServer); // 只有专用服务器为false, 其余都为true
	const bool bLoadServer = GIsEditor || (OwnerNetMode != NM_Client);
	if (bLoadClient)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateClient);
	}
	if (bLoadServer)
	{
		BundlesToLoad.Add(UGameFeaturesSubsystemSettings::LoadStateServer);
	}
	
	TSharedPtr<FStreamableHandle> BundleLoadHandle = nullptr;
	if (BundleAssetList.Num() > 0)
	{
		// 加载BundleAssetList中每个资产的BundlesToLoad分组, 目前是加载 {"Equipped", "Client", "Server"} 这三个Bundle的资产
		BundleLoadHandle = AssetManager.ChangeBundleStateForPrimaryAssets(
				BundleAssetList.Array(),      // 要加载的资产列表
				BundlesToLoad,                // 要添加的Bundle
				{},                           // 要移除的Bundle（空）
				false,                        // 不移除所有现有Bundle
				FStreamableDelegate(),        // 回调（稍后设置）
				FStreamableManager::AsyncLoadHighPriority // 高优先级
			);
	}
	
	TSharedPtr<FStreamableHandle> RawLoadHandle = nullptr;
	if (RawAssetList.Num() > 0)
	{
		RawLoadHandle = AssetManager.LoadAssetList(RawAssetList.Array(), FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority, TEXT("StartExperienceLoad()"));
	}
	// 如果两个异步加载操作都在进行中，就将它们合并起来
	TSharedPtr<FStreamableHandle> Handle = nullptr;
	if (BundleLoadHandle.IsValid() && RawLoadHandle.IsValid())
	{
		Handle = AssetManager.GetStreamableManager().CreateCombinedHandle({BundleLoadHandle, RawLoadHandle});
	}
	else
	{
		Handle = BundleLoadHandle.IsValid() ? BundleLoadHandle : RawLoadHandle;
	}
	
	FStreamableDelegate OnAssetsLoadedDelegate = FStreamableDelegate::CreateUObject(this, &ThisClass::OnExperienceLoadComplete);
	if (!Handle.IsValid() || Handle->HasLoadCompleted())
	{
		// 资产已经加载完成，直接调用委托通知Experience加载完成
		FStreamableHandle::ExecuteDelegate(OnAssetsLoadedDelegate);
	}
	else
	{
		// 未加载完成就绑定委托, 加载完成后自动调用委托
		Handle->BindCompleteDelegate(OnAssetsLoadedDelegate);

		Handle->BindCancelDelegate(FStreamableDelegate::CreateLambda([OnAssetsLoadedDelegate]()
		{
			OnAssetsLoadedDelegate.ExecuteIfBound(); // 目前取消和完成都是回调OnExperienceLoadComplete()函数
		}));
	}
	
	// 这些资产会被预加载，但不会阻止Experience的启动
	const TSet<FPrimaryAssetId> PreloadAssetList;
	//@TODO: Determine assets to preload (but not blocking-ly)
	if (PreloadAssetList.Num() > 0)
	{
		AssetManager.ChangeBundleStateForPrimaryAssets(PreloadAssetList.Array(), BundlesToLoad, {});
	}
}

void UYcExperienceManagerComponent::OnExperienceLoadComplete()
{
	check(LoadState == EYcExperienceLoadState::Loading);
	check(CurrentExperience != nullptr);
	
	UE_LOG(LogYcGameplay, Log, TEXT("EXPERIENCE: OnExperienceLoadComplete(CurrentExperience = %s, %s)"),
	   *CurrentExperience->GetPrimaryAssetId().ToString(),
	   *GetClientServerContextString(this));
	
	// 查找GameFeaturePlugin的URL，过滤掉重复的和没有有效映射的
	GameFeaturePluginURLs.Reset();
	
	// 创建一个lambda函数对象, 用作收集GameFeaturePluginURLs
	auto CollectGameFeaturePluginURLs = [This=this](const UPrimaryDataAsset* Context, const TArray<FString>& FeaturePluginList)
	{
		for (const FString& PluginName : FeaturePluginList)
		{
			FString PluginURL;
			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(PluginName, /*out*/ PluginURL))
			{
				This->GameFeaturePluginURLs.AddUnique(PluginURL);
			}
			else
			{
				ensureMsgf(false, TEXT("OnExperienceLoadComplete failed to find plugin URL from PluginName %s for experience %s - fix data, ignoring for this run"), *PluginName,
						   *Context->GetPrimaryAssetId().ToString());
			}
		}
	};
	
	// 收集当前游戏体验中的配置的GameFeature PluginURLs, 通过配置的GameFeature Name来查找到对应的PluginURL进行添加
	CollectGameFeaturePluginURLs(CurrentExperience, CurrentExperience->GameFeaturesToEnable);
	// 遍历当前游戏体验中ActionSet里配置的GameFeature PluginURL
	for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet == nullptr) continue;
		CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
	}
	
	// 遍历YcWorldSettings中ActionSet里配置的GameFeature PluginURL
	if(AYcWorldSettings* YcWorldSettings = AYcWorldSettings::GetYcWorldSettings(this))
	{
		for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
		{
			if (ActionSet == nullptr) continue;
			CollectGameFeaturePluginURLs(ActionSet, ActionSet->GameFeaturesToEnable);
		}
	}
	
	// 加载并激活收集到的所有GameFeaturePlugin
	NumGameFeaturePluginsLoading = GameFeaturePluginURLs.Num();
	if (NumGameFeaturePluginsLoading > 0)
	{
		LoadState = EYcExperienceLoadState::LoadingGameFeatures; // 更新状态为加载GameFeaturePlugin中
		for (const FString& PluginURL : GameFeaturePluginURLs)
		{
			UE_LOG(LogYcGameplay, Log, TEXT("EXPERIENCE: LoadAndActivateGameFeaturePlugin(CurrentExperience = %s, %s; CurrentGFPluginURL = %s)"),
					   *CurrentExperience->GetPrimaryAssetId().ToString(),
					   *GetClientServerContextString(this), *PluginURL);
			UYcExperienceManagerSubsystem::NotifyOfPluginActivation(PluginURL); //向UYcExperienceManagerSubsystem发起激活通知, 添加计数追踪, 也可用于结束后的插件卸载
			// 调用GameFeature子系统加载激活GameFeaturePlugin
			UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete::CreateUObject(this, &ThisClass::OnGameFeaturePluginLoadComplete));
		}
	}
	else
	{
		OnExperienceFullLoadCompleted();
	}
}

void UYcExperienceManagerComponent::OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result)
{
	NumGameFeaturePluginsLoading--;
	// 当所有GameFeaturePlugins加载完成后调用 ExperienceFullLoadCompleted(), 进入下一阶段开始GameFeatureAction的激活
	if (NumGameFeaturePluginsLoading == 0)
	{
		UE_LOG(LogYcGameplay, Log, TEXT("EXPERIENCE: LoadAndActivateGameFeaturePlugin(CurrentExperience = %s, %s; NumGameFeaturePluginsLoaded = %d)"),
	   *CurrentExperience->GetPrimaryAssetId().ToString(),
	   *GetClientServerContextString(this), GameFeaturePluginURLs.Num());
		OnExperienceFullLoadCompleted();
	}
}

void UYcExperienceManagerComponent::OnExperienceFullLoadCompleted()
{
	check(LoadState != EYcExperienceLoadState::Loaded);
	
	// 插入一个随机延迟用于测试，不然可能少量资源的情况下会加载得很快观察不到这个过程（前提是配置了）
	if (LoadState != EYcExperienceLoadState::LoadingChaosTestingDelay)
	{
		const float DelaySecs = YcConsoleVariables::GetExperienceLoadDelayDuration();
		// 这里的逻辑是判断延迟时间是否大于0，如果大于0那么将状态更新，并设置延迟时间后再次调用OnExperienceFullLoadCompleted()这个函数，下次进来后因为状态被更新为了LoadingChaosTestingDelay，所以会跳过这里正常执行后续加载逻辑
		// 简而言之就是使用Timer配合状态判断实现了延迟加载
		if (DelaySecs > 0.0f)
		{
			FTimerHandle DummyHandle;

			LoadState = EYcExperienceLoadState::LoadingChaosTestingDelay;
			GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &ThisClass::OnExperienceFullLoadCompleted, DelaySecs, /*bLooping=*/ false);

			return;
		}
	}
	
	LoadState = EYcExperienceLoadState::ExecutingActions;
	
	// 执行GameFeatureAction
	FGameFeatureActivatingContext Context;
	
	// 仅在设置了特定的世界上下文时应用
	const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
	if (ExistingWorldContext)
	{
		Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
	}
	
	// 用于激活一组GameFeatureAction的lambda函数
	auto ActivateListOfActions = [&Context, this](const TArray<UGameFeatureAction*>& ActionList)
	{
		for (UGameFeatureAction* Action : ActionList)
		{
			if (Action == nullptr) continue;
			//@TODO: The fact that these don't take a world are potentially problematic in client-server PIE
			// The current behavior matches systems like gameplay tags where loading and registering apply to the entire process,
			// but actually applying the results to actors is restricted to a specific world
			Action->OnGameFeatureRegistering();
			Action->OnGameFeatureLoading();
			Action->OnGameFeatureActivating(Context);
			NumGameFeatureActionsLoading++;
		}
	};
	
	// 激活当前Experience中配置的ActionSets
	ActivateListOfActions(CurrentExperience->Actions);
	for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
	{
		if (ActionSet == nullptr) continue;
		ActivateListOfActions(ActionSet->Actions);
	} 
	
	// 激活当前UYcWorldSettings中的Actions和ActionSet
	if(AYcWorldSettings* YcWorldSettings = AYcWorldSettings::GetYcWorldSettings(this))
	{
		ActivateListOfActions(YcWorldSettings->Actions);
		for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : YcWorldSettings->ActionSets)
		{
			if (ActionSet == nullptr) continue;
			ActivateListOfActions(ActionSet->Actions);
		}
	}
	
	UE_LOG(LogYcGameplay, Log, TEXT("EXPERIENCE: OnExperienceFullLoadCompleted(CurrentExperience = %s, %s; NumGameFeatureActionsLoading = %d)"),
			*CurrentExperience->GetPrimaryAssetId().ToString(),
			*GetClientServerContextString(this), NumGameFeatureActionsLoading);
	
	LoadState = EYcExperienceLoadState::Loaded;
	
	// 按照优先级调用委托
	OnExperienceLoaded_HighPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_HighPriority.Clear();

	OnExperienceLoaded.Broadcast(CurrentExperience);
	OnExperienceLoaded.Clear();

	OnExperienceLoaded_LowPriority.Broadcast(CurrentExperience);
	OnExperienceLoaded_LowPriority.Clear();
	
	// @TODO 通知设置类Experience加载完成, 以提供设置对Experience系统的响应能力(非必要)
	// Apply any necessary scalability settings
// #if !UE_SERVER
// 	UYcSettingsLocal::Get()->OnExperienceLoaded();
// #endif
}

const UYcExperienceDefinition* UYcExperienceManagerComponent::GetCurrentExperienceChecked() const
{
	check(LoadState == EYcExperienceLoadState::Loaded);
	check(CurrentExperience != nullptr);
	return CurrentExperience;
}

bool UYcExperienceManagerComponent::IsExperienceLoaded() const
{
	return (LoadState == EYcExperienceLoadState::Loaded) && (CurrentExperience != nullptr);
}

void UYcExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_HighPriority(
	FOnYcExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_HighPriority.Add(MoveTemp(Delegate));
	}
}

void UYcExperienceManagerComponent::CallOrRegister_OnExperienceLoaded(FOnYcExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded.Add(MoveTemp(Delegate));
	}
}

void UYcExperienceManagerComponent::CallOrRegister_OnExperienceLoaded_LowPriority(FOnYcExperienceLoaded::FDelegate&& Delegate)
{
	if (IsExperienceLoaded())
	{
		Delegate.Execute(CurrentExperience);
	}
	else
	{
		OnExperienceLoaded_LowPriority.Add(MoveTemp(Delegate));
	}
}

void UYcExperienceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	// 禁用此体验加载的功能
	//@TODO: This should be handled FILO as well
	for (const FString& PluginURL : GameFeaturePluginURLs)
	{
		if (UYcExperienceManagerSubsystem::RequestToDeactivatePlugin(PluginURL))
		{
			UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
		}
	}
	
	//@TODO: Ensure proper handling of a partially-loaded state too
	if (LoadState == EYcExperienceLoadState::Loaded)
	{
		LoadState = EYcExperienceLoadState::Deactivating;

		// 确保如果有人注册为暂停器但立即触发，我们不会过早完成转换
		NumExpectedPausers = INDEX_NONE;
		NumObservedPausers = 0;

		// 反激活并卸载Action
		FGameFeatureDeactivatingContext Context(TEXT(""), [this](FStringView) { this->OnActionDeactivationCompleted(); });
	
		// 依次禁用所有刚才激活了的GameFeatureAction
		const FWorldContext* ExistingWorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (ExistingWorldContext)
		{
			Context.SetRequiredWorldContextHandle(ExistingWorldContext->ContextHandle);
		}

		auto DeactivateListOfActions = [&Context](const TArray<UGameFeatureAction*>& ActionList)
		{
			for (UGameFeatureAction* Action : ActionList)
			{
				if (!Action) continue;
				Action->OnGameFeatureDeactivating(Context);
				Action->OnGameFeatureUnregistering();
			}
		};

		DeactivateListOfActions(CurrentExperience->Actions);
		for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : CurrentExperience->ActionSets)
		{
			if (ActionSet == nullptr) continue;
			DeactivateListOfActions(ActionSet->Actions);
		}
		
		if(AYcWorldSettings* YcWorldSettings = AYcWorldSettings::GetYcWorldSettings(this))
		{
			DeactivateListOfActions(YcWorldSettings->Actions);
			for (const TObjectPtr<UYcExperienceActionSet>& ActionSet : YcWorldSettings->ActionSets)
			{
				if (ActionSet == nullptr) continue;
				DeactivateListOfActions(ActionSet->Actions);
			}
		}

		NumExpectedPausers = Context.GetNumPausers();

		if (NumExpectedPausers > 0)
		{
			UE_LOG(LogYcGameplay, Error, TEXT("Actions that have asynchronous deactivation aren't fully supported yet in SC experiences"));
		}

		if (NumExpectedPausers == NumObservedPausers)
		{
			OnAllActionsDeactivated();
		}
	}
}

bool UYcExperienceManagerComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
	const AYcWorldSettings* TypedWorldSettings = Cast<AYcWorldSettings>(GetOwner()->GetWorldSettings());
	if (TypedWorldSettings && !TypedWorldSettings->bEnableGameplayExperience)
	{
		return false;
	}
	if (LoadState != EYcExperienceLoadState::Loaded)
	{
		OutReason = TEXT("Experience still loading");
		return true;
	}
	else
	{
		return false;
	}
}

void UYcExperienceManagerComponent::OnActionDeactivationCompleted()
{
	check(IsInGameThread());
	++NumObservedPausers;

	if (NumObservedPausers == NumExpectedPausers)
	{
		OnAllActionsDeactivated();
	}
}

void UYcExperienceManagerComponent::OnAllActionsDeactivated()
{
	//@TODO: 目前来说实际上只是停用了，并没有完全卸载
	LoadState = EYcExperienceLoadState::Unloaded;
	CurrentExperience = nullptr;
	//@TODO: GEngine->ForceGarbageCollection(true);
}
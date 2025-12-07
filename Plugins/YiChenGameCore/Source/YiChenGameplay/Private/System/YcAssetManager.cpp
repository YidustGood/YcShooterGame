// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "System/YcAssetManager.h"
#include "YiChenGameplay.h"
#include "System/YcGameData.h"
#include "Character/YcPawnData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAssetManager)

const FName FYcBundles::Equipped("Equipped");

//////////////////////////////////////////////////////////////////////

/** 通过执行Yc.DumpLoadedAssets会在日志输出LoadedAssets中的资源对象信息 */
static FAutoConsoleCommand CVarDumpLoadedAssets(
	TEXT("Yc.DumpLoadedAssets"),
	TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
	FConsoleCommandDelegate::CreateStatic(UYcAssetManager::DumpLoadedAssets)
);

//////////////////////////////////////////////////////////////////////

#define STARTUP_JOB_WEIGHTED(JobFunc, JobWeight) StartupJobs.Add(FYcAssetManagerStartupJob(#JobFunc, [this](const FYcAssetManagerStartupJob& StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){JobFunc;}, JobWeight))
#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)

//////////////////////////////////////////////////////////////////////

UYcAssetManager::UYcAssetManager()
{
	DefaultPawnData = nullptr;
}

UYcAssetManager& UYcAssetManager::Get()
{
	check(GEngine);

	if (UYcAssetManager* Singleton = Cast<UYcAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	UE_LOG(LogYcGameplay, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to YcAssetManager!"));

	// 上面的致命错误会阻止执行到这里
	return *NewObject<UYcAssetManager>();
}

UObject* UYcAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (!AssetPath.IsValid()) return nullptr;
	
	TUniquePtr<FScopeLogTime> LogTimePtr; // 声明计时器, 基于RAII思想, 在超出作用域析构时输出从创建到结束的时间

	if (ShouldLogAssetLoads()) // 如果需要日志信息才创建计时器
	{
		LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
	}

	if (IsInitialized())
	{
		return GetStreamableManager().LoadSynchronous(AssetPath, false);
	}
	
	// 如果资产管理器尚未准备就绪，就使用“TryLoad”函数。
	return AssetPath.TryLoad();
}

bool UYcAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UYcAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical); // 加锁, 记录追踪传入的已加载资产对象
		LoadedAssets.Add(Asset); // 因为LoadedAssets被标记了UPROPERTY所以加入的资产对象将不会被GC
	}
}

void UYcAssetManager::DumpLoadedAssets()
{
	UE_LOG(LogYcGameplay, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	for (const UObject* LoadedAsset : Get().LoadedAssets)
	{
		UE_LOG(LogYcGameplay, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogYcGameplay, Log, TEXT("... %d assets in loaded pool"), Get().LoadedAssets.Num());
	UE_LOG(LogYcGameplay, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

void UYcAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("UYCAssetManager::StartInitialLoading"); //启动性能分析

	// 执行所有资产扫描，即使延迟加载也需要立即执行
	Super::StartInitialLoading();

	STARTUP_JOB(InitializeGameplayCueManager());

	{
		// 加载基础游戏数据资产
		STARTUP_JOB_WEIGHTED(GetGameData(), 25.f);
	}

	// 执行所有排队的启动工作
	DoAllStartupJobs();
}

void UYcAssetManager::InitializeGameplayCueManager()
{
	SCOPED_BOOT_TIMING("UYcAssetManager::InitializeGameplayCueManager");

	// @TODO 加载GameplayCues逻辑
}

const UYcGameData& UYcAssetManager::GetGameData()
{
	return GetOrLoadTypedGamePrimaryDataAsset<UYcGameData>(YcGameDataPath);
}

const UYcPawnData* UYcAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}

UPrimaryDataAsset* UYcAssetManager::LoadGamePrimaryDataAssetOfClass(TSubclassOf<UPrimaryDataAsset> DataClass,
	const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Game PrimaryDataAsset Object"), STAT_GameData, STATGROUP_LoadTime);
	
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		// FScopedSlowTask是虚幻引擎的长任务进度管理工具，用于显示耗时操作的进度，防止编辑器"假死"。
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("YcEditor", "BeginLoadingGamePrimaryDataAssetTask", "Loading Game PrimaryDataAsset {0}"), FText::FromName(DataClass->GetFName())));
		// 设置Dialog行为并显示Dialog
		constexpr bool bShowCancelButton = false;
		constexpr bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif
		UE_LOG(LogYcGameplay, Log, TEXT("Loading GamePrimaryDataAsset: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GamePrimaryDataAsset loaded!"), nullptr);

		// 在编辑器中可能被递归调用（因为在PostLoad时按需调用），所以强制主资产同步加载，其他资产异步加载
		// 编辑器和非编辑器状态下走不同的加载方式
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				// 直至所需资源加载完毕后才会停止。这会将所请求的资源推至优先级列表的首位, 0代表一直等待直到完成
				Handle->WaitUntilComplete(0.0f, false);

				// 这应该总是能成功
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}
	
	// 如果资产加载成功添加到Map中记录
	if (Asset)
	{
		GamePrimaryDataAssetMap.Add(DataClass, Asset);
	}
	else
	{
		// 主数据资产加载失败是不可接受的，会导致难以诊断的软错误, 所以我们直接Fatal掉，以便即使得到错误并处理它
		UE_LOG(LogYcGameplay, Fatal, TEXT("Failed to load GamePrimaryDataAsset asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return Asset;
}

void UYcAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("UYCAssetManager::DoAllStartupJobs");
	const double AllStartupJobsStartTime = FPlatformTime::Seconds();

	if (IsRunningDedicatedServer())
	{
		// DS端无需反馈进度，直接执行工作
		for (const FYcAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			StartupJob.DoJob();
		}
	}
	else
	{
		if (StartupJobs.Num() > 0)
		{
			float TotalJobValue = 0.0f;
			for (const FYcAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				TotalJobValue += StartupJob.JobWeight;
			}

			float AccumulatedJobValue = 0.0f;
			for (FYcAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				const float JobValue = StartupJob.JobWeight;
				StartupJob.SubstepProgressDelegate.BindLambda([This = this, AccumulatedJobValue, JobValue, TotalJobValue](float NewProgress)
					{
						const float SubstepAdjustment = FMath::Clamp(NewProgress, 0.0f, 1.0f) * JobValue;
						const float OverallPercentWithSubstep = (AccumulatedJobValue + SubstepAdjustment) / TotalJobValue;

						This->UpdateInitialGameContentLoadPercent(OverallPercentWithSubstep);
					});

				StartupJob.DoJob();

				StartupJob.SubstepProgressDelegate.Unbind();

				AccumulatedJobValue += JobValue;

				UpdateInitialGameContentLoadPercent(AccumulatedJobValue / TotalJobValue);
			}
		}
		else
		{
			UpdateInitialGameContentLoadPercent(1.0f);
		}
	}

	StartupJobs.Empty();

	UE_LOG(LogYcGameplay, Display, TEXT("All startup jobs took %.2f seconds to complete"), FPlatformTime::Seconds() - AllStartupJobsStartTime);
}

void UYcAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
	// @TODO: 可以将此进度路由到早期启动加载屏幕
}

#if WITH_EDITOR
void UYcAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("YcEditor", "BeginLoadingPIEData", "Loading PIE Data"));
		constexpr bool bShowCancelButton = false;
		constexpr bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

		const UYcGameData& LocalGameDataCommon = GetGameData();

		// 故意放在GetGameData之后，避免计时器计算GameData加载时间
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);

		// 可在此处添加其他需要预加载的资产
		// (例如：从世界设置获取默认Experience + 开发者设置中的Experience覆盖)
	}
}
#endif

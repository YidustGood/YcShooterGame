// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes//YcGameMode.h"

#include "CommonSessionSubsystem.h"
#include "CommonUserSubsystem.h"
#include "GameMapsSettings.h"
#include "YiChenGameplay.h"
#include "Character/YcCharacter.h"
#include "Development/YcGameDeveloperSettings.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameModes/YcGameSession.h"
#include "GameModes/YcGameState.h"
#include "GameModes/YcWorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Player/YcPlayerController.h"
#include "Player/YcPlayerState.h"
#include "System/YcAssetManager.h"
#include "Utils/CommonSimpleUtil.h"
#include "Character/YcPawnData.h"
#include "Character/YcPawnExtensionComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayCommon/GameplayMessageTypes.h"
#include "Player/YcPlayerSpawningManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameMode)

AYcGameMode::AYcGameMode(const FObjectInitializer& ObjectInitializer)
{
	GameStateClass = AYcGameState::StaticClass();
	GameSessionClass = AYcGameSession::StaticClass();
	DefaultPawnClass = AYcCharacter::StaticClass();
	PlayerStateClass = AYcPlayerController::StaticClass();
	PlayerControllerClass = AYcPlayerController::StaticClass();
}

void AYcGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// 延迟到下一帧开始寻找要使用的Experience，给初始化启动设置时间
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void AYcGameMode::InitGameState()
{
	Super::InitGameState();

	// 注册Experience加载完成事件的回调
	UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnYcExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void AYcGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	const AYcWorldSettings* TypedWorldSettings = Cast<AYcWorldSettings>(GetWorldSettings());
	// 判断world settings中是否开启游戏体验模式
	if (TypedWorldSettings && !TypedWorldSettings->bEnableGameplayExperience)
	{
		return Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
	
	// 如果Experience还未加载完成, 玩家的生成都会放到OnExperienceLoaded回调中处理
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

UClass* AYcGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const UYcPawnData* PawnData = GetPawnDataForController(InController);
	if (!PawnData || !PawnData->PawnClass) return Super::GetDefaultPawnClassForController_Implementation(InController);
	// 从PawnData中获取要生成的Pawn类
	return PawnData->PawnClass;
}

APawn* AYcGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// @TODO 这里可用于断线重连时查找并返回先前的Pawn
	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

APawn* AYcGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	const AYcWorldSettings* TypedWorldSettings = Cast<AYcWorldSettings>(GetWorldSettings());
	if (TypedWorldSettings && !TypedWorldSettings->bEnableGameplayExperience)
	{
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
	}
	
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient; // 永远不要将默认的Player Pawns保存到地图中
	SpawnInfo.bDeferConstruction = true;
	
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const UYcPawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					// PawnExtComp依赖PawnData推进初始化进度，这里设置后可推进其初始化
					PawnExtComp->SetPawnData(PawnData);
				}
				else
				{
					UE_LOG(LogYcGameplay, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]."), *GetNameSafe(SpawnedPawn));
				}
			}

			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
		else
		{
			UE_LOG(LogYcGameplay, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
		}
	}
	else
	{
		UE_LOG(LogYcGameplay, Error, TEXT("Game mode was unable to spawn Pawn due to NULL pawn class."));
	}

	return nullptr;
}

bool AYcGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return Super::ShouldSpawnAtStartSpot(Player);
}

AActor* AYcGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 通过自定义的玩家生成管理器组件来选择PlayerStart
	if (UYcPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UYcPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ChoosePlayerStart(Player);
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AYcGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	// 通知自定义的玩家生成管理器组件FinishRestartPlayer
	if (UYcPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UYcPlayerSpawningManagerComponent>())
	{
		PlayerSpawningComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}

	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

bool AYcGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

void AYcGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
	
	// 通过GMS广播新玩家/AI加入，此时Controller已完成初始化，以便其它系统模块响应，例如团队系统为玩家分配团队ID
	auto& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	const FGameModePlayerInitializedMessage Message(this, NewPlayer);
	MessageSubsystem.BroadcastMessage(YcGameplayTags::Gameplay_GameMode_PlayerInitialized, Message);
	
	// 同时也通过委托的形式通知，两种方式二选一使用即可
	OnGameModePlayerInitialized.Broadcast(this, NewPlayer);
}

void AYcGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer);

	// 如果尝试生成Pawn失败，则再次尝试。注意：在永久重试前检查是否确实有Pawn类
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
		{
			// 如果是玩家，不要无限循环，可能某些条件改变导致无法重生，此时停止尝试
			if (PlayerCanRestart(NewPC))
			{
				RequestPlayerRestartNextFrame(NewPlayer, false);
			}
			else
			{
				UE_LOG(LogYcGameplay, Verbose, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
			}
		}
		else
		{
			RequestPlayerRestartNextFrame(NewPlayer, false);
		}
	}
	else
	{
		UE_LOG(LogYcGameplay, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
	}
}

bool AYcGameMode::IsExperienceLoaded() const
{
	check(GameState);
	const UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
	check(ExperienceComponent);

	return ExperienceComponent->IsExperienceLoaded();
}

void AYcGameMode::HandleMatchAssignmentIfNotExpectingOne()
{
	AYcWorldSettings* TypedWorldSettings = Cast<AYcWorldSettings>(GetWorldSettings());
	// 如果关卡配置了关闭Game Experience功能就直接不执行后续逻辑了
	if (TypedWorldSettings && !TypedWorldSettings->bEnableGameplayExperience)
	{
		YC_LOG(LogYcGameplay, Log, TEXT("当前WorldSettings中未启用GameplayExperience。(AYcWorldSettings->bEnableGameplayExperience = false)"));
		return;
	}
	// Experience的主资产ID，用于提供给AssetManger寻找加载Experience资产
	FPrimaryAssetId ExperienceId; 
	// Experience的来源标记, 项目有多个Experience来源, 类似GameMode那样, 关卡设置是可被其它地方覆盖的, 所以这里记录一下真实的来源
	FString ExperienceIdSource;
	
	// 优先级顺序（最高优先级获胜）：
	//  - 匹配分配（如果存在）
	//  - URL选项覆盖
	//  - 开发者设置（仅PIE）
	//  - 命令行覆盖
	//  - 世界设置
	//  - 专用服务器
	//  - 默认体验
	
	UWorld* World = GetWorld();
	
	// 1. 先从关卡启动时传入的URL中尝试解析Experience字段获取ExperienceId
	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UYcExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}
	
	// 2. 如果ExperienceId无效，并且是PIE模式，则从UYcGameDeveloperSettings中获取ExperienceOverride
	if (!ExperienceId.IsValid() && World->IsPlayInEditor())
	{
		ExperienceId = GetDefault<UYcGameDeveloperSettings>()->ExperienceOverride;
		ExperienceIdSource = TEXT("DeveloperSettings");
	}
	
	// 3. 如果ExperienceId无效，查看启动命令行是否指定了Experience
	if (!ExperienceId.IsValid())
	{
		FString ExperienceFromCommandLine;
		if (FParse::Value(FCommandLine::Get(), TEXT("Experience="), ExperienceFromCommandLine))
		{
			ExperienceId = FPrimaryAssetId::ParseTypeAndName(ExperienceFromCommandLine);
			if (!ExperienceId.PrimaryAssetType.IsValid())
			{
				ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UYcExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromCommandLine));
			}
			ExperienceIdSource = TEXT("CommandLine");
		}
	}
	
	// 4. 如果ExperienceId无效，查看WorldSettings是否指定了默认的Experience
	if (!ExperienceId.IsValid())
	{
		if (TypedWorldSettings)
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}
	
	UYcAssetManager& AssetManager = UYcAssetManager::Get();
	FAssetData Dummy;
	if (ExperienceId.IsValid() && !AssetManager.GetPrimaryAssetData(ExperienceId, /*out*/ Dummy))
	{
		UE_LOG(LogYcGameplay, Error, TEXT("EXPERIENCE: Wanted to use %s but couldn't find it, falling back to the default)"), *ExperienceId.ToString());
		ExperienceId = FPrimaryAssetId();
	}
	
	// 最终回退到默认体验
	if (!ExperienceId.IsValid())
	{
		if (TryDedicatedServerLogin())
		{
			// @TODO 这将作为专用服务器启动
			return;
		}

		// @TODO: 可以将保底的默认体验移至config或其他地方，这里是硬编码
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("YcExperienceDefinition"), FName("BP_YcDefaultExperience")); // 从蓝图类中寻找默认的ExperienceDef
		ExperienceIdSource = TEXT("Default");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

void AYcGameMode::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	if (ExperienceId.IsValid())
	{
		UE_LOG(LogYcGameplay, Log, TEXT("Identified experience %s (Source: %s)"), *ExperienceId.ToString(), *ExperienceIdSource);

		UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
		check(ExperienceComponent);
		ExperienceComponent->SetCurrentExperience(ExperienceId);
	}
	else
	{
		UE_LOG(LogYcGameplay, Error, TEXT("Failed to identify experience, loading screen will stay up forever"));
	}
}

void AYcGameMode::OnExperienceLoaded(const UYcExperienceDefinition* CurrentExperience)
{
	// 为所有已连接的玩家生成Pawn
	// @TODO: 这里只处理了玩家控制器，但在GetDefaultPawnClassForController_Implementation中我们跳过了所有控制器
	// GetDefaultPawnClassForController_Implementation可能只会为玩家调用
	// 尝试为所有PlayerController生成Pawn
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

void AYcGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
	// @TODO 处理Bot重生逻辑
}

bool AYcGameMode::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}
	else
	{
		// Bot version of Super::PlayerCanRestart_Implementation
		if ((Controller == nullptr) || Controller->IsPendingKillPending())
		{
			return false;
		}
	}

	if (UYcPlayerSpawningManagerComponent* PlayerSpawningComponent = GameState->FindComponentByClass<UYcPlayerSpawningManagerComponent>())
	{
		return PlayerSpawningComponent->ControllerCanRestart(Controller);
	}

	return true;
}

const UYcPawnData* AYcGameMode::GetPawnDataForController(const AController* InController) const
{
	// 首先查看PlayerState上是否已设置PawnData
	if (InController != nullptr)
	{
		if (const AYcPlayerState* YcPS = InController->GetPlayerState<AYcPlayerState>())
		{
			if (const UYcPawnData* PawnData = YcPS->GetPawnData<UYcPawnData>())
			{
				return PawnData;
			}
		}
	}

	// 如果没有，则回退到当前Experience的默认PawnData
	check(GameState);
	UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
	check(ExperienceComponent);

	if (ExperienceComponent->IsExperienceLoaded())
	{
		const UYcExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperienceChecked();
		if (Experience->DefaultPawnData != nullptr)
		{
			return Experience->DefaultPawnData;
		}

		// Experience已加载但仍然没有PawnData，回退到全局默认值
		return UYcAssetManager::Get().GetDefaultPawnData();
	}

	// Experience尚未加载，因此没有可用的PawnData
	return nullptr;
}

bool AYcGameMode::TryDedicatedServerLogin()
{
	// 一些基本代码用于注册为活动的专用服务器，这将根据游戏需求进行大量修改
	FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && World && World->GetNetMode() == NM_DedicatedServer && World->URL.Map == DefaultMap)
	{
		// 仅在专用服务器的默认地图上注册
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

		// 专用服务器可能需要进行在线登录
		UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &AYcGameMode::OnUserInitializedForDedicatedServer);

		// 专用服务器上没有本地用户，但索引0表示默认平台用户，由在线登录代码处理
		if (!UserSubsystem->TryToLoginForOnlinePlay(0))
		{
			OnUserInitializedForDedicatedServer(nullptr, false, FText(), ECommonUserPrivilege::CanPlayOnline, ECommonUserOnlineContext::Default);
		}

		return true;
	}

	return false;
}

void AYcGameMode::HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode)
{
	// @TODO 待实现
}

void AYcGameMode::OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
	ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		// 解绑委托
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
		UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &AYcGameMode::OnUserInitializedForDedicatedServer);

		if (bSuccess)
		{
			// 在线登录成功，启动完整的在线游戏
			UE_LOG(LogYcGameplay, Log, TEXT("Dedicated server online login succeeded, starting online server"));
			HostDedicatedServerMatch(ECommonSessionOnlineMode::Online);
		}
		else
		{
			// 无论如何都尝试托管，但不支持在线功能
			// 此行为相当特定于游戏，但这种行为为测试提供了最大的灵活性
			UE_LOG(LogYcGameplay, Log, TEXT("Dedicated server online login failed, starting LAN-only server"));
			HostDedicatedServerMatch(ECommonSessionOnlineMode::LAN);
		}
	}
}
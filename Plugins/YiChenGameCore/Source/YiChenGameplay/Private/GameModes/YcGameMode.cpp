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

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameMode)

AYcGameMode::AYcGameMode(const FObjectInitializer& ObjectInitializer)
{
	GameStateClass = AYcGameState::StaticClass();
	GameSessionClass = AYcGameSession::StaticClass();
	DefaultPawnClass = AYcCharacter::StaticClass();
	PlayerStateClass = AYcPlayerController::StaticClass();
	PlayerControllerClass = AYcPlayerController::StaticClass();
}

void AYcGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AYcGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AYcGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// Wait for the next frame to give time to initialize startup settings
	// 等待到下一帧开始寻找要使用的Experience，给初始化启动设置时间
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
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
	
	// Precedence order (highest wins)
	//  - Matchmaking assignment (if present)
	//  - URL Options override
	//  - Developer Settings (PIE only)
	//  - Command Line override
	//  - World Settings
	//  - Dedicated server
	//  - Default experience
	
	UWorld* World = GetWorld();
	
	// 1. 先从关卡启动时传入的URLs中尝试解析Experience字段获取ExperienceId
	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UYcExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}
	
	// 2. 如果ExperienceId无效，并且是PIE模式，那么从UYcGameDeveloperSettings中获得ExperienceOverride
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
	
	// 4. 如果ExperienceId无效，查看WorldSettings是否指定了默认的Experience类
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
	
	// Final fallback to the default experience
	if (!ExperienceId.IsValid())
	{
		if (TryDedicatedServerLogin())
		{
			// @TODO This will start to host as a dedicated server
			return;
		}

		// @TODO: 可以将保底的默认体验移动至config或者其他地方, 这里是一个硬编码
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("YcExperienceDefinition"), FName("BP_YcDefaultExperience")); //从蓝图类中寻找默认的ExperienceDef
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
}

const UYcPawnData* AYcGameMode::GetPawnDataForController(const AController* InController) const
{
	// See if pawn data is already set on the player state
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

	// If not, fall back to the the default for the current experience
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

		// Experience is loaded and there's still no pawn data, fall back to the default for now
		// 体验已经加载，但仍然没有pawn数据，暂时回退到默认值
		return UYcAssetManager::Get().GetDefaultPawnData();
	}

	// Experience not loaded yet, so there is no pawn data to be had
	return nullptr;
}

bool AYcGameMode::TryDedicatedServerLogin()
{
	// Some basic code to register as an active dedicated server, this would be heavily modified by the game
	FString DefaultMap = UGameMapsSettings::GetGameDefaultMap();
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance && World && World->GetNetMode() == NM_DedicatedServer && World->URL.Map == DefaultMap)
	{
		// Only register if this is the default map on a dedicated server
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();

		// Dedicated servers may need to do an online login
		UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &AYcGameMode::OnUserInitializedForDedicatedServer);

		// There are no local users on dedicated server, but index 0 means the default platform user which is handled by the online login code
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
		// Unbind
		UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
		UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &AYcGameMode::OnUserInitializedForDedicatedServer);

		if (bSuccess)
		{
			// Online login worked, start a full online game
			UE_LOG(LogYcGameplay, Log, TEXT("Dedicated server online login succeeded, starting online server"));
			HostDedicatedServerMatch(ECommonSessionOnlineMode::Online);
		}
		else
		{
			// Go ahead and try to host anyway, but without online support
			// This behavior is fairly game specific, but this behavior provides the most flexibility for testing
			UE_LOG(LogYcGameplay, Log, TEXT("Dedicated server online login failed, starting LAN-only server"));
			HostDedicatedServerMatch(ECommonSessionOnlineMode::LAN);
		}
	}
}
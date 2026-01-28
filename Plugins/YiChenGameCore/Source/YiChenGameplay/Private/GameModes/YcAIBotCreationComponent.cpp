// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcAIBotCreationComponent.h"

#include "AIController.h"
#include "YiChenGameplay.h"
#include "Character/YcHealthComponent.h"
#include "Character/YcPawnExtensionComponent.h"
#include "Development/YcGameDeveloperSettings.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameModes/YcGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "System/YcGameSystemStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAIBotCreationComponent)

UYcAIBotCreationComponent::UYcAIBotCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UYcAIBotCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// 监听Experience加载完成事件
	// 只有在Experience加载完成后才能正确创建Bot（需要Pawn类、能力等资源）
	const AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnYcExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UYcAIBotCreationComponent::OnExperienceLoaded(const UYcExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	// 确保只在服务器端创建Bot（客户端不需要创建）
	if (HasAuthority())
	{
		ServerCreateBots();
	}
#endif
}

#if WITH_SERVER_CODE
void UYcAIBotCreationComponent::ServerCreateBots_Implementation()
{
	if (BotControllerClass == nullptr)
	{
		return;
	}
	
	// 初始化可用名称列表
	RemainingBotNames = RandomBotNames;

	// 确定要生成的Bot数量（优先级：默认配置 < 开发者设置 < 命令行参数 < URL参数）
	int32 EffectiveBotCount = NumBotsToCreate;

	// 编辑器模式下，开发者设置可以覆盖Bot数量
	if (GIsEditor)
	{
		const UYcGameDeveloperSettings* DeveloperSettings = GetDefault<UYcGameDeveloperSettings>();
		
		if (DeveloperSettings->bOverrideBotCount)
		{
			EffectiveBotCount = DeveloperSettings->OverrideNumPlayerBotsToSpawn;
		}
	}

	// 命令行参数可以覆盖Bot数量（例如：-NumBots=10）
	FString CMDNumBots;
	if (UYcGameSystemStatics::GetMatchedCommandLine(TEXT("-NumBots="),CMDNumBots))
	{
		EffectiveBotCount = FCString::Atoi(*CMDNumBots);
	}
	
	// URL参数具有最高优先级（例如：?NumBots=10）
	// 这允许在运行时动态指定Bot数量
	if (AGameModeBase* GameModeBase = GetGameMode<AGameModeBase>())
	{
		EffectiveBotCount = UGameplayStatics::GetIntOption(GameModeBase->OptionsString, TEXT("NumBots"), EffectiveBotCount);
	}

	// 批量创建Bot
	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
	{
		SpawnOneBot();
	}
}

void UYcAIBotCreationComponent::SpawnOneBot()
{
	// 配置生成参数
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;
	
	// 生成AI控制器
	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (NewController == nullptr)
	{
		UE_LOG(LogYcGameplay, Warning, TEXT("SpawnOneBot Failed"));
		return;
	}

	// 初始化Bot
	AYcGameMode* GameMode = GetGameMode<AYcGameMode>();
	check(GameMode);
	
	// 设置Bot名称
	if (NewController->PlayerState != nullptr)
	{
		NewController->PlayerState->SetPlayerName(CreateBotName(NewController->PlayerState->GetPlayerId()));
	}
	
	// 通用玩家初始化（设置队伍、能力等）
	GameMode->GenericPlayerInitialization(NewController);
	
	// 重启玩家（生成Pawn）
	GameMode->RestartPlayer(NewController);

	// 确保Pawn扩展组件正确初始化
	if (NewController->GetPawn() != nullptr)
	{
		if (UYcPawnExtensionComponent* PawnExtComponent = NewController->GetPawn()->FindComponentByClass<UYcPawnExtensionComponent>())
		{
			PawnExtComponent->CheckDefaultInitialization();
		}
	}

	// 记录已生成的Bot
	SpawnedBotList.Add(NewController);
}

void UYcAIBotCreationComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() <= 0) return;
	
	// 随机选择一个Bot进行移除
	// 注意：当前实现是随机移除，未来可以根据技能等级等因素选择性移除
	const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() - 1);

	AAIController* BotToRemove = SpawnedBotList[BotToRemoveIndex];
	SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);
	if (!BotToRemove) return;

	// 优先通过健康组件触发死亡流程（播放死亡动画等）
	// 如果没有健康组件，则直接销毁Pawn
	if (APawn* ControlledPawn = BotToRemove->GetPawn())
	{
		if (UYcHealthComponent* HealthComponent = UYcHealthComponent::FindHealthComponent(ControlledPawn))
		{
			// 注意：当前实现存在问题 - 控制器销毁时PlayerState会立即消失
			// 这会导致死亡动画等能力被立即中断
			// TODO: 考虑延迟销毁控制器，等待死亡流程完成
			HealthComponent->DamageSelfDestruct();
		}
		else
		{
			ControlledPawn->Destroy();
		}
	}

	// 销毁控制器（会触发Logout等流程）
	BotToRemove->Destroy();
}

FString UYcAIBotCreationComponent::CreateBotName(int32 PlayerIndex)
{
	FString Result;
	
	// 优先从预设名称列表中随机选择
	if (RemainingBotNames.Num() > 0)
	{
		const int32 NameIndex = FMath::RandRange(0, RemainingBotNames.Num() - 1);
		Result = RemainingBotNames[NameIndex];
		RemainingBotNames.RemoveAtSwap(NameIndex);
	}
	else
	{
		// 名称列表用完后，生成默认名称
		// TODO: PlayerId目前只为真实玩家初始化，这里使用随机数代替
		PlayerIndex = FMath::RandRange(260, 260+100);
		Result = FString::Printf(TEXT("Tinplate %d"), PlayerIndex);
	}
	return Result;
}
#else // !WITH_SERVER_CODE

// 客户端版本不包含Bot功能，这些函数不应被调用
void UYcAIBotCreationComponent::ServerCreateBots_Implementation()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in YcGameClient!"));
}

void UYcAIBotCreationComponent::SpawnOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in YcGameClient!"));
}

void UYcAIBotCreationComponent::RemoveOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in YcGameClient!"));
}

#endif



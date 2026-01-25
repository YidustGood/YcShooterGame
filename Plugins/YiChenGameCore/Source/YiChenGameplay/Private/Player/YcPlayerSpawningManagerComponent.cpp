// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerSpawningManagerComponent.h"

#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerState.h"
#include "Player/YcPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerSpawningManagerComponent)


DEFINE_LOG_CATEGORY_STATIC(LogPlayerSpawning, Log, All);

UYcPlayerSpawningManagerComponent::UYcPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(false);
	bAutoRegister = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UYcPlayerSpawningManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelAdded);

	UWorld* World = GetWorld();
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleOnActorSpawned));
	
	// 收集并缓存World中的AYcPlayerStart对象
	for (TActorIterator<AYcPlayerStart> It(World); It; ++It)
	{
		if (AYcPlayerStart* PlayerStart = *It)
		{
			CachedPlayerStarts.Add(PlayerStart);
		}
	}
}

void UYcPlayerSpawningManagerComponent::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld != GetWorld()) return;
	
	// 收集新加载关卡中的AYcPlayerStart对象
	for (AActor* Actor : InLevel->Actors)
	{
		if (AYcPlayerStart* PlayerStart = Cast<AYcPlayerStart>(Actor))
		{
			ensure(!CachedPlayerStarts.Contains(PlayerStart));
			CachedPlayerStarts.Add(PlayerStart);
		}
	}
}

void UYcPlayerSpawningManagerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	
	// 缓存动态创建的AYcPlayerStart对象
	if (AYcPlayerStart* PlayerStart = Cast<AYcPlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.Add(PlayerStart);
	}
}

AActor* UYcPlayerSpawningManagerComponent::ChoosePlayerStart(AController* Player)
{
	if (!Player) return nullptr;
	
#if WITH_EDITOR
	if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
	{
		return PlayerStart;
	}
#endif

	TArray<AYcPlayerStart*> StarterPoints;
	for (auto StartIt = CachedPlayerStarts.CreateIterator(); StartIt; ++StartIt)
	{
		if (AYcPlayerStart* Start = StartIt->Get())
		{
			StarterPoints.Add(Start);
		}
		else
		{
			StartIt.RemoveCurrent();
		}
	}

	if (APlayerState* PlayerState = Player->GetPlayerState<APlayerState>())
	{
		// 纯观察者在任意随机出生点开始，但不占用它
		if (PlayerState->IsOnlyASpectator())
		{
			if (!StarterPoints.IsEmpty())
			{
				return StarterPoints[FMath::RandRange(0, StarterPoints.Num() - 1)];
			}

			return nullptr;
		}
	}

	AActor* PlayerStart = OnChoosePlayerStart(Player, StarterPoints);

	if (!PlayerStart)
	{
		PlayerStart = GetFirstRandomUnoccupiedPlayerStart(Player, StarterPoints);
	}

	if (AYcPlayerStart* YcStart = Cast<AYcPlayerStart>(PlayerStart))
	{
		YcStart->TryClaim(Player);
	}

	return PlayerStart;
}

#if WITH_EDITOR
APlayerStart* UYcPlayerSpawningManagerComponent::FindPlayFromHereStart(AController* Player)
{
	// 只对玩家控制器使用"从此处开始游戏"，AI等应从正常出生点生成
	if (!Player->IsA<APlayerController>()) return nullptr;
	
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		if (APlayerStart* PlayerStart = *It)
		{
			if (PlayerStart->IsA<APlayerStartPIE>())
			{
				// 在PIE模式下，始终优先使用第一个"从此处开始游戏"的出生点
				return PlayerStart;
			}
		}
	}
	
	return nullptr;
}
#endif

bool UYcPlayerSpawningManagerComponent::ControllerCanRestart(AController* Player)
{
	bool bCanRestart = true;

	// TODO 检查是否可以重生

	return bCanRestart;
}

void UYcPlayerSpawningManagerComponent::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(NewPlayer, StartRotation);
	K2_OnFinishRestartPlayer(NewPlayer, StartRotation);
}

void UYcPlayerSpawningManagerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

APlayerStart* UYcPlayerSpawningManagerComponent::GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AYcPlayerStart*>& StartPoints) const
{
	if (Controller)
	{
		TArray<AYcPlayerStart*> UnOccupiedStartPoints;
		TArray<AYcPlayerStart*> OccupiedStartPoints;

		// 将出生点分类为未占用和部分占用
		for (AYcPlayerStart* StartPoint : StartPoints)
		{
			const EYcPlayerStartLocationOccupancy State = StartPoint->GetLocationOccupancy(Controller);

			switch (State)
			{
				case EYcPlayerStartLocationOccupancy::Empty:
					UnOccupiedStartPoints.Add(StartPoint);
					break;
				case EYcPlayerStartLocationOccupancy::Partial:
					OccupiedStartPoints.Add(StartPoint);
					break;
				default: ;
			}
		}

		// 优先选择完全未占用的出生点
		if (UnOccupiedStartPoints.Num() > 0)
		{
			return UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		// 其次选择部分占用的出生点
		else if (OccupiedStartPoints.Num() > 0)
		{
			return OccupiedStartPoints[FMath::RandRange(0, OccupiedStartPoints.Num() - 1)];
		}
	}

	return nullptr;
}

AActor* UYcPlayerSpawningManagerComponent::OnChoosePlayerStart_Implementation(AController* Player,
	TArray<AYcPlayerStart*>& PlayerStarts)
{
	return nullptr;
}
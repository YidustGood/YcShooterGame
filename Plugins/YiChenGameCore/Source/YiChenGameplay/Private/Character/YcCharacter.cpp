// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcCharacter.h"
#include "YcAbilitySystemComponent.h"
#include "YcTeamAgentInterface.h"
#include "YcTeamSubsystem.h"
#include "Character/YcHealthComponent.h"
#include "Character/YcPawnExtensionComponent.h"
#include "Character/YcMovementSyncComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayCommon/GameplayMessageTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/YcPlayerController.h"
#include "Player/YcPlayerState.h"
#include "YiChenGameplay.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcCharacter)

AYcCharacter::AYcCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	// Network
	bReplicateUsingRegisteredSubObjectList = true;
	SetReplicates(true);
	
	// 创建PawnExtComponent组件, 这个最好作为本类第一个组件, 好协调其它功能扩展组件初始化
	PawnExtComponent = CreateDefaultSubobject<UYcPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	
	// 创建HealthComponent组件, 并绑定健康生命周期回调函数
	HealthComponent = CreateDefaultSubobject<UYcHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);
	
	// 创建MovementSyncComponent组件，用于同步AttributeSet中的移动速度到CharacterMovementComponent
	MovementSyncComponent = CreateDefaultSubobject<UYcMovementSyncComponent>(TEXT("MovementSyncComponent"));
}

void AYcCharacter::Reset()
{
	// 自定义重置逻辑, 以便走符合项目框架逻辑的销毁逻辑
	DisableMovementAndCollision();

	K2_OnReset();
	
	UninitAndDestroy();
}

AYcPlayerController* AYcCharacter::GetYcPlayerController() const
{
	if (!IsPlayerControlled()) return nullptr; // 确保是玩家控制的角色
	return CastChecked<AYcPlayerController>(Controller, ECastCheckedType::NullAllowed);
}

AYcPlayerState* AYcCharacter::GetYcPlayerState() const
{
	return CastChecked<AYcPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UYcAbilitySystemComponent* AYcCharacter::GetYcAbilitySystemComponent() const
{
	return Cast<UYcAbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* AYcCharacter::GetAbilitySystemComponent() const
{
	if (PawnExtComponent == nullptr)
	{
		return nullptr;
	}

	return PawnExtComponent->GetYcAbilitySystemComponent();
}

void AYcCharacter::OnAbilitySystemInitialized()
{
	UYcAbilitySystemComponent* ASC = GetYcAbilitySystemComponent();
	check(ASC);
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FAbilitySystemLifeCycleMessage Message;
	Message.ASC = ASC;
	Message.Owner = this;
	MessageSubsystem.BroadcastMessage(YcGameplayTags::TAG_Ability_State_AbilitySystemInitialized, Message);
	UKismetSystemLibrary::PrintString(this, TEXT("AYcCharacter::OnAbilitySystemInitialized()"));
}

void AYcCharacter::OnAbilitySystemUninitialized()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	FAbilitySystemLifeCycleMessage Message;
	Message.ASC = GetAbilitySystemComponent();
	Message.Owner = this;
	MessageSubsystem.BroadcastMessage(YcGameplayTags::TAG_Ability_State_AbilitySystemInitialized, Message);
}

void AYcCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	// 通知PawnExtComponent以便其推进初始化进度
	PawnExtComponent->HandleControllerChanged();
}

void AYcCharacter::UnPossessed()
{
	Super::UnPossessed();
	
	PawnExtComponent->HandleControllerChanged();
}

void AYcCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	// 通知PawnExtComponent以便其推进初始化进度
	PawnExtComponent->HandleControllerChanged();
}

void AYcCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// 通知PawnExtComponent以便其推进初始化进度
	PawnExtComponent->HandlePlayerStateReplicated();
}

void AYcCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// 通知PawnExtComponent InputComponent准备好了, 以便其推进初始化进度
	PawnExtComponent->SetupPlayerInputComponent();
}

void AYcCharacter::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
	K2_OnDeathStarted();
}

void AYcCharacter::OnDeathFinished(AActor* OwningActor)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}

void AYcCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}

void AYcCharacter::UninitAndDestroy()
{
	// 检查是否为服务器端（权威端）
	if (GetLocalRole() == ROLE_Authority)
	{
		// 从控制器分离并等待销毁
		DetachFromControllerPendingDestroy();
		// 设置0.1秒后自动销毁生命周期
		SetLifeSpan(0.1f);
	}

	// 如果我们仍然是Avatar Actor，则反初始化ASC（否则当其他Pawn成为Avatar Actor时已经处理了）
	if (const UYcAbilitySystemComponent* ASC = GetYcAbilitySystemComponent())
	{
		// 确认当前角色仍是能力系统的Avatar Actor
		if (ASC->GetAvatarActor() == this)
		{
			// 清理能力系统相关资源
			PawnExtComponent->UninitializeAbilitySystem();
		}
	}

	// 隐藏游戏中的角色模型
	SetActorHiddenInGame(true);
}

void AYcCharacter::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	UCharacterMovementComponent* MoveComp = CastChecked<UCharacterMovementComponent>(GetCharacterMovement());
	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
}

void AYcCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// Character 不允许直接设置队伍ID
	// 队伍ID应该通过 Controller 的 PlayerState 来设置
	UE_LOG(LogYcGameplay, Error, TEXT("You can't set the team ID on a character (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AYcCharacter::GetGenericTeamId() const
{
	// 性能优化：直接从 Controller 的 PlayerState 获取
	// 避免使用 UYcTeamSubsystem::FindTeamAgentFromActor 的多次类型转换和查找开销
	const AController* MyController = GetController();
	if (MyController == nullptr)
	{
		return FGenericTeamId::NoTeam;
	}
	
	// 直接转换 PlayerState，这是最快的方式
	if (const IYcTeamAgentInterface* TeamAgent = Cast<IYcTeamAgentInterface>(MyController->PlayerState))
	{
		return TeamAgent->GetGenericTeamId();
	}
	
	return FGenericTeamId::NoTeam;
}

FOnYcTeamIndexChangedDelegate* AYcCharacter::GetOnTeamIndexChangedDelegate()
{
	// 从 Controller 的 PlayerState 获取
	AController* MyController = GetController();
	if (MyController == nullptr)
	{
		return nullptr;
	}
	
	if (IYcTeamAgentInterface* TeamAgent = Cast<IYcTeamAgentInterface>(MyController->PlayerState))
	{
		return TeamAgent->GetOnTeamIndexChangedDelegate();
	}
	
	UE_LOG(LogYcGameplay, Warning, TEXT("GetOnTeamIndexChangedDelegate called on character (%s) with no valid team agent"), *GetPathNameSafe(this));
	return nullptr;
}

ETeamAttitude::Type AYcCharacter::GetTeamAttitudeTowards(const AActor& Other) const
{
	// 性能优化：直接获取双方的队伍ID进行比较
	// 避免复杂的查找逻辑和多次虚函数调用
	
	// 获取自己的队伍ID
	const FGenericTeamId MyTeamID = GetGenericTeamId();
	if (MyTeamID == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}
	
	// 获取目标的队伍ID（优化：直接尝试最常见的情况）
	FGenericTeamId OtherTeamID = FGenericTeamId::NoTeam;
	
	// 情况1：目标是 Pawn（最常见）
	if (const APawn* OtherPawn = Cast<APawn>(&Other))
	{
		if (const AController* OtherController = OtherPawn->GetController())
		{
			if (const IYcTeamAgentInterface* OtherTeamAgent = Cast<IYcTeamAgentInterface>(OtherController->PlayerState))
			{
				OtherTeamID = OtherTeamAgent->GetGenericTeamId();
			}
		}
	}
	// 情况2：目标是 Controller
	else if (const AController* OtherController = Cast<AController>(&Other))
	{
		if (const IYcTeamAgentInterface* OtherTeamAgent = Cast<IYcTeamAgentInterface>(OtherController->PlayerState))
		{
			OtherTeamID = OtherTeamAgent->GetGenericTeamId();
		}
	}
	// 情况3：特殊情况（如投射物、陷阱等），使用 FindTeamAgentFromActor
	else
	{
		TScriptInterface<IYcTeamAgentInterface> OtherTeamAgent;
		if (UYcTeamSubsystem::FindTeamAgentFromActor(const_cast<AActor*>(&Other), OtherTeamAgent))
		{
			OtherTeamID = OtherTeamAgent->GetGenericTeamId();
		}
	}

	// 比较队伍ID并返回态度
	if (OtherTeamID == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}
	
	return (OtherTeamID.GetId() != MyTeamID.GetId()) ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
}
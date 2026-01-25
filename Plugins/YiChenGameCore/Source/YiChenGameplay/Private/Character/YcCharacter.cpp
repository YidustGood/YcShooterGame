// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcCharacter.h"
#include "YcAbilitySystemComponent.h"
#include "Character/YcHealthComponent.h"
#include "Character/YcPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayCommon/GameplayMessageTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/YcPlayerController.h"
#include "Player/YcPlayerState.h"

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
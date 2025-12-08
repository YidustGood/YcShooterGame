// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcCharacter.h"
#include "YcAbilitySystemComponent.h"
#include "Character/YcPawnExtensionComponent.h"
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
}

void AYcCharacter::OnAbilitySystemUninitialized()
{
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
// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcHeroComponent.h"

#include "EnhancedInputSubsystems.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "Character/YcPawnData.h"
#include "Character/YcPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Input/YcInputComponent.h"
#include "Misc/UObjectToken.h"
#include "Player/YcPlayerController.h"
#include "Player/YcPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHeroComponent)

const FName UYcHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UYcHeroComponent::NAME_ActorFeatureName("Hero");

bool UYcHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	// 检查是否可以从无效状态转换到Spawned状态
	if (!CurrentState.IsValid() && DesiredState == YcGameplayTags::InitState_Spawned)
	{
		// 只要存在有效的Pawn就允许转换
		if (Pawn) return true;
	}
	// 检查是否可以从Spawned转换到DataAvailable状态
	else if (CurrentState == YcGameplayTags::InitState_Spawned && DesiredState == YcGameplayTags::InitState_DataAvailable)
	{
		// 需要PlayerState有效
		if (!GetPlayerState<AYcPlayerState>()) return false;

		// 如果是权威端或自主控制，需要等待Controller与PlayerState配对
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			const AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS) return false;
		}

		// 如果是本地控制的玩家Pawn（非AI），还需要确保InputComponent和LocalPlayer有效
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AYcPlayerController* YcPC = GetController<AYcPlayerController>();

			// 本地控制时需要InputComponent和LocalPlayer有效
			if (!Pawn->InputComponent || !YcPC || !YcPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	// 检查是否可用从DataAvailable转换到DataInitialized状态
	else if (CurrentState == YcGameplayTags::InitState_DataAvailable && DesiredState == YcGameplayTags::InitState_DataInitialized)
	{
		// 等待PlayerState和PawnExtensionComponent初始化完成
		AYcPlayerState* YcPS = GetPlayerState<AYcPlayerState>();
		bool bCheckPawnExt = Manager->HasFeatureReachedInitState(Pawn, UYcPawnExtensionComponent::NAME_ActorFeatureName, YcGameplayTags::InitState_DataInitialized) ||
							Manager->HasFeatureReachedInitState(Pawn, UYcPawnExtensionComponent::NAME_ActorFeatureName, YcGameplayTags::InitState_GameplayReady);
		
		// 确保PawnExtensionComponent已初始化，因为此阶段需要PawnData有效
		return YcPS && bCheckPawnExt;
	}
	// 检查是否可以从DataInitialized转换到GameplayReady状态
	else if (CurrentState == YcGameplayTags::InitState_DataInitialized && DesiredState == YcGameplayTags::InitState_GameplayReady)
	{
		// @TODO 添加Ability初始化检查?
		return true;
	}

	return false;
}

void UYcHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("HandleChangeInitState [%s] / Role [%s] / CurrentState[%s] / DesiredState[%s]"),
	       *GetNameSafe(GetOwner()), *LocalRoleName, *CurrentState.ToString(), *DesiredState.ToString());
#endif
	
	if (CurrentState == YcGameplayTags::InitState_DataAvailable && DesiredState == YcGameplayTags::InitState_DataInitialized)
	{
		const APawn* Pawn = GetPawn<APawn>();
		AYcPlayerState* YcPS = GetPlayerState<AYcPlayerState>();
		if (!ensure(Pawn && YcPS)) return;

		const UYcPawnData* PawnData = nullptr;

		if (UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UYcPawnData>();

			// 玩家状态（PlayerState）持有该玩家的持久性数据（在死亡和多个Pawn之间持续存在的状态）。
			// 技能系统组件（Ability System Component）和属性集（Attribute Sets）存在于玩家状态上。
			PawnExtComp->InitializeAbilitySystem(YcPS->GetYcAbilitySystemComponent(), YcPS);
		}

		if (AYcPlayerController* YcPC = GetController<AYcPlayerController>())
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}
	}
}

void UYcHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("OnActorInitStateChanged [%s] / Role [%s] FeatureName[%s] / FeatureState[%s]"), *GetNameSafe(GetOwner()), *LocalRoleName, *Params.FeatureName.ToString(), *Params.FeatureState.ToString())
#endif
	
	if (Params.FeatureName == UYcPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == YcGameplayTags::InitState_DataInitialized)
		{
			// 如果PawnExtensionComponent报告所有其他组件均已初始化完成，则尝试推进到下一状态
			CheckDefaultInitialization();
		}
	}
}

void UYcHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { YcGameplayTags::InitState_Spawned, YcGameplayTags::InitState_DataAvailable, YcGameplayTags::InitState_DataInitialized, YcGameplayTags::InitState_GameplayReady };
	
	// 此流程将从"InitState_Spawned"状态（仅在BeginPlay中设置）开始，依次推进数据初始化各阶段，直至达到游戏可运行状态(InitState_GameplayReady)。
	ContinueInitStateChain(StateChain);
}


UYcHeroComponent::UYcHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReadyToBindInputs = false;
}

void UYcHeroComponent::AddAdditionalInputConfig(const UYcInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;
	
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;
	
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);
	
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);
	
	if (const UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UYcInputComponent* YcIC = Pawn->FindComponentByClass<UYcInputComponent>();
		if (ensureMsgf(YcIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UYcInputComponent or a subclass of it.")))
		{
			YcIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void UYcHeroComponent::RemoveAdditionalInputConfig(const UYcInputConfig* InputConfig)
{
	//@TODO: 待实现移除InputConfig功能
}

bool UYcHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UYcHeroComponent::OnRegister()
{
	Super::OnRegister();
	// 检验是否挂载在APawn及其子类上的, 如果不是发出错误警告
	if (!GetPawn<APawn>())
	{
		UE_LOG(LogYcGameplay, Error, TEXT("[UYcHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("YcHeroComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("YcHeroComponent");
			
			FMessageLog(HeroMessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
				
			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
	else
	{
		// 尽早向初始化状态系统进行注册，此操作仅当处于游戏世界时才会生效
		OwnerPawn = GetPawn<APawn>();
		RegisterInitStateFeature();
	}
}

void UYcHeroComponent::BeginPlay()
{
	Super::BeginPlay();
#if DEBUG_INIT_STATE
	UKismetSystemLibrary::PrintString(this, TEXT("YcHeroComponent-BeginPlay."));
#endif
	
	// 监听PawnExtension组件的初始化状态变更事件
	BindOnActorInitStateChanged(UYcPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);
	
	// 在接收到生成完成通知后，继续执行剩余初始化流程
	ensure(TryToChangeInitState(YcGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UYcHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void UYcHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn) return;
	
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = Cast<ULocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);
	
	
	if (const UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
		const UYcPawnData* PawnData = PawnExtComp->GetPawnData<UYcPawnData>()) 
	{
		if (const UYcInputConfig* InputConfig = PawnData->InputConfig)
		{
			// @TODO 添加组件配置的默认IMC功能
			// 绑定指定Tag对应的操作，内部通过指定Tag找到IA进行绑定
			// YcInputComponent提供了额外的函数将GameplayTag映射到InputAction
			// 如果想要此功能但要更改InputComponent类，请将其作为UYcInputComponent的子类
			UYcInputComponent* YcIC = Cast<UYcInputComponent>(PlayerInputComponent);
			if (ensureMsgf(YcIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to USCInputComponent or a subclass of it.")))
			{
				// 添加玩家可能设置的按键映射
				YcIC->AddInputMappings(InputConfig, Subsystem);

				// 在此处将InputAction绑定到GameplayTag，这意味着Gameplay Ability蓝图将直接由这些InputAction的Triggered事件触发
				TArray<uint32> BindHandles;
				YcIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

				YcIC->BindNativeAction(InputConfig, YcGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
				YcIC->BindNativeAction(InputConfig, YcGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
				YcIC->BindNativeAction(InputConfig, YcGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
				YcIC->BindNativeAction(InputConfig, YcGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
			}
		}
	}
	
	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}
	
	// 发送输入现在可绑定的扩展事件, GameFeatureAction可用监听此事件然后做出响应, 例如通过GameFeatureAction响应添加更多的输入绑定
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UYcHeroComponent::Input_AbilityInputTagPressed(const FGameplayTag InputTag)
{
#if DEBUG_INPUT_STATE
	UE_LOG(LogYcInput, Log, TEXT("[UYcHeroComponent::Input_AbilityInputTagPressed] 接收到输入标签按下: %s, 组件所有者: %s"), 
		*InputTag.ToString(), 
		*GetNameSafe(GetOwner()));
#endif
	
	if (!OwnerPawn) return;
	
	const UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(OwnerPawn);
	if (!PawnExtComp) return;
	
	if (UYcAbilitySystemComponent* ASC = PawnExtComp->GetYcAbilitySystemComponent())
	{
		ASC->AbilityInputTagPressed(InputTag);
	}
}

void UYcHeroComponent::Input_AbilityInputTagReleased(const FGameplayTag InputTag)
{
#if DEBUG_INPUT_STATE
	UE_LOG(LogYcInput, Log, TEXT("[UYcHeroComponent::Input_AbilityInputTagReleased] 接收到输入标签释放: %s, 组件所有者: %s"), 
		*InputTag.ToString(), 
		*GetNameSafe(GetOwner()));
#endif
	
	if (!OwnerPawn) return;

	const UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(OwnerPawn);
	if (!PawnExtComp) return;
	
	if (UYcAbilitySystemComponent* ASC = PawnExtComp->GetYcAbilitySystemComponent())
	{
		ASC->AbilityInputTagReleased(InputTag);
	}
}

void UYcHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	const AController* Controller = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	
	if (!Controller) return;
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();
	if(bFirstPersonMode)
	{
		OwnerPawn->AddMovementInput(OwnerPawn->GetActorForwardVector(), Value.Y);
		OwnerPawn->AddMovementInput(OwnerPawn->GetActorRightVector(), Value.X);
	}
	else
	{
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			OwnerPawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			OwnerPawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
	
	// 调用蓝图实现, 以便有让蓝图子类有机会根据输入做一些特殊效果, 例如移动时根据输入值做武器的摆动效果
	K2_OnMove(InputActionValue);
}

void UYcHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	if (!OwnerPawn) return;
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		OwnerPawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		OwnerPawn->AddControllerPitchInput(Value.Y);
	}
	
	// 调用蓝图实现, 以便有让蓝图子类有机会根据输入做一些特殊效果, 例如移动视角时根据输入值做武器的摆动效果
	K2_OnLookMouse(InputActionValue);
}

void UYcHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	if (!OwnerPawn) return;
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		OwnerPawn->AddControllerYawInput(Value.X * YcInput::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		OwnerPawn->AddControllerPitchInput(Value.Y * YcInput::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UYcHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	// @TODO 下蹲输入
}

UE_ENABLE_OPTIMIZATION
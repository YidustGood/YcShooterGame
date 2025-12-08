// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/Actions/YcGameFeatureAction_AddInputContextMapping.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Character/YcHeroComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Misc/DataValidation.h"
#include "System/YcAssetManager.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_AddInputContextMapping)

#define LOCTEXT_NAMESPACE "YiChenGameFeatures"

void UYcGameFeatureAction_AddInputContextMapping::OnGameFeatureRegistering()
{
	Super::OnGameFeatureRegistering();
	
	RegisterInputMappingContexts();
}

void UYcGameFeatureAction_AddInputContextMapping::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FPerContextData& ActiveData = ContextData.FindOrAdd(Context);
	if (!ensure(ActiveData.ExtensionRequestHandles.IsEmpty()) ||
		!ensure(ActiveData.LocalPlayersAddedTo.IsEmpty()))
	{
		Reset(ActiveData);
	}
	Super::OnGameFeatureActivating(Context);
}

void UYcGameFeatureAction_AddInputContextMapping::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	FPerContextData* ActiveData = ContextData.Find(Context);
	if (ensure(ActiveData))
	{
		Reset(*ActiveData);
	}
}

void UYcGameFeatureAction_AddInputContextMapping::OnGameFeatureUnregistering()
{
	Super::OnGameFeatureUnregistering();

	UnregisterInputMappingContexts();
}

#if WITH_EDITOR
EDataValidationResult UYcGameFeatureAction_AddInputContextMapping::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 Index = 0;

	for (const FYcInputMappingContextAndPriority& Entry : InputMappings)
	{
		if (Entry.InputMapping.IsNull())
		{
			Result = EDataValidationResult::Invalid;
			Context.AddError(FText::Format(LOCTEXT("NullInputMapping", "Null InputMapping at index {0}."), Index));
		}
		++Index;
	}

	return Result;
}
#endif

void UYcGameFeatureAction_AddInputContextMapping::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	// 检查World和GameInstance有效性
	if ((GameInstance == nullptr) || (World == nullptr) || !World->IsGameWorld()) return;
	UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (ComponentManager == nullptr) return;
	
	// 注册PlayerController扩展处理器，当PlayerController添加时触发输入映射添加
	auto AddAbilitiesDelegate =
			UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this, &ThisClass::HandleControllerExtension, ChangeContext);
	auto ExtensionRequestHandle =
		ComponentManager->AddExtensionHandler(APlayerController::StaticClass(), AddAbilitiesDelegate);

	// 保存扩展请求句柄以便后续清理
	ActiveData.ExtensionRequestHandles.Add(ExtensionRequestHandle);
}

void UYcGameFeatureAction_AddInputContextMapping::Reset(FPerContextData& ActiveData)
{
	ActiveData.ExtensionRequestHandles.Empty();

	while (!ActiveData.LocalPlayersAddedTo.IsEmpty())
	{
		TWeakObjectPtr<ULocalPlayer> LocalPlayerPtr = ActiveData.LocalPlayersAddedTo.Top();
		if (LocalPlayerPtr.IsValid())
		{
			RemoveInputMapping(LocalPlayerPtr.Get(), ActiveData);
		}
		else
		{
			ActiveData.LocalPlayersAddedTo.Pop();
		}
	}
}

void UYcGameFeatureAction_AddInputContextMapping::HandleControllerExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext)
{
	APlayerController* AsController = CastChecked<APlayerController>(Actor);
	ULocalPlayer* LocalPlayer = AsController ? Cast<ULocalPlayer>(AsController->GetLocalPlayer()) : nullptr;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);
	
	if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved) || (EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved))
	{
		RemoveInputMapping(LocalPlayer, ActiveData);
	}
	else if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded) || (EventName == UYcHeroComponent::NAME_BindInputsNow))
	{
		AddInputMappingForPlayer(LocalPlayer, ActiveData);
	}
}

void UYcGameFeatureAction_AddInputContextMapping::AddInputMappingForPlayer(ULocalPlayer* LocalPlayer, FPerContextData& ActiveData)
{
	UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (InputSystem == nullptr)
	{
		UE_LOG(LogGameFeatures, Error, TEXT("Failed to find `UEnhancedInputLocalPlayerSubsystem` for local player. Input mappings will not be added. Make sure you're set to use the EnhancedInput system via config file."));
		return;
	}
	
	for (const FYcInputMappingContextAndPriority& Entry : InputMappings)
	{
		if (const UInputMappingContext* IMC = Entry.InputMapping.Get())
		{
			InputSystem->AddMappingContext(IMC, Entry.Priority);
		}
	}
	ActiveData.LocalPlayersAddedTo.AddUnique(LocalPlayer);
}

void UYcGameFeatureAction_AddInputContextMapping::RemoveInputMapping(ULocalPlayer* LocalPlayer, FPerContextData& ActiveData)
{
	UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (InputSystem == nullptr) return;
	
	for (const FYcInputMappingContextAndPriority& Entry : InputMappings)
	{
		if (const UInputMappingContext* IMC = Entry.InputMapping.Get())
		{
			InputSystem->RemoveMappingContext(IMC);
		}
	}
	ActiveData.LocalPlayersAddedTo.Remove(LocalPlayer);
}

void UYcGameFeatureAction_AddInputContextMapping::RegisterInputMappingContexts()
{
	RegisterInputContextMappingsForGameInstanceHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::RegisterInputContextMappingsForGameInstance);

	// 处理多World实例注册IMC(在PIE模式下可能存在多个World实例)
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (auto WorldContextIterator = WorldContexts.CreateConstIterator(); WorldContextIterator; ++WorldContextIterator)
	{
		RegisterInputContextMappingsForGameInstance(WorldContextIterator->OwningGameInstance);
	}
}

void UYcGameFeatureAction_AddInputContextMapping::RegisterInputContextMappingsForGameInstance(UGameInstance* GameInstance)
{
	if (GameInstance == nullptr || GameInstance->OnLocalPlayerAddedEvent.IsBoundToObject(this)) return;
	// 监听GI的LocalPlayer添加/移除事件
	GameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &UYcGameFeatureAction_AddInputContextMapping::RegisterInputMappingContextsForLocalPlayer);
	GameInstance->OnLocalPlayerRemovedEvent.AddUObject(this, &UYcGameFeatureAction_AddInputContextMapping::UnregisterInputMappingContextsForLocalPlayer);

	//遍历处理所有LocalPlayer(例如双人成行那种本地分屏的双玩家就是两个LocalPlayer，每个LocalPlayer都需要单独注册IMC)
	for (auto LocalPlayerIterator = GameInstance->GetLocalPlayerIterator(); LocalPlayerIterator; ++LocalPlayerIterator)
	{
		RegisterInputMappingContextsForLocalPlayer(*LocalPlayerIterator);
	}
}

void UYcGameFeatureAction_AddInputContextMapping::RegisterInputMappingContextsForLocalPlayer(ULocalPlayer* LocalPlayer)
{
	if (!ensure(LocalPlayer)) return;
	
	UYcAssetManager& AssetManager = UYcAssetManager::Get();
	UEnhancedInputLocalPlayerSubsystem* EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	UEnhancedInputUserSettings* Settings = EISubsystem ? EISubsystem->GetUserSettings() : nullptr;
	if (Settings == nullptr) return;
	
	for (const FYcInputMappingContextAndPriority& Entry : InputMappings)
	{
		// 跳过不需要注册到设置的条目
		if (!Entry.bRegisterWithSettings) continue;
		
		// 将此IMC注册到设置系统，使其在用户按键设置中可见
		if (const UInputMappingContext* IMC = AssetManager.GetAsset(Entry.InputMapping))
		{
			Settings->RegisterInputMappingContext(IMC);
		}
	}
}

void UYcGameFeatureAction_AddInputContextMapping::UnregisterInputMappingContexts()
{
	FWorldDelegates::OnStartGameInstance.Remove(RegisterInputContextMappingsForGameInstanceHandle);
	RegisterInputContextMappingsForGameInstanceHandle.Reset();

	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (TIndirectArray<FWorldContext>::TConstIterator WorldContextIterator = WorldContexts.CreateConstIterator(); WorldContextIterator; ++WorldContextIterator)
	{
		UnregisterInputContextMappingsForGameInstance(WorldContextIterator->OwningGameInstance);
	}
}

void UYcGameFeatureAction_AddInputContextMapping::UnregisterInputContextMappingsForGameInstance(UGameInstance* GameInstance)
{
	if (GameInstance == nullptr) return;
	
	GameInstance->OnLocalPlayerAddedEvent.RemoveAll(this);
	GameInstance->OnLocalPlayerRemovedEvent.RemoveAll(this);
	for (TArray<ULocalPlayer*>::TConstIterator LocalPlayerIterator = GameInstance->GetLocalPlayerIterator(); LocalPlayerIterator; ++LocalPlayerIterator)
	{
		UnregisterInputMappingContextsForLocalPlayer(*LocalPlayerIterator);
	}
}

void UYcGameFeatureAction_AddInputContextMapping::UnregisterInputMappingContextsForLocalPlayer(ULocalPlayer* LocalPlayer)
{
	if (!ensure(LocalPlayer)) return;
	UEnhancedInputLocalPlayerSubsystem* EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	UEnhancedInputUserSettings* Settings = EISubsystem ? EISubsystem->GetUserSettings() : nullptr;
	if (Settings == nullptr) return;
	
	for (const FYcInputMappingContextAndPriority& Entry : InputMappings)
	{
		// 跳过不需要注册到设置的条目
		if (!Entry.bRegisterWithSettings) continue;

		// 从设置系统注销此IMC
		if (UInputMappingContext* IMC = Entry.InputMapping.Get())
		{
			Settings->UnregisterInputMappingContext(IMC);
		}
	}
}

#undef LOCTEXT_NAMESPACE
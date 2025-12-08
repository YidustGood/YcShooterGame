// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameFeatures/Actions/YcGameFeatureAction_AddInputConfigs.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "EnhancedInputSubsystems.h"
#include "GameFeaturesSubsystem.h"
#include "Character/YcHeroComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Input/YcInputConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_AddInputConfigs)

#define LOCTEXT_NAMESPACE "YiChenGameFeatures"

void UYcGameFeatureAction_AddInputConfigs::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	// 如存在与Context匹配的FPerContextData, 那么就清空, 不存在就添加到ContextData中
	FPerContextData& ActiveData = ContextData.FindOrAdd(Context);
	if (!ensure(ActiveData.ExtensionRequestHandles.IsEmpty()) ||
		!ensure(ActiveData.PawnsAddedTo.IsEmpty()))
	{
		Reset(ActiveData);
	}
	Super::OnGameFeatureActivating(Context);
}

void UYcGameFeatureAction_AddInputConfigs::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	// 清理匹配的FPerContextData
	FPerContextData* ActiveData = ContextData.Find(Context);

	if (ensure(ActiveData))
	{
		Reset(*ActiveData);
	}
}

#if WITH_EDITOR
EDataValidationResult UYcGameFeatureAction_AddInputConfigs::IsDataValid(FDataValidationContext& Context) const
{
	// InputConfigs有效性检查
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	int32 Index = 0;

	for (const TSoftObjectPtr<const UYcInputConfig>& Entry : InputConfigs)
	{
		if (!Entry.IsNull()) continue;
		
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::Format(LOCTEXT("NullInputConfig", "Null InputConfig at index {0}."), Index));
		++Index;
	}

	return Result;
}
#endif

void UYcGameFeatureAction_AddInputConfigs::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	const UWorld* World = WorldContext.World();
	const UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);
	
	// 确保GI、World有效且World是GameWorld, 因为编辑器下也会由一个编辑器World, 对于这个World我们不需要执行操作
	if (GameInstance == nullptr || World == nullptr || !World->IsGameWorld()) return;
	UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (ComponentManager == nullptr) return;
	
	// 创建一个扩展处理委托, 转发到HandlePawnExtension函数
	const auto AddAbilitiesDelegate =
		UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this, &ThisClass::HandlePawnExtension, ChangeContext);
	
	// 绑定委托到ComponentManager, 当收到Pawn类型对象的扩展事件时委托会被调用, 这样我们就能在对其进行扩展
	const TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle =
		ComponentManager->AddExtensionHandler(APawn::StaticClass(), AddAbilitiesDelegate);

	// 记录委托Handle, 以便在取消激活时清理
	ActiveData.ExtensionRequestHandles.Add(ExtensionRequestHandle);
}

void UYcGameFeatureAction_AddInputConfigs::Reset(FPerContextData& ActiveData)
{
	ActiveData.ExtensionRequestHandles.Empty();

	while (!ActiveData.PawnsAddedTo.IsEmpty())
	{
		TWeakObjectPtr<APawn> PawnPtr = ActiveData.PawnsAddedTo.Top();
		if (PawnPtr.IsValid())
		{
			RemoveInputConfig(PawnPtr.Get(), ActiveData);
		}
		else
		{
			ActiveData.PawnsAddedTo.Pop();
		}
	}
}

void UYcGameFeatureAction_AddInputConfigs::HandlePawnExtension(AActor* Actor, FName EventName,
	FGameFeatureStateChangeContext ChangeContext)
{
	APawn* AsPawn = CastChecked<APawn>(Actor);
	if (AsPawn == nullptr) return;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);
	// 根据传入的扩展事件名称判断是要移除还是添加
	if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved) || (EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved))
	{
		RemoveInputConfig(AsPawn, ActiveData);
	}
	else if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded) || (EventName == UYcHeroComponent::NAME_BindInputsNow))
	{
		AddInputConfigForPlayer(AsPawn, ActiveData);
	}
}

void UYcGameFeatureAction_AddInputConfigs::AddInputConfigForPlayer(APawn* Pawn, FPerContextData& ActiveData)
{
	const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	
	if (InputSystem == nullptr)
	{
		UE_LOG(LogGameFeatures, Error, TEXT("Failed to find `UEnhancedInputLocalPlayerSubsystem` for local player. Input mappings will not be added. Make sure you're set to use the EnhancedInput system via config file."));
		return;
	}
	UYcHeroComponent* HeroComponent = Pawn->FindComponentByClass<UYcHeroComponent>();
	
	// 检查Hero组件是否准备好添加输入绑定
	if (!HeroComponent || !HeroComponent->IsReadyToBindInputs()) return;
	
	// 遍历添加输入配置
	for (const TSoftObjectPtr<const UYcInputConfig>& Entry : InputConfigs)
	{
		if (const UYcInputConfig* Config = Entry.Get())
		{
			HeroComponent->AddAdditionalInputConfig(Config);
		}
	}
	
	ActiveData.PawnsAddedTo.AddUnique(Pawn);
}

void UYcGameFeatureAction_AddInputConfigs::RemoveInputConfig(APawn* Pawn, FPerContextData& ActiveData)
{
	const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UYcHeroComponent* HeroComponent = Pawn->FindComponentByClass<UYcHeroComponent>();
	if (!HeroComponent) return;
	
	// 遍历移除输入配置
	for (const TSoftObjectPtr<const UYcInputConfig>& Entry : InputConfigs)
	{
		if (const UYcInputConfig* InputConfig = Entry.Get())
		{
			HeroComponent->RemoveAdditionalInputConfig(InputConfig);
		}
	}
	
	ActiveData.PawnsAddedTo.Remove(Pawn); // 移除这个Pawn的记录
}

#undef LOCTEXT_NAMESPACE

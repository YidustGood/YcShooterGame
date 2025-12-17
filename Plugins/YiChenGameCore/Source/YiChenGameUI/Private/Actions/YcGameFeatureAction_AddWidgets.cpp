// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Actions/YcGameFeatureAction_AddWidgets.h"

#include "CommonActivatableWidget.h"
#include "CommonUIExtensions.h"
#include "GameFeaturesSubsystemSettings.h"
#include "YcGameHUD.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Misc/DataValidation.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameFeatureAction_AddWidgets)

#define LOCTEXT_NAMESPACE "YiChenGameFeatures"

void UYcGameFeatureAction_AddWidgets::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	// 查找当前上下文的激活数据
	FPerContextData* ActiveData = ContextData.Find(Context);
	if ensure(ActiveData)
	{
		// 重置数据，移除已添加的 Widgets 和扩展
		Reset(*ActiveData);
	}
}

#if WITH_EDITORONLY_DATA
void UYcGameFeatureAction_AddWidgets::AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData)
{
	// 将 Widgets 的资源包设置为客户端加载，这样它们只会在客户端被加载。
	for (const FYcHUDElementEntry& Entry : Widgets)
	{
		AssetBundleData.AddBundleAsset(UGameFeaturesSubsystemSettings::LoadStateClient, Entry.WidgetClass.ToSoftObjectPath().GetAssetPath());
	}
}
#endif

#if WITH_EDITOR
EDataValidationResult UYcGameFeatureAction_AddWidgets::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	// 验证布局请求列表
	{
		int32 EntryIndex = 0;
		for (const FYcHUDLayoutRequest& Entry : Layout)
		{
			// 验证布局类是否为空
			if (Entry.LayoutClass.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("LayoutHasNullClass", "Null WidgetClass at index {0} in Layout"), FText::AsNumber(EntryIndex)));
			}

			// 验证层级ID是否有效
			if (!Entry.LayerID.IsValid())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("LayoutHasNoTag", "LayerID is not set at index {0} in Widgets"), FText::AsNumber(EntryIndex)));
			}

			++EntryIndex;
		}
	}

	// 验证 Widget 条目列表
	{
		int32 EntryIndex = 0;
		for (const FYcHUDElementEntry& Entry : Widgets)
		{
			// 验证 Widget 类是否为空
			if (Entry.WidgetClass.IsNull())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNullClass", "Null WidgetClass at index {0} in Widgets"), FText::AsNumber(EntryIndex)));
			}

			// 验证插槽ID是否有效
			if (!Entry.SlotID.IsValid())
			{
				Result = EDataValidationResult::Invalid;
				Context.AddError(FText::Format(LOCTEXT("EntryHasNoTag", "SlotID is not set at index {0} in Widgets"), FText::AsNumber(EntryIndex)));
			}
			++EntryIndex;
		}
	}

	return Result;
}
#endif

void UYcGameFeatureAction_AddWidgets::AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext)
{
	UWorld* World = WorldContext.World();
	UGameInstance* GameInstance = WorldContext.OwningGameInstance;
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	// 验证世界和游戏实例的有效性，确保我们处于一个有效的游戏世界中
	if ((GameInstance == nullptr) || (World == nullptr) || !World->IsGameWorld()) return;
	
	UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(GameInstance);
	if (ComponentManager == nullptr) return;

	// 定义要监听的目标 Actor 类型
	TSoftClassPtr<AActor> HUDActorClass = AYcGameHUD::StaticClass();

	// 添加一个扩展处理器，当指定类型的 Actor (AYcGameHUD) 被添加到世界时，会调用 HandleActorExtension
	TSharedPtr<FComponentRequestHandle> ExtensionRequestHandle = ComponentManager->AddExtensionHandler(
		HUDActorClass,
		UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(this, &ThisClass::HandleActorExtension, ChangeContext));
	ActiveData.ComponentRequests.Add(ExtensionRequestHandle);
}

void UYcGameFeatureAction_AddWidgets::HandleActorExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext)
{
	FPerContextData& ActiveData = ContextData.FindOrAdd(ChangeContext);

	// 当 Actor 的扩展被移除或接收者被移除时，移除 Widgets
	if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved) || (EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved))
	{
		RemoveWidgets(Actor, ActiveData);
	}
	// 当 Actor 的扩展被添加或 Actor 准备就绪时，添加 Widgets
	else if ((EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded) || (EventName == UGameFrameworkComponentManager::NAME_GameActorReady))
	{
		AddWidgets(Actor, ActiveData);
	}
}

void UYcGameFeatureAction_AddWidgets::AddWidgets(AActor* Actor, FPerContextData& ActiveData)
{
	AYcGameHUD* HUD = CastChecked<AYcGameHUD>(Actor);
	ULocalPlayer* LocalPlayer = HUD ? Cast<ULocalPlayer>(HUD->GetOwningPlayerController()->Player): nullptr;

	if (LocalPlayer == nullptr) return;
	
	FPerActorData& ActorData = ActiveData.ActorData.FindOrAdd(HUD);

	// 添加布局
	for (const FYcHUDLayoutRequest& Entry : Layout)
	{
		if (TSubclassOf<UCommonActivatableWidget> ConcreteWidgetClass = Entry.LayoutClass.Get())
		{
			ActorData.LayoutsAdded.Add(UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, Entry.LayerID, ConcreteWidgetClass));
		}
	}

	// 添加 Widgets
	UUIExtensionSubsystem* ExtensionSubsystem = HUD->GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
	for (const FYcHUDElementEntry& Entry : Widgets)
	{
		ActorData.ExtensionHandles.Add(ExtensionSubsystem->RegisterExtensionAsWidgetForContext(Entry.SlotID, LocalPlayer, Entry.WidgetClass.Get(), -1));
	}
}

void UYcGameFeatureAction_AddWidgets::RemoveWidgets(AActor* Actor, FPerContextData& ActiveData)
{
	AYcGameHUD* HUD = CastChecked<AYcGameHUD>(Actor);

	// 仅当这是同一个已注册的 HUD Actor 时才取消注册，因为客户端上可能同时存在多个活跃的 HUD Actor
	FPerActorData* ActorData = ActiveData.ActorData.Find(HUD);

	if (ActorData == nullptr) return;
	
	// 停用并移除布局
	for (TWeakObjectPtr<UCommonActivatableWidget>& AddedLayout : ActorData->LayoutsAdded)
	{
		if (AddedLayout.IsValid())
		{
			AddedLayout->DeactivateWidget();
		}
	}

	// 注销 UI 扩展
	for (FUIExtensionHandle& Handle : ActorData->ExtensionHandles)
	{
		Handle.Unregister();
	}

	// 从激活数据中移除此 Actor 的数据
	ActiveData.ActorData.Remove(HUD);
}

void UYcGameFeatureAction_AddWidgets::Reset(FPerContextData& ActiveData)
{
	// 清空组件请求句柄
	ActiveData.ComponentRequests.Empty();

	// 遍历所有 Actor 数据并注销其 UI 扩展句柄
	for (TPair<FObjectKey, FPerActorData>& Pair : ActiveData.ActorData)
	{
		for (FUIExtensionHandle& Handle : Pair.Value.ExtensionHandles)
		{
			Handle.Unregister();
		}
	}
	// 清空 Actor 数据映射
	ActiveData.ActorData.Empty();
}

#undef LOCTEXT_NAMESPACE
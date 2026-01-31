// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/YcInteractableComponent.h"

#include "UIExtensionSystem.h"
#include "Blueprint/UserWidget.h"
#include "Interaction/InteractionWidgetInterface.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInteractableComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_HUD_Slot_Interaction, "HUD.Slot.Interaction");

UYcInteractableComponent::UYcInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UYcInteractableComponent::GatherInteractionOptions(const FYcInteractionQuery& InteractQuery,
	FYcInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

void UYcInteractableComponent::UpdateInteractionOption(const FYcInteractionOption& NewOption)
{
	Option = NewOption;
}

void UYcInteractableComponent::OnPlayerFocusBegin(const FYcInteractionQuery& InteractQuery)
{
	check(GetWorld());
	
	auto* PC = Cast<APlayerController>(InteractQuery.RequestingController);
	auto* LPC = PC?  PC->GetLocalPlayer() : nullptr;
	UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
	if (LPC && ExtensionSubsystem)
	{
		// 在下一帧执行UI创建, 详细原因请查查看UYcGameplayAbility_Interact::UpdateInteractions()中的逻辑和描述
		GetWorld()->GetTimerManager().SetTimerForNextTick([this, LPC,ExtensionSubsystem]()
		{
			UClass* WidgetClass = Option.InteractionWidgetClass.Get();
			if (!WidgetClass)
			{
				// @TODO 这里交互Widget的加载是同步加载, 一般来说它的体量很小, 不会带来明显卡顿, 如果追求更优可以在这里进行异步加载
				// 如果WidgetClass无效则先进行加载
				WidgetClass = Option.InteractionWidgetClass.LoadSynchronous(); 
			}
			const FGameplayTag ExtensionPointTag = Option.WidgetExtensionPointTag.IsValid() ? Option.WidgetExtensionPointTag : TAG_HUD_Slot_Interaction;
			
			// 添加交互提示Widget
			InteractionWidgetHandle = ExtensionSubsystem->RegisterExtensionAsWidgetForContext(ExtensionPointTag, LPC, WidgetClass, -1);
			
			// 通过接口传递交互组件, 以便UI能够获得交互目标的信息用于显示
			if (InteractionWidgetHandle.IsValid() &&
				InteractionWidgetHandle.GetWidgetInstance() &&
				InteractionWidgetHandle.GetWidgetInstance()->Implements<UInteractionWidgetInterface>())
			{
				IInteractionWidgetInterface::Execute_BindInteractableComponent(InteractionWidgetHandle.GetWidgetInstance(), this);
			}
		});
	}

	OnPlayerFocusBeginEvent.Broadcast(InteractQuery);
}

void UYcInteractableComponent::OnPlayerFocusEnd(const FYcInteractionQuery& InteractQuery)
{
	// 移除交互提示Widget
	if (!InteractionWidgetHandle.IsValid())
	{
		OnPlayerFocusEndEvent.Broadcast(InteractQuery);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		OnPlayerFocusEndEvent.Broadcast(InteractQuery);
		return;
	}

	UUIExtensionSubsystem* ExtensionSubsystem = World->GetSubsystem<UUIExtensionSubsystem>();
	if (!ExtensionSubsystem)
	{
		OnPlayerFocusEndEvent.Broadcast(InteractQuery);
		return;
	}

	// 如果Widget实例存在，先解绑接口
	if (UUserWidget* WidgetInstance = InteractionWidgetHandle.GetWidgetInstance())
	{
		if (WidgetInstance->Implements<UInteractionWidgetInterface>())
		{
			IInteractionWidgetInterface::Execute_UnbindInteractableComponent(WidgetInstance, this);
		}
	}

	// 无论Widget实例是否存在，都需要注销扩展以清理资源
	ExtensionSubsystem->UnregisterExtension(InteractionWidgetHandle);
	
	OnPlayerFocusEndEvent.Broadcast(InteractQuery);
}
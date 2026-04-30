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
	// 当配置为“不可交互且隐藏提示”时，不向外提供交互项
	if (Option.ShouldShowPrompt())
	{
		InteractionBuilder.AddInteractionOption(Option);
	}
}

void UYcInteractableComponent::UpdateInteractionOption(const FYcInteractionOption& NewOption)
{
	// 如果UI类或挂载点发生变化，说明现有Widget需要重建
	const bool bNeedRecreateWidget = Option.InteractionWidgetClass != NewOption.InteractionWidgetClass ||
		Option.WidgetExtensionPointTag != NewOption.WidgetExtensionPointTag;

	Option = NewOption;

	// 运行时更新配置时，如果玩家当前正在聚焦该物体，需要立即刷新交互提示
	RefreshInteractionWidget(bNeedRecreateWidget);
}

void UYcInteractableComponent::SetInteractionOption(const FYcInteractionOption& NewOption)
{
	UpdateInteractionOption(NewOption);
}

FYcInteractionOption UYcInteractableComponent::GetInteractionOption() const
{
	return Option;
}

void UYcInteractableComponent::SetInteractionEnabled(bool bNewEnabled)
{
	if (Option.bCanInteract == bNewEnabled)
	{
		return;
	}

	Option.bCanInteract = bNewEnabled;

	// 支持在运行时动态切换可交互状态，并立即刷新当前提示UI
	RefreshInteractionWidget();
}

void UYcInteractableComponent::SetDisabledDisplayPolicy(EYcInteractionDisabledDisplayPolicy NewPolicy)
{
	if (Option.DisabledDisplayPolicy == NewPolicy)
	{
		return;
	}

	Option.DisabledDisplayPolicy = NewPolicy;

	// 支持在运行时动态切换“隐藏提示 / 显示禁用提示”的表现
	RefreshInteractionWidget();
}

bool UYcInteractableComponent::IsInteractionEnabled() const
{
	return Option.IsInteractable();
}

bool UYcInteractableComponent::ShouldShowInteractionPrompt() const
{
	return Option.ShouldShowPrompt();
}

void UYcInteractableComponent::OnPlayerFocusBegin(const FYcInteractionQuery& InteractQuery)
{
	bIsFocused = true;
	LastFocusQuery = InteractQuery;

	// 玩家开始聚焦后尝试显示交互提示，如果当前状态不允许显示则内部会直接跳过
	RefreshInteractionWidget();

	OnPlayerFocusBeginEvent.Broadcast(InteractQuery);
}

void UYcInteractableComponent::OnPlayerFocusEnd(const FYcInteractionQuery& InteractQuery)
{
	bIsFocused = false;

	// 移除交互提示Widget
	UnregisterInteractionWidget();
	
	OnPlayerFocusEndEvent.Broadcast(InteractQuery);
}

void UYcInteractableComponent::RefreshInteractionWidget(bool bForceRecreate)
{
	// 只有在玩家当前聚焦该物体时，才需要刷新交互提示UI
	if (!bIsFocused)
	{
		if (bForceRecreate)
		{
			UnregisterInteractionWidget();
		}
		return;
	}

	// 当前配置要求不显示提示时，确保已有Widget被移除
	if (!Option.ShouldShowPrompt())
	{
		UnregisterInteractionWidget();
		return;
	}

	if (bForceRecreate)
	{
		UnregisterInteractionWidget();
	}

	if (!InteractionWidgetHandle.IsValid())
	{
		// 在下一帧执行UI创建, 详细原因请查查看UYcGameplayAbility_Interact::UpdateInteractions()中的逻辑和描述
		RegisterInteractionWidget();
		return;
	}

	// 如果Widget已经存在，则通过接口重新绑定组件，让UI主动拉取最新状态
	if (InteractionWidgetHandle.GetWidgetInstance() &&
		InteractionWidgetHandle.GetWidgetInstance()->Implements<UInteractionWidgetInterface>())
	{
		IInteractionWidgetInterface::Execute_BindInteractableComponent(InteractionWidgetHandle.GetWidgetInstance(), this);
	}
}

void UYcInteractableComponent::RegisterInteractionWidget()
{
	check(GetWorld());
	
	auto* PC = Cast<APlayerController>(LastFocusQuery.RequestingController);
	auto* LPC = PC ? PC->GetLocalPlayer() : nullptr;
	UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
	if (LPC && ExtensionSubsystem)
	{
		// 在下一帧执行UI创建, 详细原因请查看看UYcGameplayAbility_Interact::UpdateInteractions()中的逻辑和描述
		GetWorld()->GetTimerManager().SetTimerForNextTick([this, LPC, ExtensionSubsystem]()
		{
			// 下一帧真正创建前再次校验状态，避免这期间玩家已经失焦或状态又变化
			if (!IsValid(this) || !bIsFocused || !Option.ShouldShowPrompt() || InteractionWidgetHandle.IsValid())
			{
				return;
			}

			UClass* WidgetClass = Option.InteractionWidgetClass.Get();
			if (!WidgetClass)
			{
				// @TODO 这里交互Widget的加载是同步加载, 一般来说它的体量很小, 不会带来明显卡顿, 如果追求更优可以在这里进行异步加载
				// 如果WidgetClass无效则先进行加载
				WidgetClass = Option.InteractionWidgetClass.LoadSynchronous(); 
			}
			if (!WidgetClass)
			{
				return;
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
}

void UYcInteractableComponent::UnregisterInteractionWidget()
{
	// 移除交互提示Widget
	if (!InteractionWidgetHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UUIExtensionSubsystem* ExtensionSubsystem = World->GetSubsystem<UUIExtensionSubsystem>();
	if (!ExtensionSubsystem)
	{
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
	InteractionWidgetHandle = FUIExtensionHandle();
}

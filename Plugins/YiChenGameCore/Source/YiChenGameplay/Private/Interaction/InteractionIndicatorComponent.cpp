// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/InteractionIndicatorComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionIndicatorComponent)

void FIndicatorDescriptor::SetIndicatorManagerComponent(UInteractionIndicatorComponent* InManager)
{
	// 确保没有设置过
	if (ensure(ManagerPtr.IsExplicitlyNull()))
	{
		ManagerPtr = InManager;
	}
}

UInteractionIndicatorComponent::UInteractionIndicatorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

UInteractionIndicatorComponent* UInteractionIndicatorComponent::GetComponent(const AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<UInteractionIndicatorComponent>();
	}

	return nullptr;
}

void UInteractionIndicatorComponent::AddIndicator(FIndicatorDescriptor IndicatorDescriptor)
{
	auto& Ref = Indicators.Add_GetRef(IndicatorDescriptor);
	Ref.SetIndicatorManagerComponent(this);
}

bool UInteractionIndicatorComponent::GetFirstIndicator(FIndicatorDescriptor& InOutIndicator)
{
	if (Indicators.IsEmpty()) return false;
	InOutIndicator = Indicators[0];
	return true;
}

void UInteractionIndicatorComponent::EmptyIndicators()
{
	Indicators.Empty();
}
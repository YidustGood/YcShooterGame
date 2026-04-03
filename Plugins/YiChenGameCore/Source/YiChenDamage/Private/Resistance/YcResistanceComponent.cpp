// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Resistance/YcResistanceComponent.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcResistanceComponent)

UYcResistanceComponent::UYcResistanceComponent()
{
	SetIsReplicatedByDefault(true);
}

void UYcResistanceComponent::BeginPlay()
{
	Super::BeginPlay();

	// 应用配置到容器
	ResistanceContainer.MaxResistance = MaxResistance;
	ResistanceContainer.MinResistance = MinResistance;
}

void UYcResistanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UYcResistanceComponent, ResistanceContainer);
}

void UYcResistanceComponent::SetResistance(FGameplayTag Tag, float Value)
{
	ResistanceContainer.SetResistance(Tag, Value);
}

void UYcResistanceComponent::AddResistance(FGameplayTag Tag, float Delta)
{
	ResistanceContainer.AddResistance(Tag, Delta);
}

void UYcResistanceComponent::RemoveResistance(FGameplayTag Tag)
{
	ResistanceContainer.RemoveResistance(Tag);
}

float UYcResistanceComponent::GetResistance(FGameplayTag Tag) const
{
	return ResistanceContainer.GetResistance(Tag);
}

bool UYcResistanceComponent::HasResistance(FGameplayTag Tag) const
{
	return ResistanceContainer.ContainsResistance(Tag);
}

TArray<FGameplayTag> UYcResistanceComponent::GetAllResistanceTags() const
{
	return ResistanceContainer.GetAllResistanceTags();
}

void UYcResistanceComponent::ClearAllResistances()
{
	ResistanceContainer.ClearAllResistances();
}

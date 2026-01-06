// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentActorComponent.h"
#include "YcEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentActorComponent)

UYcEquipmentActorComponent::UYcEquipmentActorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UYcEquipmentActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 只在初始化时同步一次，后续不会修改
	DOREPLIFETIME_CONDITION(UYcEquipmentActorComponent, EquipmentInst, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(UYcEquipmentActorComponent, EquipmentTags, COND_InitialOnly);
}

UYcEquipmentInstance* UYcEquipmentActorComponent::GetEquipmentFromActor(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	
	if (UYcEquipmentActorComponent* Comp = Actor->FindComponentByClass<UYcEquipmentActorComponent>())
	{
		return Comp->GetOwningEquipment();
	}
	return nullptr;
}

void UYcEquipmentActorComponent::OnRep_EquipmentInst()
{
	/**
	 * 对于客户端来说，Equipment所附加的Actors同步的在客户端创建后并且EquipmentInst同步到位
	 * 发现是Unequipped状态就做隐藏以及关闭tick、碰撞, 至于Actor的网络休眠是服务器在做生成时就能设置，客户端无权也无需处理网络休眠的问题
	 * 
	 * 改进说明：
	 * 原本是放在FYcEquipmentList::PostReplicatedAdd()进行的, 但是这里不能确保在客户端执行时这个客户端已经完成Equipment所需附加Actors的同步创建
	 * 这就会导致可能无法正确的将Unequipped状态的Equipment的Actors隐藏, 所以我们就把客户端处理未装备时Actors的隐藏挪动到这里了, 这个组件是挂载在Actor上的
	 * 所以当OnRep_EquipmentInst触发时Actor一定是完成了同步创建的, 就能确保正确处理客户端装备附加Actors的Unequipped状态隐藏问题, 这样时序有保障、职责也清晰
	 * 
	 * 补充Actor显示隐藏方式选择原因：
	 * 之所以不直接用Actor的bHidden开控制显隐藏是因为它是网络复制的, 在为主控客户端做连续多次预测时bHidden的同步会影响预测表现, 导致Actor出现多余的显隐问题
	 * 所以我们采用了服务端/客户端通过特定的属性同步通知来独立控制显隐的方案, 这也可以正确的保持多端同步
	 */
	if (!GetOwner()->HasAuthority() && EquipmentInst && !EquipmentInst->IsEquipped())
	{
		// 如果所属的装备实例处于Unequipped状态就隐藏
		UYcEquipmentInstance::SetActorVisualVisibility(GetOwner(), false);
		// 关闭Tick
		GetOwner()->SetActorTickEnabled(false);
		// 关闭碰撞
		GetOwner()->SetActorEnableCollision(false);
	}
	OnEquipmentRep.Broadcast(GetOwningEquipment());
}
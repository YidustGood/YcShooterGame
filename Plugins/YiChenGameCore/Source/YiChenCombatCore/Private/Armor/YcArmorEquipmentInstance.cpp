// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Armor/YcArmorEquipmentInstance.h"
#include "Armor/InventoryFragment_ArmorStats.h"
#include "Armor/YcArmorMessage.h"
#include "YcInventoryItemInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "YcAbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Armor/YcArmorSet.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UYcArmorEquipmentInstance::UYcArmorEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UYcArmorEquipmentInstance::OnEquipmentInstanceCreated(const FYcEquipmentDefinition& Definition)
{
	Super::OnEquipmentInstanceCreated(Definition);

	// 缓存护甲静态数据
	ArmorStats = GetArmorStatsFragment();

	// 从 Fragment 获取护甲标签
	if (ArmorStats && ArmorStats->ArmorTag.IsValid())
	{
		ArmorTag = ArmorStats->ArmorTag;
	}
}

void UYcArmorEquipmentInstance::OnEquipped()
{
	Super::OnEquipped();

	// 装备时创建并应用护甲属性集到 GAS
	ApplyArmorAttribute();

	// 绑定护甲属性变化监听（实时写回耐久度）
	BindArmorChangeDelegate();

	// 广播护甲装备消息（NewValue 为当前耐久度）
	const float CurrentDurability = GetCurrentDurability();
	BroadcastArmorMessage(TAG_MESSAGE_ARMOR_EQUIPPED, 0.0f, CurrentDurability);
}

void UYcArmorEquipmentInstance::OnUnequipped()
{
	// 广播护甲卸下消息（OldValue 为当前耐久度，NewValue 为 0）
	const float CurrentDurability = GetCurrentDurability();
	BroadcastArmorMessage(TAG_MESSAGE_ARMOR_UNEQUIPPED, CurrentDurability, 0.0f);

	// 解绑护甲属性变化监听
	UnbindArmorChangeDelegate();

	// 卸下时保存耐久度到 ItemInstance，并移除护甲属性集
	RemoveArmorAttribute();

	Super::OnUnequipped();
}

const FInventoryFragment_ArmorStats* UYcArmorEquipmentInstance::GetArmorStatsFragment() const
{
	if (ArmorStats)
	{
		return ArmorStats;
	}

	// 从关联的 ItemInstance 获取 Fragment
	if (UYcInventoryItemInstance* ItemInst = GetAssociatedItem())
	{
		return ItemInst->GetTypedFragment<FInventoryFragment_ArmorStats>();
	}

	return nullptr;
}

float UYcArmorEquipmentInstance::GetCurrentDurability() const
{
	return FInventoryFragment_ArmorStats::GetCurrentDurability(GetAssociatedItem());
}

void UYcArmorEquipmentInstance::SetCurrentDurability(float Value)
{
	FInventoryFragment_ArmorStats::SetCurrentDurability(GetAssociatedItem(), Value);
}

bool UYcArmorEquipmentInstance::IsBroken() const
{
	return FInventoryFragment_ArmorStats::IsBroken(GetAssociatedItem());
}

void UYcArmorEquipmentInstance::ApplyArmorAttribute()
{
	UYcAbilitySystemComponent* YcASC = GetOwnerYcASC();
	if (!YcASC || !ArmorStats)
	{
		return;
	}

	// 获取当前耐久度
	float CurrentDurability = GetCurrentDurability();
	if (CurrentDurability <= 0.0f)
	{
		// 护甲已损坏，不应用属性
		return;
	}

	// 确保护甲标签有效
	if (!ArmorTag.IsValid())
	{
		// 如果没有指定标签，使用默认标签
		ArmorTag = FGameplayTag::RequestGameplayTag(TEXT("Armor.Default"));
	}

	// 检查是否已存在该标签的护甲属性集
	if (UYcArmorSet* ExistingArmorSet = Cast<UYcArmorSet>(YcASC->GetAttributeSetByTag(ArmorTag)))
	{
		// 已存在，更新属性
		ExistingArmorSet->InitMaxArmor(ArmorStats->MaxDurability);
		ExistingArmorSet->InitArmor(CurrentDurability);
		ExistingArmorSet->InitArmorAbsorption(ArmorStats->ArmorAbsorption);
		CachedArmorSet = ExistingArmorSet;
	}
	else
	{
		// 创建新的护甲属性集
		if (UYcArmorSet* NewArmorSet = Cast<UYcArmorSet>(YcASC->AddAttributeSet(UYcArmorSet::StaticClass(), ArmorTag)))
		{
			// 初始化属性
			NewArmorSet->InitMaxArmor(ArmorStats->MaxDurability);
			NewArmorSet->InitArmor(CurrentDurability);
			NewArmorSet->InitArmorAbsorption(ArmorStats->ArmorAbsorption);
			CachedArmorSet = NewArmorSet;
		}
	}
}

void UYcArmorEquipmentInstance::RemoveArmorAttribute()
{
	UYcAbilitySystemComponent* YcASC = GetOwnerYcASC();
	if (!YcASC)
	{
		return;
	}

	// 从 ASC 移除护甲属性集
	YcASC->RemoveAttributeSetByTag(ArmorTag);

	CachedArmorSet.Reset();
}

UYcAbilitySystemComponent* UYcArmorEquipmentInstance::GetOwnerYcASC() const
{
	APawn* Pawn = GetPawn();
	if (!Pawn)
	{
		return nullptr;
	}

	// 通过接口获取 ASC
	if (Pawn->Implements<UAbilitySystemInterface>())
	{
		return Cast<UYcAbilitySystemComponent>(Cast<IAbilitySystemInterface>(Pawn)->GetAbilitySystemComponent());
	}

	// 尝试从 Actor 上查找组件
	return Pawn->FindComponentByClass<UYcAbilitySystemComponent>();
}

void UYcArmorEquipmentInstance::BindArmorChangeDelegate()
{
	// 获取护甲属性集
	const UYcArmorSet* ArmorSet = CachedArmorSet.Get();
	if (!ArmorSet)
	{
		// 尝试从 ASC 获取
		if (UYcAbilitySystemComponent* YcASC = GetOwnerYcASC())
		{
			ArmorSet = Cast<UYcArmorSet>(YcASC->GetAttributeSetByTag(ArmorTag));
		}
	}

	if (!ArmorSet)
	{
		return;
	}

	// 缓存护甲属性集引用
	CachedArmorSet = ArmorSet;

	// 绑定护甲耐久度变化委托
	ArmorSet->OnArmorChanged.AddUObject(this, &ThisClass::OnArmorAttributeChanged);
}

void UYcArmorEquipmentInstance::UnbindArmorChangeDelegate()
{
	// 解绑护甲耐久度变化委托
	if (const UYcArmorSet* ArmorSet = CachedArmorSet.Get())
	{
		ArmorSet->OnArmorChanged.RemoveAll(this);
	}

	CachedArmorSet.Reset();
}

void UYcArmorEquipmentInstance::OnArmorAttributeChanged(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue)
{
	// 仅在权威端执行写回（服务端或独立模式）
	if (APawn* Pawn = GetPawn())
	{
		if (!Pawn->HasAuthority())
		{
			return;
		}
	}

	// 实时写回耐久度到 ItemInstance
	SetCurrentDurability(NewValue);

	// 广播护甲耐久度变化消息
	BroadcastArmorMessage(TAG_MESSAGE_ARMOR_CHANGED, OldValue, NewValue);

	// 如果之前有护甲但现在归零了，广播护甲破碎消息
	if (OldValue > 0.0f && NewValue <= 0.0f)
	{
		BroadcastArmorMessage(TAG_MESSAGE_ARMOR_BROKEN, OldValue, NewValue);
	}
}

void UYcArmorEquipmentInstance::BroadcastArmorMessage(FGameplayTag MessageTag, float OldValue, float NewValue)
{
	APawn* Pawn = GetPawn();
	if (!Pawn)
	{
		return;
	}

	// 构造护甲消息
	FYcArmorMessage Message;
	Message.TargetActor = Pawn;
	Message.ItemInstance = GetAssociatedItem();
	Message.ArmorTag = ArmorTag;
	Message.OldValue = OldValue;
	Message.NewValue = NewValue;
	Message.MaxArmor = ArmorStats ? ArmorStats->MaxDurability : 0.0f;

	// 广播消息
	UGameplayMessageSubsystem::Get(Pawn).BroadcastMessage(MessageTag, Message);
}

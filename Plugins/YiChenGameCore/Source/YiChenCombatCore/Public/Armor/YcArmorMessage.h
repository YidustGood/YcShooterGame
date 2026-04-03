// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "YcArmorMessage.generated.h"

class UYcInventoryItemInstance;

/** 护甲变化消息标签 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MESSAGE_ARMOR_CHANGED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MESSAGE_ARMOR_BROKEN);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MESSAGE_ARMOR_EQUIPPED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MESSAGE_ARMOR_UNEQUIPPED);

/**
 * 护甲变化消息
 * 用于 GameplayMessageSubsystem 广播护甲值变化
 */
USTRUCT(BlueprintType)
struct YICHENCOMBATCORE_API FYcArmorMessage
{
	GENERATED_BODY()

	/** 护甲所属 Actor */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 护甲物品实例（用于区分多个部位的护甲） */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	/** 护甲部位标签（如 Armor.Head、Armor.Body） */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag ArmorTag;

	/** 变化前的护甲值 */
	UPROPERTY(BlueprintReadOnly)
	float OldValue = 0.0f;

	/** 变化后的护甲值 */
	UPROPERTY(BlueprintReadOnly)
	float NewValue = 0.0f;

	/** 最大护甲值（耐久度上限） */
	UPROPERTY(BlueprintReadOnly)
	float MaxArmor = 0.0f;
};

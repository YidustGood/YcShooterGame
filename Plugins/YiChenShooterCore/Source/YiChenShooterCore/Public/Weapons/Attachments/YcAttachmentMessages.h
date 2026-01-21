// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "DataRegistryId.h"
#include "NativeGameplayTags.h"
#include "YcAttachmentMessages.generated.h"

class AYcWeaponActor;
class APawn;

// ════════════════════════════════════════════════════════════════════════════
// GameplayMessage Tags - 配件系统消息标签
// ════════════════════════════════════════════════════════════════════════════

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Attachment_Installed);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Attachment_Uninstalled);

// ════════════════════════════════════════════════════════════════════════════
// 消息结构体
// ════════════════════════════════════════════════════════════════════════════

/**
 * FYcAttachmentChangedMessage - 配件变化消息
 * 
 * 用于 GameplayMessageSubsystem 广播配件安装/卸载事件。
 * UI 和其他解耦系统可以监听此消息，无需直接引用武器组件。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentChangedMessage
{
	GENERATED_BODY()

	/** 武器Actor弱引用 */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	TWeakObjectPtr<AYcWeaponActor> WeaponActor;

	/** 武器拥有者Pawn弱引用 */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	TWeakObjectPtr<APawn> OwnerPawn;

	/** 变化的槽位类型 */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	FGameplayTag SlotType;

	/** 配件 DataRegistry ID */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	FDataRegistryId AttachmentId;
};

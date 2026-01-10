// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fragments/YcEquipmentFragment.h"
#include "YcAttachmentTypes.h"
#include "YcFragment_WeaponAttachments.generated.h"

/**
 * FYcFragment_WeaponAttachments - 武器配件系统配置Fragment
 * 
 * 添加到武器的EquipmentDefinition中，定义该武器支持的配件槽位。
 * 
 * 使用方式：
 * 1. 在武器的EquipmentDefinition中添加此Fragment
 * 2. 配置AttachmentSlots定义武器支持的槽位
 * 3. 每个槽位配置可用的配件列表
 * 4. 运行时通过UYcWeaponAttachmentSubsystem管理配件安装
 */
USTRUCT(BlueprintType, meta=(DisplayName="武器配件配置"))
struct YICHENSHOOTERCORE_API FYcFragment_WeaponAttachments : public FYcEquipmentFragment
{
	GENERATED_BODY()

	FYcFragment_WeaponAttachments();

	/** 武器支持的配件槽位列表 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="配件槽位", 
		meta=(TitleProperty="DisplayName"))
	TArray<FYcAttachmentSlotDef> AttachmentSlots;

	/** 最大可安装配件数量（0=无限制） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="限制")
	int32 MaxAttachments = 5;

	// ════════════════════════════════════════════════════════════════════════
	// 辅助函数
	// ════════════════════════════════════════════════════════════════════════

	/** 获取指定槽位的定义 */
	const FYcAttachmentSlotDef* GetSlotDef(FGameplayTag SlotType) const;

	/** 检查是否支持指定槽位 */
	bool HasSlot(FGameplayTag SlotType) const;

	/** 获取所有槽位类型 */
	TArray<FGameplayTag> GetAllSlotTypes() const;

	virtual FString GetDebugString() const override
	{
		return FString::Printf(TEXT("WeaponAttachments: Slots=%d, MaxAttachments=%d"), 
			AttachmentSlots.Num(), MaxAttachments);
	}
};

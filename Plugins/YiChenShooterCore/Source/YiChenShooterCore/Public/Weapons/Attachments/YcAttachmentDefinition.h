// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YcAttachmentTypes.h"
#include "YcAttachmentDefinition.generated.h"

/**
 * EYcAttachmentTarget - 配件附加目标类型
 */
UENUM(BlueprintType)
enum class EYcAttachmentTarget : uint8
{
	/** 附加到武器骨骼网格体的Socket */
	WeaponSocket    UMETA(DisplayName="武器Socket"),
	/** 附加到另一个配件槽位的Mesh组件上 */
	AttachmentSlot  UMETA(DisplayName="其他配件"),
};

/**
 * UYcAttachmentDefinition - 配件定义资产
 * 
 * 每个配件对应一个DataAsset，包含配件的所有静态配置。
 * 
 * 配件系统参考COD: Modern Warfare的Gunsmith系统：
 * - 每个配件属于特定槽位（枪口、瞄具、握把等）
 * - 配件可以修改武器的多个属性
 * - 部分配件之间存在互斥关系
 * - 高级配件支持微调参数
 */
UCLASS(BlueprintType)
class YICHENSHOOTERCORE_API UYcAttachmentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UYcAttachmentDefinition();

	//~ Begin UPrimaryDataAsset Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UPrimaryDataAsset Interface

	// ════════════════════════════════════════════════════════════════════════
	// 基础信息
	// ════════════════════════════════════════════════════════════════════════

	/** 配件唯一标识Tag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1.基础信息")
	FGameplayTag AttachmentId;

	/** 配件显示名称 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1.基础信息")
	FText DisplayName;

	/** 配件描述 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1.基础信息", meta=(MultiLine=true))
	FText Description;

	/** 配件图标 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1.基础信息")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 配件所属槽位类型 (e.g. Attachment.Slot.Muzzle) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="1.基础信息")
	FGameplayTag SlotType;

	// ════════════════════════════════════════════════════════════════════════
	// 属性修改器
	// ════════════════════════════════════════════════════════════════════════

	/** 配件提供的属性修改器列表 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="2.属性修改")
	TArray<FYcStatModifier> StatModifiers;

	// ════════════════════════════════════════════════════════════════════════
	// 视觉配置
	// ════════════════════════════════════════════════════════════════════════

	/** 配件网格体 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="3.视觉")
	TSoftObjectPtr<UStaticMesh> AttachmentMesh;

	/** 附加目标类型 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="3.视觉")
	EYcAttachmentTarget AttachTarget = EYcAttachmentTarget::WeaponSocket;

	/** 
	 * 当AttachTarget为AttachmentSlot时，指定要附加到的目标配件槽位
	 * 例如：消音器附加到枪管槽位，则设置为 Attachment.Slot.Barrel
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="3.视觉", 
		meta=(EditCondition="AttachTarget==EYcAttachmentTarget::AttachmentSlot", EditConditionHides))
	FGameplayTag TargetSlotType;

	/** 
	 * 附加到目标的Socket名称
	 * - WeaponSocket模式：武器骨骼网格体上的Socket名称
	 * - AttachmentSlot模式：目标配件Mesh上的Socket名称
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="3.视觉")
	FName AttachSocketName;

	/** 相对位置偏移 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="3.视觉")
	FTransform AttachOffset;

	/** 安装此配件后需要隐藏的武器部件槽位列表 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="3.视觉")
	TArray<FGameplayTag> HiddenSlots;

	// ════════════════════════════════════════════════════════════════════════
	// 互斥与依赖
	// ════════════════════════════════════════════════════════════════════════

	/** 与此配件互斥的配件Tag列表 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="4.互斥与依赖")
	FGameplayTagContainer IncompatibleAttachments;

	/** 安装此配件需要的前置配件 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="4.互斥与依赖")
	FGameplayTagContainer RequiredAttachments;

	// ════════════════════════════════════════════════════════════════════════
	// 解锁条件
	// ════════════════════════════════════════════════════════════════════════

	/** 解锁所需武器等级 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="5.解锁")
	int32 UnlockWeaponLevel = 1;

	/** 解锁所需挑战Tag（可选） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="5.解锁")
	FGameplayTag UnlockChallengeTag;

	// ════════════════════════════════════════════════════════════════════════
	// 配件调校（可选）
	// ════════════════════════════════════════════════════════════════════════

	/** 是否支持配件调校 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="6.调校")
	bool bSupportsTuning = false;

	/** 调校参数定义 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="6.调校", 
		meta=(EditCondition="bSupportsTuning"))
	TArray<FYcAttachmentTuningParam> TuningParams;

	// ════════════════════════════════════════════════════════════════════════
	// 扩展槽位
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 安装此配件后提供的额外槽位
	 * 例如：战术长枪管可以提供 [枪口, 激光, 下挂] 槽位
	 * 卸载此配件时，其提供的槽位上的子配件也会被自动卸载
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="7.扩展槽位")
	TArray<FYcAttachmentSlotDef> ProvidedSlots;

	/** 检查此配件是否提供指定槽位 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool HasProvidedSlot(FGameplayTag InSlotType) const;

	/** 获取此配件提供的所有槽位类型 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	TArray<FGameplayTag> GetProvidedSlotTypes() const;

	// ════════════════════════════════════════════════════════════════════════
	// 辅助函数
	// ════════════════════════════════════════════════════════════════════════

	/** 获取指定属性的修改器 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	TArray<FYcStatModifier> GetModifiersForStat(FGameplayTag StatTag) const;

	/** 检查是否与指定配件互斥 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool IsIncompatibleWith(FGameplayTag OtherAttachmentId) const;

	/** 检查是否满足前置配件要求 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool HasRequiredAttachments(const TArray<FGameplayTag>& InstalledAttachmentIds) const;
};

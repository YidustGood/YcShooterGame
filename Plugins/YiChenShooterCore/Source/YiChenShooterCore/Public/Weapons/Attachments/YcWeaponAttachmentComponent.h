// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "YcAttachmentTypes.h"
#include "YcWeaponAttachmentComponent.generated.h"

class UYcAttachmentDefinition;
class UYcHitScanWeaponInstance;
struct FYcFragment_WeaponAttachments;

/** 配件变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttachmentChanged, FGameplayTag, SlotType, UYcAttachmentDefinition*, Attachment);

/** 属性重算委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsRecalculated);

/** 槽位变化委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotAvailabilityChanged, FGameplayTag, SlotType, bool, bIsAvailable);

/**
 * UYcWeaponAttachmentComponent - 武器配件管理器组件
 * 
 * 挂载到武器Actor上，管理该武器的所有配件。
 * 
 * 职责：
 * - 管理配件的安装/卸载
 * - 计算配件对武器属性的影响
 * - 管理配件的视觉表现（Phase 2）
 * - 处理配件互斥逻辑
 * 
 * 网络同步：
 * - InstalledAttachments 复制到所有客户端
 * - 配件安装/卸载通过服务器RPC执行
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class YICHENSHOOTERCORE_API UYcWeaponAttachmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcWeaponAttachmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent Interface

	// ════════════════════════════════════════════════════════════════════════
	// 初始化
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 初始化配件系统
	 * @param InWeaponInstance 关联的武器实例
	 * @param AttachmentsFragment 配件配置Fragment
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	void Initialize(UYcHitScanWeaponInstance* InWeaponInstance, 
		const FYcFragment_WeaponAttachments& AttachmentsFragment);

	// ════════════════════════════════════════════════════════════════════════
	// 配件操作
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 安装配件
	 * @param AttachmentDef 要安装的配件定义
	 * @param SlotType 目标槽位（可选，默认使用配件定义的槽位）
	 * @return 是否安装成功
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool InstallAttachment(UYcAttachmentDefinition* AttachmentDef, 
		FGameplayTag SlotType = FGameplayTag());

	/**
	 * 卸载配件
	 * @param SlotType 要卸载的槽位
	 * @return 是否卸载成功
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool UninstallAttachment(FGameplayTag SlotType);

	/**
	 * 检查配件是否可以安装
	 * @param AttachmentDef 要检查的配件
	 * @param OutReason 不可安装的原因
	 * @return 是否可以安装
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool CanInstallAttachment(UYcAttachmentDefinition* AttachmentDef, 
		FText& OutReason) const;

	/**
	 * 清空所有配件
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	void ClearAllAttachments();

	// ════════════════════════════════════════════════════════════════════════
	// 配件查询
	// ════════════════════════════════════════════════════════════════════════

	/** 获取指定槽位的已安装配件 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	FYcAttachmentInstance GetAttachmentInSlot(FGameplayTag SlotType) const;

	/** 获取指定槽位的配件定义 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	UYcAttachmentDefinition* GetAttachmentDefInSlot(FGameplayTag SlotType) const;

	/** 获取所有已安装配件 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	TArray<FYcAttachmentInstance> GetAllAttachments() const { return InstalledAttachments; }

	/** 检查指定槽位是否有配件 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool HasAttachmentInSlot(FGameplayTag SlotType) const;

	/** 获取已安装配件数量 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	int32 GetInstalledAttachmentCount() const;

	/** 获取所有已安装配件的ID列表 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	TArray<FGameplayTag> GetInstalledAttachmentIds() const;

	// ════════════════════════════════════════════════════════════════════════
	// 槽位查询
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 获取所有当前可用的槽位（基础槽位 + 配件提供的动态槽位）
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Slots")
	TArray<FYcAttachmentSlotDef> GetAllAvailableSlots() const;

	/**
	 * 获取基础槽位（武器本身定义的）
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Slots")
	TArray<FYcAttachmentSlotDef> GetBaseSlots() const;

	/**
	 * 获取动态槽位（由已安装配件提供的）
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Slots")
	TArray<FYcAttachmentSlotDef> GetDynamicSlots() const;

	/**
	 * 检查指定槽位是否当前可用
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Slots")
	bool IsSlotAvailable(FGameplayTag SlotType) const;

	/**
	 * 获取指定槽位的定义
	 * @param SlotType 槽位类型
	 * @param OutSlotDef 输出的槽位定义
	 * @return 是否找到
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Slots")
	bool GetSlotDef(FGameplayTag SlotType, FYcAttachmentSlotDef& OutSlotDef) const;

	/**
	 * 获取指定配件提供的槽位
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Slots")
	TArray<FYcAttachmentSlotDef> GetSlotsProvidedByAttachment(FGameplayTag SlotType) const;

	// ════════════════════════════════════════════════════════════════════════
	// 属性修改器
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 获取指定属性的所有修改器
	 * @param StatTag 属性Tag
	 * @return 该属性的所有修改器列表
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Stats")
	TArray<FYcStatModifier> GetModifiersForStat(FGameplayTag StatTag) const;

	/**
	 * 计算属性最终值
	 * 
	 * 应用顺序：
	 * 1. Override修改器（如果有，直接覆盖基础值）
	 * 2. Add修改器（累加）
	 * 3. Multiply修改器（累乘，Value表示增加的百分比，如0.1表示+10%）
	 * 
	 * @param StatTag 属性Tag
	 * @param BaseValue 基础值
	 * @return 应用所有修改器后的最终值
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Stats")
	float CalculateFinalStatValue(FGameplayTag StatTag, float BaseValue) const;

	/**
	 * 批量计算多个属性的最终值
	 * @param StatBaseValues 属性Tag到基础值的映射
	 * @return 属性Tag到最终值的映射
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Stats")
	TMap<FGameplayTag, float> CalculateFinalStatValues(
		const TMap<FGameplayTag, float>& StatBaseValues) const;

	// ════════════════════════════════════════════════════════════════════════
	// 调校系统
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 设置配件调校值
	 * @param SlotType 配件槽位
	 * @param StatTag 调校参数Tag
	 * @param Value 调校值
	 * @return 是否设置成功
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Tuning")
	bool SetTuningValue(FGameplayTag SlotType, FGameplayTag StatTag, float Value);

	/**
	 * 获取配件调校值
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Tuning")
	float GetTuningValue(FGameplayTag SlotType, FGameplayTag StatTag) const;

	/**
	 * 重置配件调校值到默认
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Tuning")
	void ResetTuningValues(FGameplayTag SlotType);

	// ════════════════════════════════════════════════════════════════════════
	// 事件委托
	// ════════════════════════════════════════════════════════════════════════

	/** 配件安装事件 */
	UPROPERTY(BlueprintAssignable, Category="Attachment")
	FOnAttachmentChanged OnAttachmentInstalled;

	/** 配件卸载事件 */
	UPROPERTY(BlueprintAssignable, Category="Attachment")
	FOnAttachmentChanged OnAttachmentUninstalled;

	/** 属性变化事件（配件变化导致属性重算时触发） */
	UPROPERTY(BlueprintAssignable, Category="Attachment")
	FOnStatsRecalculated OnStatsRecalculated;

	/** 槽位可用性变化事件（配件提供/移除槽位时触发） */
	UPROPERTY(BlueprintAssignable, Category="Attachment")
	FOnSlotAvailabilityChanged OnSlotAvailabilityChanged;

protected:
	/** 已安装的配件列表 */
	UPROPERTY(ReplicatedUsing=OnRep_InstalledAttachments, BlueprintReadOnly, Category="Attachment")
	TArray<FYcAttachmentInstance> InstalledAttachments;

	/** 武器实例引用 */
	UPROPERTY()
	TWeakObjectPtr<UYcHitScanWeaponInstance> WeaponInstance;

	/** 配件配置Fragment缓存 */
	const FYcFragment_WeaponAttachments* AttachmentsConfig = nullptr;

	/** 动态槽位列表（由已安装配件提供） */
	UPROPERTY()
	TArray<FYcAttachmentSlotDef> DynamicSlots;

	/** 是否已初始化 */
	bool bInitialized = false;

	// ════════════════════════════════════════════════════════════════════════
	// 内部函数
	// ════════════════════════════════════════════════════════════════════════

	/** 查找槽位索引 */
	int32 FindSlotIndex(FGameplayTag SlotType) const;

	/** 通知武器实例重算属性 */
	void NotifyStatsChanged();

	/** 网络复制回调 */
	UFUNCTION()
	void OnRep_InstalledAttachments();

	/** 收集所有修改器（包括调校） */
	void CollectAllModifiers(TArray<FYcStatModifier>& OutModifiers) const;

	/** 添加配件提供的动态槽位 */
	void AddDynamicSlots(UYcAttachmentDefinition* AttachmentDef, FGameplayTag ProviderSlotType);

	/** 移除配件提供的动态槽位（同时卸载槽位上的子配件） */
	void RemoveDynamicSlots(FGameplayTag ProviderSlotType);

	/** 检查槽位是否为动态槽位 */
	bool IsDynamicSlot(FGameplayTag SlotType) const;
};

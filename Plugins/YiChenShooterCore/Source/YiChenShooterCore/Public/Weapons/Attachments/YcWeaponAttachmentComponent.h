// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DataRegistryId.h"
#include "YcAttachmentTypes.h"
#include "YcWeaponAttachmentComponent.generated.h"

class UYcHitScanWeaponInstance;
struct FYcFragment_WeaponAttachments;
struct FYcAttachmentDefinition;

/** 配件变化委托 - 使用 FDataRegistryId 标识配件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttachmentChanged, FGameplayTag, SlotType, const FDataRegistryId&, AttachmentId);

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
 * - AttachmentArray 使用 Fast Array 增量复制到所有客户端
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
	 * @param AttachmentId 要安装的配件 DataRegistry ID
	 * @param SlotType 目标槽位（可选，默认使用配件定义的槽位）
	 * @return 是否安装成功
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool InstallAttachment(const FDataRegistryId& AttachmentId, 
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
	 * @param AttachmentId 要检查的配件 DataRegistry ID
	 * @param OutReason 不可安装的原因
	 * @return 是否可以安装
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool CanInstallAttachment(const FDataRegistryId& AttachmentId, 
		FText& OutReason) const;

	/**
	 * 清空所有配件
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	void ClearAllAttachments();

	// ════════════════════════════════════════════════════════════════════════
	// 配件查询
	// ════════════════════════════════════════════════════════════════════════
	
	/** 获取指定槽位的配件定义 (从 DataRegistry 获取) - 仅 C++ 可用，返回指针避免复制 */
	const FYcAttachmentDefinition* GetAttachmentDefInSlot(FGameplayTag SlotType) const;

	/** 
	 * 获取指定槽位的配件定义 - 蓝图版本
	 * @param SlotType 槽位类型
	 * @param OutDefinition 输出的配件定义（复制）
	 * @return 是否找到配件定义
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment", meta=(DisplayName="Get Attachment Def In Slot"))
	bool K2_GetAttachmentDefInSlot(FGameplayTag SlotType, FYcAttachmentDefinition& OutDefinition) const;
	
	/** 获取指定槽位的已安装配件 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	FYcAttachmentInstance GetAttachmentInSlot(FGameplayTag SlotType) const;

	/** 获取所有已安装配件 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	TArray<FYcAttachmentInstance> GetAllAttachments() const { return AttachmentArray.Items; }

	/** 检查指定槽位是否有配件 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	bool HasAttachmentInSlot(FGameplayTag SlotType) const;

	/** 获取已安装配件数量 */
	UFUNCTION(BlueprintCallable, Category="Attachment")
	int32 GetInstalledAttachmentCount() const;

	/** 获取所有已安装配件的ID列表 (返回配件的 SlotType Tag) */
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
	// 武器配置/预设系统
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 应用武器配置预设
	 * 
	 * 会先清空现有配件，然后按预设安装配件并应用调校值。
	 * 如果预设中包含无效的配件ID，会跳过该条目并记录警告。
	 * 
	 * @param Loadout 要应用的武器配置
	 * @return 是否所有配件都成功安装（部分失败返回false）
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Loadout")
	bool ApplyLoadout(const FYcWeaponLoadout& Loadout);

	/**
	 * 从当前配置创建预设
	 * 
	 * 遍历所有已安装的配件，创建包含配件ID和调校值的预设。
	 * 
	 * @return 当前武器配置的预设
	 */
	UFUNCTION(BlueprintCallable, Category="Attachment|Loadout")
	FYcWeaponLoadout CreateLoadoutFromCurrent() const;

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
	/** 配件数组 (Fast Array)*/
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleInstanceOnly, Category="Attachment")
	FYcAttachmentArray AttachmentArray;

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

	/** 收集所有修改器（包括调校） */
	void CollectAllModifiers(TArray<FYcStatModifier>& OutModifiers) const;

	/** 
	 * 从 DataRegistry 获取配件定义
	 * @param AttachmentId 配件的 DataRegistry ID
	 * @return 配件定义指针，如果未找到返回 nullptr
	 */
	const FYcAttachmentDefinition* GetAttachmentDefinition(const FDataRegistryId& AttachmentId) const;

	/** 添加配件提供的动态槽位 */
	void AddDynamicSlots(const FYcAttachmentDefinition* AttachmentDef, FGameplayTag ProviderSlotType);

	/** 移除配件提供的动态槽位（同时卸载槽位上的子配件） */
	void RemoveDynamicSlots(FGameplayTag ProviderSlotType);

	/** 检查槽位是否为动态槽位 */
	bool IsDynamicSlot(FGameplayTag SlotType) const;

	// ════════════════════════════════════════════════════════════════════════
	// Fast Array 回调处理 (友元访问)
	// ════════════════════════════════════════════════════════════════════════

	friend struct FYcAttachmentInstance;
	friend struct FYcAttachmentArray;

	/** 处理配件添加回调 (由 Fast Array PostReplicatedAdd 调用) */
	void HandleAttachmentAdded(const FYcAttachmentInstance& Instance);

	/** 处理配件移除回调 (由 Fast Array PreReplicatedRemove 调用) */
	void HandleAttachmentRemoved(const FYcAttachmentInstance& Instance);

	/** 处理配件修改回调 (由 Fast Array PostReplicatedChange 调用) */
	void HandleAttachmentChanged(const FYcAttachmentInstance& Instance);

	// ════════════════════════════════════════════════════════════════════════
	// Server RPC
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 服务器安装配件 RPC
	 * @param AttachmentId 要安装的配件 DataRegistry ID
	 * @param SlotType 目标槽位
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_InstallAttachment(const FDataRegistryId& AttachmentId, FGameplayTag SlotType);

	/**
	 * 服务器卸载配件 RPC
	 * @param SlotType 要卸载的槽位
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UninstallAttachment(FGameplayTag SlotType);

	// ════════════════════════════════════════════════════════════════════════
	// 内部实现方法
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 服务器端执行安装配件
	 * @param AttachmentId 要安装的配件 DataRegistry ID
	 * @param SlotType 目标槽位
	 * @return 是否安装成功
	 */
	bool Internal_InstallAttachment(const FDataRegistryId& AttachmentId, FGameplayTag SlotType);

	/**
	 * 服务器端执行卸载配件
	 * @param SlotType 要卸载的槽位
	 * @return 是否卸载成功
	 */
	bool Internal_UninstallAttachment(FGameplayTag SlotType);
};

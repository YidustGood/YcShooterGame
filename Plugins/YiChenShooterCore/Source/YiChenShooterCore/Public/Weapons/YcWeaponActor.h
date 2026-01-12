// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "DataRegistryId.h"
#include "PrimitiveViewRelevance.h"
#include "YcWeaponActor.generated.h"

class UYcEquipmentInstance;
class UYcEquipmentActorComponent;
class UYcWeaponAttachmentComponent;
class UYcHitScanWeaponInstance;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class USceneComponent;
struct FYcAttachmentDefinition;
struct FDataRegistryId;

/**
 * 武器Actor基类
 * 武器由一个骨骼网格体作为主模型，配件通过配件系统动态附加
 * 由Equipment配置生成
 */
UCLASS(BlueprintType, Blueprintable)
class YICHENSHOOTERCORE_API AYcWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AYcWeaponActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

public:
	// 根组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	// 武器主骨骼网格体
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	// ════════════════════════════════════════════════════════════════════════
	// 配件Mesh组件（通过配件系统动态设置网格体）
	// ════════════════════════════════════════════════════════════════════════

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshMuzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshBarrel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshOptic;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshUnderbarrel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshMagazine;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshMagazineReserve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshLaser;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshForestock;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshIronsights;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Attachments")
	TObjectPtr<UStaticMeshComponent> MeshSlide;

public:
	/** 设置备用弹匣可见性 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetReserveMagazineVisibility(bool bVisibility);

	/** 设置弹匣可见性 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetMagazineVisibility(bool bVisibility);

protected:
	/** 
	 * 装备实例复制回调（内部使用）
	 * 处理配件系统初始化等核心逻辑，然后调用 OnWeaponInitialized
	 * 注意: 这个回调发生的时机可能会晚于Actor的某些网络复制事件, 需要注意时序问题, 例如武器默认配件同步到客户端时这个回调就还未触发
	 */
	UFUNCTION()
	void OnEquipmentInstRep(UYcEquipmentInstance* EquipmentInst);

	/**
	 * 武器初始化完成后的蓝图回调
	 * 在配件系统初始化完成后调用，蓝图可在此添加自定义逻辑
	 * @param EquipmentInst 装备实例
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon", meta = (DisplayName = "On Weapon Initialized"))
	void K2_OnWeaponInitialized(UYcEquipmentInstance* EquipmentInst);

	// ════════════════════════════════════════════════════════════════════════
	// 配件系统
	// ════════════════════════════════════════════════════════════════════════

	/** 获取配件管理器组件 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	UYcWeaponAttachmentComponent* GetAttachmentComponent() const { return AttachmentComponent; }

	/** 获取关联的武器实例 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UYcHitScanWeaponInstance* GetWeaponInstance() const;

	// ════════════════════════════════════════════════════════════════════════
	// 配件视觉系统
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 获取指定槽位对应的Mesh组件
	 * @param SlotType 配件槽位类型Tag
	 * @return 对应的StaticMeshComponent，未找到返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	UStaticMeshComponent* GetMeshComponentForSlot(FGameplayTag SlotType) const;

	/**
	 * 更新指定槽位的配件视觉
	 * @param SlotType 配件槽位类型Tag
	 * @param AttachmentDef 配件定义结构体指针（nullptr则清空）
	 */
	void UpdateAttachmentVisual(FGameplayTag SlotType, const FYcAttachmentDefinition* AttachmentDef);

	/**
	 * 清空指定槽位的配件视觉
	 * @param SlotType 配件槽位类型Tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	void ClearAttachmentVisual(FGameplayTag SlotType);

	/**
	 * 刷新所有配件视觉
	 * 根据AttachmentComponent中的已安装配件更新所有视觉
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	void RefreshAllAttachmentVisuals();

	/**
	 * 设置指定槽位的可见性
	 * @param SlotType 配件槽位类型Tag
	 * @param bVisible 是否可见
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	void SetSlotVisibility(FGameplayTag SlotType, bool bVisible);

	/**
	 * 配件安装时的视觉更新回调
	 * @param SlotType 配件槽位类型Tag
	 * @param AttachmentId 配件的 DataRegistry ID
	 */
	UFUNCTION()
	void OnAttachmentInstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId);

	/**
	 * 配件卸载时的视觉更新回调
	 * @param SlotType 配件槽位类型Tag
	 * @param AttachmentId 配件的 DataRegistry ID
	 */
	UFUNCTION()
	void OnAttachmentUninstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId);

	/**
	 * 槽位可用性变化回调（动态槽位添加/移除时）
	 */
	UFUNCTION()
	void OnSlotAvailabilityChanged(FGameplayTag SlotType, bool bIsAvailable);
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = ( AllowPrivateAccess = "true" ))
	TObjectPtr<UYcEquipmentActorComponent> EquipmentActorComponent;

	/** 配件管理器组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = ( AllowPrivateAccess = "true" ))
	TObjectPtr<UYcWeaponAttachmentComponent> AttachmentComponent;

	/** 槽位类型到Mesh组件的映射 */
	UPROPERTY()
	TMap<FGameplayTag, TObjectPtr<UStaticMeshComponent>> SlotToMeshMap;

	/** 初始化槽位到Mesh组件的映射 */
	void InitializeSlotMeshMapping();

	/** 
	 * 初始化配件系统
	 * 在OnEquipmentInstRep()中调用以确保EquipmentInst有效性
	 */
	void InitializeAttachmentSystem(UYcEquipmentInstance* EquipmentInst);

	/** 绑定配件变化事件 */
	void BindAttachmentEvents();

	/**
	 * 获取配件的附加父组件
	 * @param AttachmentDef 配件定义结构体指针
	 * @param OutParent 输出的父组件
	 * @param OutSocketName 输出的Socket名称
	 * @return 是否成功获取
	 */
	bool GetAttachmentParent(const FYcAttachmentDefinition* AttachmentDef, 
		USceneComponent*& OutParent, FName& OutSocketName) const;

	/**
	 * 应用配件的隐藏槽位设置
	 * @param AttachmentDef 配件定义结构体指针
	 * @param bHide true=隐藏, false=显示
	 */
	void ApplyHiddenSlots(const FYcAttachmentDefinition* AttachmentDef, bool bHide);

	/**
	 * 为动态槽位创建Mesh组件
	 * @param SlotType 槽位类型Tag
	 * @return 创建的Mesh组件
	 */
	UStaticMeshComponent* CreateDynamicMeshComponent(FGameplayTag SlotType);

	/**
	 * 销毁动态槽位的Mesh组件
	 * @param SlotType 槽位类型Tag
	 */
	void DestroyDynamicMeshComponent(FGameplayTag SlotType);
};






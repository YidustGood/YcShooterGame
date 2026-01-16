// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "DataRegistryId.h"
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
 * AYcWeaponActor - 武器Actor基类
 * 
 * 武器由一个骨骼网格体作为主模型，配件网格体通过配件系统动态创建/销毁。
 * 由 Equipment 系统生成和管理。
 * 
 * 配件视觉系统：
 * - 配件网格体按需动态创建，不预分配
 * - 通过 SlotToMeshMap 管理槽位到网格体组件的映射
 * - 支持配件附加到武器Socket或其他配件上
 * - 支持配件隐藏其他槽位的功能
 */
UCLASS(BlueprintType, Blueprintable)
class YICHENSHOOTERCORE_API AYcWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AYcWeaponActor();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginDestroy() override;

public:
	// ════════════════════════════════════════════════════════════════════════
	// 核心组件
	// ════════════════════════════════════════════════════════════════════════

	/** 根组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	/** 武器主骨骼网格体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// ════════════════════════════════════════════════════════════════════════
	// 公共接口
	// ════════════════════════════════════════════════════════════════════════

	/** 获取配件管理器组件 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	UYcWeaponAttachmentComponent* GetAttachmentComponent() const { return AttachmentComponent; }

	/** 获取关联的武器实例 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	UYcHitScanWeaponInstance* GetWeaponInstance() const;

	/** 获取武器主网格体 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// ════════════════════════════════════════════════════════════════════════
	// 配件视觉系统 - 公共接口
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 获取指定槽位的网格体组件
	 * @param SlotType 配件槽位类型Tag
	 * @return 对应的StaticMeshComponent，未找到返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	UStaticMeshComponent* GetAttachmentMeshForSlot(FGameplayTag SlotType) const;

	/**
	 * 检查指定槽位是否有网格体组件
	 * @param SlotType 配件槽位类型Tag
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	bool HasAttachmentMeshForSlot(FGameplayTag SlotType) const;

	/**
	 * 设置指定槽位的可见性
	 * @param SlotType 配件槽位类型Tag
	 * @param bVisible 是否可见
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	void SetSlotVisibility(FGameplayTag SlotType, bool bVisible);

	/**
	 * 刷新所有配件视觉
	 * 根据AttachmentComponent中的已安装配件更新所有视觉
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachment")
	void RefreshAllAttachmentVisuals();

protected:
	// ════════════════════════════════════════════════════════════════════════
	// 装备系统回调
	// ════════════════════════════════════════════════════════════════════════

	/** 
	 * 装备实例复制回调（内部使用）
	 * 处理配件系统初始化等核心逻辑，然后调用 K2_OnWeaponInitialized
	 * 
	 * 注意: 此回调可能晚于 Actor 的某些网络复制事件，
	 * 例如武器默认配件可能在此回调前就已同步到客户端
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
	// 配件视觉系统 - 内部实现
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 创建配件网格体组件
	 * @param SlotType 槽位类型Tag
	 * @param AttachmentDef 配件定义
	 * @return 创建的网格体组件，失败返回nullptr
	 */
	UStaticMeshComponent* CreateAttachmentMesh(FGameplayTag SlotType, const FYcAttachmentDefinition* AttachmentDef);

	/**
	 * 销毁配件网格体组件
	 * @param SlotType 槽位类型Tag
	 */
	void DestroyAttachmentMesh(FGameplayTag SlotType);

	/**
	 * 销毁所有配件网格体组件
	 */
	void DestroyAllAttachmentMeshes();

	/**
	 * 配置网格体组件的附加关系和变换
	 * @param MeshComp 要配置的网格体组件
	 * @param AttachmentDef 配件定义
	 */
	void ConfigureAttachmentMesh(UStaticMeshComponent* MeshComp, const FYcAttachmentDefinition* AttachmentDef);

	/**
	 * 获取配件的附加父组件和Socket
	 * @param AttachmentDef 配件定义
	 * @param OutParent 输出的父组件
	 * @param OutSocketName 输出的Socket名称
	 * @return 是否成功获取
	 */
	bool ResolveAttachmentParent(const FYcAttachmentDefinition* AttachmentDef, 
		USceneComponent*& OutParent, FName& OutSocketName) const;

	/**
	 * 应用配件的隐藏槽位设置
	 * @param AttachmentDef 配件定义
	 * @param bHide true=隐藏, false=显示
	 */
	void ApplyHiddenSlots(const FYcAttachmentDefinition* AttachmentDef, bool bHide);

	// ════════════════════════════════════════════════════════════════════════
	// 配件事件回调
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 配件安装回调
	 * @param SlotType 配件槽位类型Tag
	 * @param AttachmentId 配件的 DataRegistry ID
	 */
	UFUNCTION()
	void OnAttachmentInstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId);

	/**
	 * 配件卸载回调
	 * @param SlotType 配件槽位类型Tag
	 * @param AttachmentId 配件的 DataRegistry ID
	 */
	UFUNCTION()
	void OnAttachmentUninstalled(FGameplayTag SlotType, const FDataRegistryId& AttachmentId);

private:
	// ════════════════════════════════════════════════════════════════════════
	// 私有组件
	// ════════════════════════════════════════════════════════════════════════

	/** 装备Actor组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcEquipmentActorComponent> EquipmentActorComponent;

	/** 配件管理器组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcWeaponAttachmentComponent> AttachmentComponent;

	// ════════════════════════════════════════════════════════════════════════
	// 配件网格体管理
	// ════════════════════════════════════════════════════════════════════════

	/** 
	 * 槽位到网格体组件的映射
	 * 动态创建的配件网格体存储在此映射中
	 */
	UPROPERTY(VisibleInstanceOnly, Category = "Attachment|Debug")
	TMap<FGameplayTag, TObjectPtr<UStaticMeshComponent>> SlotToMeshMap;
	
	/** 
	 * 武器网格体第一人称图元类型
	 * 确定武器网格体如何与第一人称渲染进行交互, 包括WeaponMesh和动态生成的配件模型, 1P/3P WeaponActor需要设置匹配的类型以确保正确渲染
	 * 注意：一定不要取名为FirstPersonPrimitiveType, 会导致运行时值为None (UE5.6)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment", meta=(AllowPrivateAccess = "true"))
	EFirstPersonPrimitiveType FirstPersonType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment", meta=(AllowPrivateAccess = "true"))
	int32 FirstPersonPrimitiveTypeInt;

	// ════════════════════════════════════════════════════════════════════════
	// 初始化
	// ════════════════════════════════════════════════════════════════════════

	/** 初始化配件系统 */
	void InitializeAttachmentSystem(UYcEquipmentInstance* EquipmentInst);

	/** 绑定配件变化事件 */
	void BindAttachmentEvents();
};

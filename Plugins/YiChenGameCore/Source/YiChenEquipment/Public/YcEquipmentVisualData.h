// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Engine/DataAsset.h"
#include "YcEquipmentVisualData.generated.h"

/** 装备视觉资产标签 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Yc_Asset_Item_EquipmentVisualData);

/**
 * 装备视觉资产类型枚举
 * 用于标识扩展资产 Map 中的资产类型
 */
UENUM(BlueprintType)
enum class EEquipmentVisualAssetType : uint8
{
	None				UMETA(DisplayName = "None"),
	SkeletalMesh		UMETA(DisplayName = "Skeletal Mesh"),
	StaticMesh			UMETA(DisplayName = "Static Mesh"),
	AnimMontage			UMETA(DisplayName = "Anim Montage"),
	AnimSequence		UMETA(DisplayName = "Anim Sequence"),
	AnimBlueprint		UMETA(DisplayName = "Anim Blueprint"),
	ParticleSystem		UMETA(DisplayName = "Particle System (Cascade)"),
	NiagaraSystem		UMETA(DisplayName = "Niagara System"),
	Sound				UMETA(DisplayName = "Sound"),
	Material			UMETA(DisplayName = "Material"),
	MaterialInstance	UMETA(DisplayName = "Material Instance"),
	Texture				UMETA(DisplayName = "Texture"),
	Blueprint			UMETA(DisplayName = "Blueprint Class"),
	DataAsset			UMETA(DisplayName = "Data Asset"),
	Other				UMETA(DisplayName = "Other"),
};

/**
 * 装备视觉资产条目
 * 用于在 TMap 中存储任意类型的视觉资产
 * 
 * 支持：
 * - 资产类型标识（用于运行时类型检查）
 * - 软引用（避免连锁加载）
 * - 附加信息（Socket、偏移变换）
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FYcEquipmentVisualAssetEntry
{
	GENERATED_BODY()

	FYcEquipmentVisualAssetEntry()
		: AssetType(EEquipmentVisualAssetType::None)
	{
	}

	/** 资产类型，用于运行时类型检查和编辑器提示 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset")
	EEquipmentVisualAssetType AssetType;

	/** 资产软引用（通用 UObject 类型） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset", meta = (AssetBundles = "EquipmentVisual"))
	TSoftObjectPtr<UObject> Asset;

	/** 获取已加载的资产并转换为指定类型 */
	template<typename T>
	T* GetAsset() const
	{
		return Cast<T>(Asset.Get());
	}

	/** 获取资产的软引用路径 */
	FSoftObjectPath GetAssetPath() const { return Asset.ToSoftObjectPath(); }

	/** 检查资产是否有效（已设置路径） */
	bool IsValid() const { return !Asset.IsNull(); }

	/** 检查资产是否已加载到内存 */
	bool IsLoaded() const { return Asset.IsValid(); }

	/** 检查资产类型是否匹配 */
	bool IsType(EEquipmentVisualAssetType InType) const { return AssetType == InType; }
};

/**
 * 装备视觉数据资产
 * 
 * 提供通用的 GameplayTag 索引资产管理能力，可直接使用或作为基类派生。
 * 
 * 设计理念：
 * - 通用性：只提供扩展 TMap，不预设具体字段，适用于任意装备类型
 * - 灵活性：通过 GameplayTag 索引资产，无需修改代码即可扩展
 * - 可继承：复杂装备（如武器）可继承此类添加类型特定的核心字段
 * 
 * 使用场景：
 * - 简单装备：直接使用此类，通过 ExtendedAssets 配置所有资产
 * - 复杂装备：继承此类，添加核心字段（如 UYcWeaponVisualData）
 * 
 * 使用方式：
 * 1. 在编辑器中创建此类（或派生类）的蓝图资产
 * 2. 在 AssetManager 中注册类型（Primary Asset Types to Scan）
 * 3. 通过 FEquipmentFragment_Visual 引用
 * 4. 运行时通过 AssetManager 异步加载
 * 
 * Tag 命名规范建议：
 * - Asset.Equipment.Mesh.XXX     - 模型资产
 * - Asset.Equipment.Anim.XXX     - 动画资产
 * - Asset.Equipment.Effect.XXX   - 特效资产
 * - Asset.Equipment.Sound.XXX    - 音效资产
 * - Asset.Equipment.Material.XXX - 材质资产
 */
UCLASS(BlueprintType)
class YICHENEQUIPMENT_API UYcEquipmentVisualData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UYcEquipmentVisualData();

	// ================================================================================
	// 扩展资产（通过 GameplayTag 索引）
	// ================================================================================

	/**
	 * 扩展视觉资产
	 * 通用资产容器，支持任意类型的视觉资源
	 * 
	 * 推荐 Tag 命名规范：
	 * - Asset.Equipment.Mesh.XXX     - 模型资产
	 * - Asset.Equipment.Anim.XXX     - 动画资产
	 * - Asset.Equipment.Effect.XXX   - 特效资产
	 * - Asset.Equipment.Sound.XXX    - 音效资产
	 * - Asset.Equipment.Material.XXX - 材质资产
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual|Extended", meta = (AssetBundles = "EquipmentVisual", Categories = "Asset", ForceInlineRow))
	TMap<FGameplayTag, FYcEquipmentVisualAssetEntry> ExtendedAssets;

	// ================================================================================
	// 辅助函数 - C++ 版本
	// ================================================================================

	/**
	 * 获取扩展资产条目
	 * @param AssetTag 资产标签
	 * @return 资产条目指针，未找到返回 nullptr
	 */
	const FYcEquipmentVisualAssetEntry* GetAsset(FGameplayTag AssetTag) const;

	/**
	 * 获取扩展资产并转换为指定类型（C++ 模板版本）
	 * @param AssetTag 资产标签
	 * @return 转换后的资产指针，未找到或类型不匹配返回 nullptr
	 */
	template<typename T>
	T* GetAssetAs(FGameplayTag AssetTag) const
	{
		if (const FYcEquipmentVisualAssetEntry* Entry = GetAsset(AssetTag))
		{
			return Entry->GetAsset<T>();
		}
		return nullptr;
	}

	/**
	 * 获取指定类型的所有资产
	 * @param AssetType 资产类型
	 * @return 匹配的资产条目数组
	 */
	TArray<const FYcEquipmentVisualAssetEntry*> GetAssetsByType(EEquipmentVisualAssetType AssetType) const;

	// ================================================================================
	// 辅助函数 - 蓝图版本
	// ================================================================================

	/**
	 * 获取扩展资产（蓝图版本）
	 * @param AssetTag 资产标签
	 * @param OutAssetEntry 输出的资产条目
	 * @return 是否找到
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Visual", meta = (DisplayName = "Get Asset"))
	bool K2_GetAsset(FGameplayTag AssetTag, FYcEquipmentVisualAssetEntry& OutAssetEntry) const;

	/**
	 * 获取扩展资产的 UObject（蓝图版本）
	 * @param AssetTag 资产标签
	 * @return 资产对象，未找到或未加载返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Visual", meta = (DisplayName = "Get Asset Object"))
	UObject* K2_GetAssetObject(FGameplayTag AssetTag) const;

	/**
	 * 检查是否存在指定的资产
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Visual")
	bool HasAsset(FGameplayTag AssetTag) const;

	/**
	 * 获取所有资产的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Visual")
	TArray<FGameplayTag> GetAllAssetTags() const;

	/**
	 * 获取资产数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Visual")
	int32 GetAssetCount() const { return ExtendedAssets.Num(); }
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "YcEquipmentVisualData.h"
#include "YcWeaponVisualData.generated.h"

class UAnimMontage;
class UStaticMesh;
class USkeletalMesh;
class UAnimSequence;
class UAnimInstance;
class UNiagaraSystem;
class USoundBase;

/** 武器视觉资产标签 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Yc_Asset_Item_WeaponVisualData);

/**
 * 武器动作视觉数据
 * 包含一个武器动作的 TP、FP 动画资产（软引用）
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcWeaponActionVisual
{
	GENERATED_BODY()
	
	/** 人物模型的第一人称动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Anims", meta = (AssetBundles = "EquipmentVisual"))
	TSoftObjectPtr<UAnimMontage> FPCharacterAnimMontage;
	
	/** 武器模型的第一人称动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Anims", meta = (AssetBundles = "EquipmentVisual"))
	TSoftObjectPtr<UAnimMontage> FPWeaponAnimMontage;

	/** 人物模型的第三人称动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Anims", meta = (AssetBundles = "EquipmentVisual"))
	TSoftObjectPtr<UAnimMontage> TPCharacterAnimMontage;
	
	/** 武器模型的第三人称动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Anims", meta = (AssetBundles = "EquipmentVisual"))
	TSoftObjectPtr<UAnimMontage> TPWeaponAnimMontage;

	/** 检查是否有任何有效的动画资产 */
	bool HasAnyAnimation() const
	{
		return !FPCharacterAnimMontage.IsNull() || !FPWeaponAnimMontage.IsNull() 
			|| !TPCharacterAnimMontage.IsNull() || !TPWeaponAnimMontage.IsNull();
	}
};

/**
 * 武器视觉数据资产
 * 
 * 继承 UYcEquipmentVisualData，添加武器特有的核心字段。
 * 
 * 架构设计：
 * - 核心资产：武器常用字段直接定义，提供类型安全和编辑器便利
 * - 扩展资产：继承自基类的 TMap，支持灵活扩展
 * 
 * 使用方式：
 * 1. 在编辑器中创建此类的蓝图资产，配置武器的视觉资源
 * 2. 在 AssetManager 中注册此类型（Primary Asset Types to Scan）
 * 3. 在 FEquipmentFragment_VisualAsset 中通过 FPrimaryAssetId 引用
 * 4. 运行时通过 AssetManager 异步加载
 */
UCLASS(BlueprintType)
class YICHENSHOOTERCORE_API UYcWeaponVisualData : public UYcEquipmentVisualData
{
	GENERATED_BODY()

public:
	UYcWeaponVisualData();

	// ================================================================================
	// 武器核心资产（类型安全，常用字段）
	// ================================================================================

	// ========== 模型资源 ==========
	
	/** 武器主模型（骨骼网格体） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Core|Meshes", meta = (AssetBundles = "EquipmentVisual"))
	TSoftObjectPtr<USkeletalMesh> MainWeaponMesh;
	
	/** 武器动画蓝图类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Core|Anims", meta = (AssetBundles = "EquipmentVisual"))
	TSoftClassPtr<UAnimInstance> WeaponAnimInstanceClass;

	// ================================================================================
	// 武器扩展资产（通过 GameplayTag 索引）
	// ================================================================================

	/**
	 * 扩展动作动画
	 * 通过 GameplayTag 索引，支持自定义动作类型
	 * 推荐 Tag 格式：Asset.Weapon.Action.XXX（如 Asset.Weapon.Action.Inspect）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual|Actions", meta = (AssetBundles = "EquipmentVisual", Categories = "Asset", ForceInlineRow))
	TMap<FGameplayTag, FYcWeaponActionVisual> Actions;

	/**
	 * 扩展姿态动画
	 * 通过 GameplayTag 索引，支持自定义姿态
	 * 推荐 Tag 格式：Asset.Weapon.Pose.XXX（如 Asset.Weapon.Pose.Crouch）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual|Pose", meta = (AssetBundles = "EquipmentVisual", Categories = "Asset", ForceInlineRow))
	TMap<FGameplayTag, TSoftObjectPtr<UAnimSequence>> PoseAnims;

	/**
	 * 武器特效
	 * 通过 GameplayTag 索引，支持自定义特效类型
	 * 推荐 Tag 格式：
	 * - Asset.Weapon.VFX.MuzzleFlash（枪口火焰）
	 * - Asset.Weapon.VFX.Shell（弹壳抛出）
	 * - Asset.Weapon.VFX.Impact（弹着点）
	 * - Asset.Weapon.VFX.Tracer（弹道轨迹）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual|VFX", meta = (AssetBundles = "EquipmentVisual", Categories = "Asset", ForceInlineRow))
	TMap<FGameplayTag, TSoftObjectPtr<UNiagaraSystem>> VFXEffects;

	/**
	 * 武器音效
	 * 通过 GameplayTag 索引，支持自定义音效类型
	 * 推荐 Tag 格式：
	 * - Asset.Weapon.Sound.Fire（开火音效）
	 * - Asset.Weapon.Sound.Fire.Suppressed（消音器开火音效）
	 * - Asset.Weapon.Sound.Reload（换弹音效）
	 * - Asset.Weapon.Sound.DryFire（空枪音效）
	 * - Asset.Weapon.Sound.Impact（弹着点音效）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual|Sound", meta = (AssetBundles = "EquipmentVisual", Categories = "Asset", ForceInlineRow))
	TMap<FGameplayTag, TSoftObjectPtr<USoundBase>> SoundEffects;

	// ================================================================================
	// 武器特有辅助函数 - C++ 版本
	// ================================================================================

	/**
	 * 获取动作动画
	 * @param ActionTag 动作标签（如 Weapon.Action.Fire）
	 * @return 动作视觉数据指针，未找到返回 nullptr
	 */
	const FYcWeaponActionVisual* GetActionVisual(FGameplayTag ActionTag) const;

	/**
	 * 获取姿态动画
	 * @param PoseTag 姿态标签（如 Weapon.Pose.Idle）
	 * @return 动画序列软引用
	 */
	TSoftObjectPtr<UAnimSequence> GetPoseAnim(FGameplayTag PoseTag) const;

	/**
	 * 获取特效
	 * @param VFXTag 特效标签（如 Asset.Weapon.VFX.MuzzleFlash）
	 * @return 特效系统软引用
	 */
	TSoftObjectPtr<UNiagaraSystem> GetVFXEffect(FGameplayTag VFXTag) const;

	/**
	 * 获取音效
	 * @param SoundTag 音效标签（如 Asset.Weapon.Sound.Fire）
	 * @return 音效软引用
	 */
	TSoftObjectPtr<USoundBase> GetSoundEffect(FGameplayTag SoundTag) const;

	// ================================================================================
	// 武器特有辅助函数 - 蓝图版本
	// ================================================================================

	/**
	 * 获取动作动画（蓝图版本）
	 * @param ActionTag 动作标签
	 * @param OutActionVisual 输出的动作视觉数据
	 * @return 是否找到
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual", meta = (DisplayName = "Get Action Visual"))
	bool K2_GetActionVisual(FGameplayTag ActionTag, FYcWeaponActionVisual& OutActionVisual) const;

	/**
	 * 获取姿态动画（蓝图版本）
	 * @param PoseTag 姿态标签
	 * @return 动画序列软引用
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual", meta = (DisplayName = "Get Pose Anim"))
	TSoftObjectPtr<UAnimSequence> K2_GetPoseAnim(FGameplayTag PoseTag) const;

	/**
	 * 检查是否存在指定的动作
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	bool HasAction(FGameplayTag ActionTag) const;

	/**
	 * 获取所有动作的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	TArray<FGameplayTag> GetAllActionTags() const;

	/**
	 * 获取所有姿态的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	TArray<FGameplayTag> GetAllPoseTags() const;

	/**
	 * 获取特效（蓝图版本）
	 * @param VFXTag 特效标签
	 * @return 特效系统软引用
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual", meta = (DisplayName = "Get VFX Effect"))
	TSoftObjectPtr<UNiagaraSystem> K2_GetVFXEffect(FGameplayTag VFXTag) const;

	/**
	 * 检查是否存在指定的特效
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	bool HasVFXEffect(FGameplayTag VFXTag) const;

	/**
	 * 获取所有特效的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	TArray<FGameplayTag> GetAllVFXTags() const;

	/**
	 * 获取音效（蓝图版本）
	 * @param SoundTag 音效标签
	 * @return 音效软引用
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual", meta = (DisplayName = "Get Sound Effect"))
	TSoftObjectPtr<USoundBase> K2_GetSoundEffect(FGameplayTag SoundTag) const;

	/**
	 * 检查是否存在指定的音效
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	bool HasSoundEffect(FGameplayTag SoundTag) const;

	/**
	 * 获取所有音效的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual")
	TArray<FGameplayTag> GetAllSoundTags() const;
};

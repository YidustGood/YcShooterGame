// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcWeaponInstance.h"
#include "Weapons/Fragments/YcFragment_WeaponStats.h"
#include "YcHitScanWeaponInstance.generated.h"

struct FYcRecoilPatternPoint;
struct FYcFragment_WeaponAttachments;
class UYcWeaponAttachmentComponent;

/**
 * FYcComputedWeaponStats - 计算后的武器数值
 * 
 * 缓存基础数值 + 配件修正后的最终数值
 * 避免每次访问都重新计算
 * 配件系统接入后，这些值会在配件变化时重新计算
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcComputedWeaponStats
{
	GENERATED_BODY()

	// ==================== 射击参数 ====================
	UPROPERTY(BlueprintReadOnly) int32 BulletsPerCartridge = 1;
	UPROPERTY(BlueprintReadOnly) float MaxDamageRange = 25000.0f;
	UPROPERTY(BlueprintReadOnly) float BulletTraceSweepRadius = 0.0f;
	UPROPERTY(BlueprintReadOnly) float FireRate = 600.0f;

	// ==================== 伤害参数 ====================
	UPROPERTY(BlueprintReadOnly) float BaseDamage = 25.0f;
	UPROPERTY(BlueprintReadOnly) float HeadshotMultiplier = 2.0f;
	UPROPERTY(BlueprintReadOnly) float ArmorPenetration = 0.0f;

	// ==================== 弹药参数 ====================
	UPROPERTY(BlueprintReadOnly) int32 MagazineSize = 30;
	UPROPERTY(BlueprintReadOnly) int32 MaxReserveAmmo = 120;
	UPROPERTY(BlueprintReadOnly) float ReloadTime = 2.0f;
	UPROPERTY(BlueprintReadOnly) float TacticalReloadTime = 1.5f;


	// ==================== 腰射扩散参数 ====================
	UPROPERTY(BlueprintReadOnly) float HipFireBaseSpread = 2.0f;
	UPROPERTY(BlueprintReadOnly) float HipFireMaxSpread = 6.0f;
	UPROPERTY(BlueprintReadOnly) float HipFireSpreadPerShot = 0.5f;
	UPROPERTY(BlueprintReadOnly) float SpreadRecoveryRate = 8.0f;
	UPROPERTY(BlueprintReadOnly) float SpreadRecoveryDelay = 0.1f;
	UPROPERTY(BlueprintReadOnly) float SpreadExponent = 1.0f;

	// ==================== 瞄准扩散参数 ====================
	UPROPERTY(BlueprintReadOnly) float ADSBaseSpread = 0.0f;
	UPROPERTY(BlueprintReadOnly) float ADSMaxSpread = 0.0f;
	UPROPERTY(BlueprintReadOnly) float ADSSpreadPerShot = 0.0f;

	// ==================== 状态扩散乘数 ====================
	UPROPERTY(BlueprintReadOnly) float MovingSpreadMultiplier = 1.5f;
	UPROPERTY(BlueprintReadOnly) float JumpingSpreadMultiplier = 3.0f;
	UPROPERTY(BlueprintReadOnly) float CrouchingSpreadMultiplier = 0.7f;

	// ==================== 后坐力参数 ====================
	UPROPERTY(BlueprintReadOnly) float VerticalRecoilMin = 0.3f;
	UPROPERTY(BlueprintReadOnly) float VerticalRecoilMax = 0.5f;
	UPROPERTY(BlueprintReadOnly) float HorizontalRecoilMin = -0.2f;
	UPROPERTY(BlueprintReadOnly) float HorizontalRecoilMax = 0.2f;
	UPROPERTY(BlueprintReadOnly) float ADSRecoilMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly) float HipFireRecoilMultiplier = 0.6f;
	UPROPERTY(BlueprintReadOnly) float CrouchingRecoilMultiplier = 0.8f;
	UPROPERTY(BlueprintReadOnly) float RecoilRecoveryRate = 10.0f;
	UPROPERTY(BlueprintReadOnly) float RecoilRecoveryDelay = 0.15f;
	UPROPERTY(BlueprintReadOnly) float RecoilRecoveryPercent = 0.5f;

	// ==================== 瞄准参数 ====================
	UPROPERTY(BlueprintReadOnly) float AimDownSightTime = 0.2f;
	UPROPERTY(BlueprintReadOnly) float ADSFOVMultiplier = 0.8f;
	UPROPERTY(BlueprintReadOnly) float ADSMoveSpeedMultiplier = 0.8f;
};


/**
 * UYcHitScanWeaponInstance - 即时命中(HitScan)武器实例类
 * 
 * 该类实现了COD风格的射击手感系统：
 * 
 * 【瞄准状态(ADS)】
 * - 子弹无扩散，瞄哪打哪
 * - 后坐力影响玩家视角，有固定的弹道轨迹
 * - 玩家可以通过练习掌握压枪技巧
 * 
 * 【腰射状态(Hip Fire)】
 * - 子弹有随机扩散
 * - 后坐力较小
 * - 扩散 = 基础扩散 + 弹道轨迹偏移 + 随机散布
 * 
 * 【配件系统预留】
 * - ComputedStats 缓存计算后的数值
 * - 配件变化时调用 RecalculateStats() 重新计算
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcHitScanWeaponInstance : public UYcWeaponInstance
{
	GENERATED_BODY()

public:
	UYcHitScanWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ UYcEquipmentInstance 接口
	virtual void OnEquipmentInstanceCreated(const FYcEquipmentDefinition& Definition) override;
	virtual void OnEquipped() override;
	virtual void OnUnequipped() override;
	//~ End of UYcEquipmentInstance 接口

	/** 每帧更新，处理扩散恢复和后坐力恢复 */
	void Tick(float DeltaSeconds);

	// ==================== 射击事件 ====================
	
	/**
	 * 射击时调用
	 * 更新扩散、后坐力、射击计数等状态
	 * @param bIsAiming 是否处于瞄准状态
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void OnFired(bool bIsAiming);

	/**
	 * 获取本次射击的后坐力
	 * @param bIsAiming 是否处于瞄准状态
	 * @param bIsCrouching 是否蹲下
	 * @return 后坐力偏移 (Pitch, Yaw)
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	FVector2D GetRecoilForCurrentShot(bool bIsAiming, bool bIsCrouching) const;


	// ==================== 扩散查询 ====================
	
	/**
	 * 获取当前扩散角度（度）
	 * @param bIsAiming 是否处于瞄准状态
	 * @return 当前扩散角度
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Spread")
	float GetCurrentSpreadAngle(bool bIsAiming) const;

	/**
	 * 获取当前扩散乘数
	 * 受移动、跳跃、蹲下状态影响
	 * @return 扩散乘数
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Spread")
	float GetCurrentSpreadMultiplier() const;

	/**
	 * 检查是否有首发精准度
	 * @param bIsAiming 是否处于瞄准状态
	 * @return 是否有首发精准度
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Spread")
	bool HasFirstShotAccuracy(bool bIsAiming) const;

	// ==================== 配置访问 ====================
	
	/** 获取计算后的武器数值（基础 + 配件修正） */
	UFUNCTION(BlueprintCallable, Category="Weapon|Stats")
	const FYcComputedWeaponStats& GetComputedStats() const { return ComputedStats; }

	/** 获取基础配置Fragment */
	const FYcFragment_WeaponStats* GetWeaponStatsFragment() const { return WeaponStatsFragment; }

	/** 获取配件管理器组件 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Attachment")
	UYcWeaponAttachmentComponent* GetAttachmentComponent() const { return AttachmentComponent; }

	/** 获取配件配置Fragment */
	const FYcFragment_WeaponAttachments* GetAttachmentsFragment() const { return AttachmentsFragment; }

	/** 设置配件管理器组件（由武器Actor调用） */
	void SetAttachmentComponent(UYcWeaponAttachmentComponent* InComponent);

	// ==================== 便捷访问函数 ====================
	
	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	int32 GetBulletsPerCartridge() const { return ComputedStats.BulletsPerCartridge; }

	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	float GetMaxDamageRange() const { return ComputedStats.MaxDamageRange; }

	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	float GetBulletTraceSweepRadius() const { return ComputedStats.BulletTraceSweepRadius; }

	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	float GetSpreadExponent() const { return ComputedStats.SpreadExponent; }

	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	float GetFireRate() const { return ComputedStats.FireRate; }

	/** 获取射击模式 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	EYcFireMode GetFireMode() const;

	/** 获取点射数量 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	int32 GetBurstCount() const;

	/** 获取连射间隔倍率 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Config")
	float GetBurstIntervalMultiplier() const;


	// ==================== 后坐力轨迹访问 ====================
	
	/** 是否使用固定弹道轨迹 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	bool UsesRecoilPattern() const;

	/** 获取弹道轨迹数组 */
	const TArray<FYcRecoilPatternPoint>* GetRecoilPattern() const;

	/** 获取当前射击计数（用于弹道轨迹索引） */
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	int32 GetCurrentShotCount() const { return CurrentShotCount; }

	/** 重置射击计数（换弹或停止射击后） */
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	void ResetShotCount() { CurrentShotCount = 0; }

	// ==================== GameplayTag 系统 ====================
	
	/** 武器当前的所有 Tag（复制到客户端） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Weapon|Tags")
	FGameplayTagContainer WeaponTags;

	/** 武器基础 Tag（不复制，本地初始化） */
	UPROPERTY(BlueprintReadOnly, Category="Weapon|Tags")
	FGameplayTagContainer BaseTags;

	/** 更新武器 Tag */
	UFUNCTION(BlueprintCallable, Category="Weapon|Tags")
	void UpdateWeaponTags();

	/** Tag 查询接口 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Tags")
	bool HasWeaponTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Tags")
	bool HasAnyWeaponTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Tags")
	bool HasAllWeaponTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Tags")
	FGameplayTagContainer GetWeaponTags() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Weapon|Tags")
	FGameplayTagContainer GetWeaponTagsByCategory(FGameplayTag CategoryTag) const;

	/** @TODO ASC 集成 */
	void ApplyAttachmentTagsToASC();
	void RemoveAttachmentTagsFromASC();

	/** Tag 变化事件 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponTagsChanged, 
		const FGameplayTagContainer&, NewTags);

	UPROPERTY(BlueprintAssignable, Category="Weapon|Tags")
	FOnWeaponTagsChanged OnWeaponTagsChanged;

	// ==================== 累计后坐力（用于视角恢复） ====================
	
	/** 获取累计后坐力偏移 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	FVector2D GetAccumulatedRecoil() const { return AccumulatedRecoil; }

	/** 消耗累计后坐力（视角恢复时调用） */
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	void ConsumeAccumulatedRecoil(FVector2D Amount);

public:
	// ==================== 配置引用 ====================
	
	/** 武器数值配置Fragment（只读） */
	const FYcFragment_WeaponStats* WeaponStatsFragment = nullptr;

	/** 配件配置Fragment（只读） */
	const FYcFragment_WeaponAttachments* AttachmentsFragment = nullptr;

	/** 配件管理器组件 */
	UPROPERTY()
	TObjectPtr<UYcWeaponAttachmentComponent> AttachmentComponent;

	/** 计算后的数值缓存（配件修正后） */
	FYcComputedWeaponStats ComputedStats;

	/** 重新计算数值（配件变化时调用） */
	void RecalculateStats();

	/** 应用配件修改器 */
	void ApplyAttachmentModifiers();

	/** 广播属性变化消息到 GameplayMessageSubsystem */
	void BroadcastStatsChangedMessage() const;

	// ==================== 运行时状态 ====================
	
	/** 当前腰射扩散角度 */
	float CurrentHipFireSpread = 0.0f;

	/** 当前瞄准扩散角度 */
	float CurrentADSSpread = 0.0f;

	/** 当前扩散乘数 */
	float CurrentSpreadMultiplier = 1.0f;

	/** 是否有首发精准度 */
	bool bHasFirstShotAccuracy = true;

	/** 当前射击计数（用于弹道轨迹） */
	int32 CurrentShotCount = 0;

	/** 累计后坐力偏移（用于视角恢复） */
	FVector2D AccumulatedRecoil = FVector2D::ZeroVector;

	/** 上次射击时间 */
	double LastFireTime = 0.0;

	/** 静止时间累计（用于首发精准度） */
	float StationaryTime = 0.0f;

	/** 缓存的阻止能力 Tag */
	FGameplayTagContainer CachedBlockedAbilityTags;


private:
	/** 更新扩散状态 */
	void UpdateSpread(float DeltaSeconds);

	/** 更新后坐力恢复 */
	void UpdateRecoilRecovery(float DeltaSeconds);

	/** 更新首发精准度状态 */
	void UpdateFirstShotAccuracy(float DeltaSeconds);

	/** 更新状态乘数（移动/跳跃/蹲下） */
	void UpdateStateMultipliers();

	/** 获取弹道轨迹中指定索引的后坐力 */
	FYcRecoilPatternPoint GetRecoilPatternPoint(int32 ShotIndex) const;

	/** 生成随机后坐力 */
	FVector2D GenerateRandomRecoil() const;
};

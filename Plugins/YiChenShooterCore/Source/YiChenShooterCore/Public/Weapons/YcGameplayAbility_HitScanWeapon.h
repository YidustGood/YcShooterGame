// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Ability/YcGameplayAbility_FromEquipment.h"
#include "YcGameplayAbility_HitScanWeapon.generated.h"

class UYcHitScanWeaponInstance;

/**
 * EYcAbilityTargetingSource - 技能瞄准来源枚举
 */
UENUM(BlueprintType)
enum class EYcAbilityTargetingSource : uint8
{
	/** 从玩家摄像机位置出发，朝向摄像机焦点（最常用） */
	CameraTowardsFocus,
	/** 从Pawn中心位置出发，沿Pawn朝向方向 */
	PawnForward,
	/** 从Pawn中心位置出发，朝向摄像机焦点 */
	PawnTowardsFocus,
	/** 从武器枪口位置出发，沿Pawn朝向方向 */
	WeaponForward,
	/** 从武器枪口位置出发，朝向摄像机焦点 */
	WeaponTowardsFocus,
	/** 自定义来源 */
	Custom
};

/**
 * UYcGameplayAbility_HitScanWeapon - 即时命中武器射击技能
 * 
 * 蓝图使用方式：
 * 1. 调用 StartFiring() 开始射击（按下射击键）
 * 2. 调用 StopFiring() 停止射击（松开射击键）
 * 3. 实现 OnRangedWeaponTargetDataReady 处理命中结果（应用伤害、播放特效等）
 * 4. 可选：实现 OnFiringStarted/OnFiringStopped/OnShotFired 处理射击事件
 * 
 * 支持的射击模式：
 * - 全自动：按住持续射击
 * - 半自动：每次按下射击一发
 * - 点射：每次按下射击固定数量子弹
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcGameplayAbility_HitScanWeapon : public UYcGameplayAbility_FromEquipment
{
	GENERATED_BODY()
public:
	UYcGameplayAbility_HitScanWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, Category="YcGameCore|Ability")
	UYcHitScanWeaponInstance* GetWeaponInstance() const;
	
	//~UGameplayAbility 接口
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility 接口

	// ════════════════════════════════════════════════════════════════════════
	// 射击控制 - 蓝图主要接口
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 开始射击
	 * 根据武器的射击模式（全自动/半自动/点射）自动处理射击逻辑
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Firing")
	void StartFiring();

	/**
	 * 停止射击
	 * 对于全自动武器会停止连续射击
	 * @param bEndAbilityWhenDone 停止后是否自动结束技能（默认true）
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Firing")
	void StopFiring(bool bEndAbilityWhenDone = true);

	/** 检查是否正在射击 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Firing")
	bool IsFiring() const { return bIsFiring; }

	/** 检查是否正在瞄准 (ADS) */
	UFUNCTION(BlueprintCallable, Category="Weapon|Firing")
	bool IsAiming() const;

	// ════════════════════════════════════════════════════════════════════════
	// 蓝图事件回调
	// ════════════════════════════════════════════════════════════════════════

	/** 射击开始时的蓝图回调 */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Firing")
	void OnFiringStarted();

	/** 射击停止时的蓝图回调 */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Firing")
	void OnFiringStopped();

	/**
	 * 单次射击时的蓝图回调（每发子弹触发一次）
	 * @param ShotIndex 当前点射中的第几发（从0开始）
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Firing")
	void OnShotFired(int32 ShotIndex);
	
	/**
	 * 目标数据准备就绪时的蓝图回调
	 * 蓝图在此实现：应用伤害、播放命中特效等
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnRangedWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);

	/** 后坐力应用时的蓝图回调 */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Recoil")
	void OnRecoilApplied(float RecoilPitch, float RecoilYaw);

	// ════════════════════════════════════════════════════════════════════════
	// 后坐力相关
	// ════════════════════════════════════════════════════════════════════════
	
	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	FVector2D GetLastShotRecoil() const { return LastShotRecoil; }

	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	void ApplyRecoilToController(float RecoilPitch, float RecoilYaw);

	UFUNCTION(BlueprintCallable, Category="Weapon|Recoil")
	void ApplyLastShotRecoil();

	// ════════════════════════════════════════════════════════════════════════
	// 底层射击函数（高级用法，通常不需要直接调用）
	// ════════════════════════════════════════════════════════════════════════
	
	UFUNCTION(BlueprintCallable, Category="Weapon|Firing|Advanced")
	void StartHitScanWeaponTargeting(bool bIsAiming = false, bool bIsCrouching = false);
	
protected:
	struct FHitScanWeaponFiringInput
	{
		FVector StartTrace;
		FVector EndAim;
		FVector AimDir;
		UYcHitScanWeaponInstance* WeaponData = nullptr;
		bool bCanPlayBulletFX = false;

		FHitScanWeaponFiringInput()
			: StartTrace(ForceInitToZero), EndAim(ForceInitToZero), AimDir(ForceInitToZero) {}
	};
	
	void PerformLocalTargeting(OUT TArray<FHitResult>& OutHits, bool bIsAiming);
	void TraceBulletsInCartridge(const FHitScanWeaponFiringInput& InputData, OUT TArray<FHitResult>& OutHits);
	FHitResult DoSingleBulletTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, OUT TArray<FHitResult>& OutHits) const;
	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated, OUT TArray<FHitResult>& OutHitResults) const;
	virtual void AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const;
	virtual ECollisionChannel DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const;
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);
	
	static int32 FindFirstPawnHitResult(const TArray<FHitResult>& HitResults);
	FVector GetWeaponTargetingSourceLocation() const;
	FTransform GetTargetingTransform(const APawn* SourcePawn, EYcAbilityTargetingSource Source) const;

	/** 执行单次射击 */
	void FireShot();
	/** 射击Timer回调 */
	void OnFireTimerTick();
	/** 计算射击间隔时间（秒） */
	float GetFireInterval() const;
	
private:
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DEBUG", meta=(AllowPrivateAccess="true"))
	bool bDebugQuery = false;

	FVector2D LastShotRecoil = FVector2D::ZeroVector;
	bool bCurrentShotIsAiming = false;
	bool bCurrentShotIsCrouching = false;

	// 射击状态
	bool bIsFiring = false;
	bool bWantsToStopFiring = false;
	bool bPendingEndAbility = false;  // 等待连射完成后结束技能
	int32 CurrentBurstShotCount = 0;
	FTimerHandle FireTimerHandle;
	double LastShotTime = 0.0;
};

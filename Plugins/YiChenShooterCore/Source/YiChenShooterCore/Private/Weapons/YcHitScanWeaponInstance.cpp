// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcHitScanWeaponInstance.h"
#include "Weapons/Fragments/YcFragment_WeaponStats.h"
#include "Weapons/Attachments/YcFragment_WeaponAttachments.h"
#include "Weapons/Attachments/YcWeaponAttachmentComponent.h"
#include "YiChenEquipment/Public/YcEquipmentDefinition.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHitScanWeaponInstance)

UYcHitScanWeaponInstance::UYcHitScanWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UYcHitScanWeaponInstance::OnEquipmentInstanceCreated(const FYcEquipmentDefinition& Definition)
{
	Super::OnEquipmentInstanceCreated(Definition);

	// 从Definition获取武器数值Fragment
	WeaponStatsFragment = Definition.GetTypedFragment<FYcFragment_WeaponStats>();

	// 从Definition获取配件配置Fragment
	AttachmentsFragment = Definition.GetTypedFragment<FYcFragment_WeaponAttachments>();

	// 如果有配件配置，创建配件管理器组件
	if (AttachmentsFragment)
	{
		// 配件组件将在武器Actor生成后初始化
		// 这里只是标记需要配件系统
	}

	// 计算初始数值
	RecalculateStats();

	// 初始化运行时状态
	CurrentHipFireSpread = ComputedStats.HipFireBaseSpread;
	CurrentADSSpread = ComputedStats.ADSBaseSpread;
	CurrentSpreadMultiplier = 1.0f;
	bHasFirstShotAccuracy = true;
	CurrentShotCount = 0;
	AccumulatedRecoil = FVector2D::ZeroVector;
}

void UYcHitScanWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// 装备时重置状态
	bHasFirstShotAccuracy = true;
	CurrentShotCount = 0;
	StationaryTime = 0.0f;
}

void UYcHitScanWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();

	// 卸下时重置扩散
	CurrentHipFireSpread = ComputedStats.HipFireBaseSpread;
	CurrentADSSpread = ComputedStats.ADSBaseSpread;
	CurrentShotCount = 0;
	AccumulatedRecoil = FVector2D::ZeroVector;
}


void UYcHitScanWeaponInstance::RecalculateStats()
{
	if (!WeaponStatsFragment)
	{
		// 没有配置Fragment，使用默认值
		ComputedStats = FYcComputedWeaponStats();
		return;
	}

	// 从Fragment复制基础数值
	const FYcFragment_WeaponStats& Stats = *WeaponStatsFragment;

	ComputedStats.BulletsPerCartridge = Stats.BulletsPerCartridge;
	ComputedStats.MaxDamageRange = Stats.MaxDamageRange;
	ComputedStats.BulletTraceSweepRadius = Stats.BulletTraceSweepRadius;
	ComputedStats.FireRate = Stats.FireRate;

	ComputedStats.BaseDamage = Stats.BaseDamage;
	ComputedStats.HeadshotMultiplier = Stats.HeadshotMultiplier;
	ComputedStats.ArmorPenetration = Stats.ArmorPenetration;

	ComputedStats.MagazineSize = Stats.MagazineSize;
	ComputedStats.MaxReserveAmmo = Stats.MaxReserveAmmo;
	ComputedStats.ReloadTime = Stats.ReloadTime;
	ComputedStats.TacticalReloadTime = Stats.TacticalReloadTime;

	ComputedStats.HipFireBaseSpread = Stats.HipFireBaseSpread;
	ComputedStats.HipFireMaxSpread = Stats.HipFireMaxSpread;
	ComputedStats.HipFireSpreadPerShot = Stats.HipFireSpreadPerShot;
	ComputedStats.SpreadRecoveryRate = Stats.SpreadRecoveryRate;
	ComputedStats.SpreadRecoveryDelay = Stats.SpreadRecoveryDelay;
	ComputedStats.SpreadExponent = Stats.SpreadExponent;

	ComputedStats.ADSBaseSpread = Stats.ADSBaseSpread;
	ComputedStats.ADSMaxSpread = Stats.ADSMaxSpread;
	ComputedStats.ADSSpreadPerShot = Stats.ADSSpreadPerShot;

	ComputedStats.MovingSpreadMultiplier = Stats.MovingSpreadMultiplier;
	ComputedStats.JumpingSpreadMultiplier = Stats.JumpingSpreadMultiplier;
	ComputedStats.CrouchingSpreadMultiplier = Stats.CrouchingSpreadMultiplier;

	ComputedStats.VerticalRecoilMin = Stats.VerticalRecoilMin;
	ComputedStats.VerticalRecoilMax = Stats.VerticalRecoilMax;
	ComputedStats.HorizontalRecoilMin = Stats.HorizontalRecoilMin;
	ComputedStats.HorizontalRecoilMax = Stats.HorizontalRecoilMax;
	ComputedStats.ADSRecoilMultiplier = Stats.ADSRecoilMultiplier;
	ComputedStats.HipFireRecoilMultiplier = Stats.HipFireRecoilMultiplier;
	ComputedStats.CrouchingRecoilMultiplier = Stats.CrouchingRecoilMultiplier;
	ComputedStats.RecoilRecoveryRate = Stats.RecoilRecoveryRate;
	ComputedStats.RecoilRecoveryDelay = Stats.RecoilRecoveryDelay;
	ComputedStats.RecoilRecoveryPercent = Stats.RecoilRecoveryPercent;

	ComputedStats.AimDownSightTime = Stats.AimDownSightTime;
	ComputedStats.ADSFOVMultiplier = Stats.ADSFOVMultiplier;
	ComputedStats.ADSMoveSpeedMultiplier = Stats.ADSMoveSpeedMultiplier;

	// 应用配件修改器
	if (AttachmentComponent)
	{
		ApplyAttachmentModifiers();
	}
}

void UYcHitScanWeaponInstance::ApplyAttachmentModifiers()
{
	if (!AttachmentComponent)
	{
		return;
	}

	// 使用宏简化属性应用
	#define APPLY_ATTACHMENT_MODIFIER(StatName, StatTag) \
		ComputedStats.StatName = AttachmentComponent->CalculateFinalStatValue( \
			FGameplayTag::RequestGameplayTag(FName(TEXT(StatTag))), ComputedStats.StatName)

	// 射击参数
	APPLY_ATTACHMENT_MODIFIER(FireRate, "Weapon.Stat.FireRate");
	APPLY_ATTACHMENT_MODIFIER(MaxDamageRange, "Weapon.Stat.Range.Max");
	APPLY_ATTACHMENT_MODIFIER(BulletTraceSweepRadius, "Weapon.Stat.BulletRadius");

	// 伤害参数
	APPLY_ATTACHMENT_MODIFIER(BaseDamage, "Weapon.Stat.Damage.Base");
	APPLY_ATTACHMENT_MODIFIER(HeadshotMultiplier, "Weapon.Stat.Damage.HeadshotMultiplier");
	APPLY_ATTACHMENT_MODIFIER(ArmorPenetration, "Weapon.Stat.Damage.ArmorPenetration");

	// 弹药参数
	APPLY_ATTACHMENT_MODIFIER(MagazineSize, "Weapon.Stat.Magazine.Size");
	APPLY_ATTACHMENT_MODIFIER(MaxReserveAmmo, "Weapon.Stat.Magazine.ReserveMax");
	APPLY_ATTACHMENT_MODIFIER(ReloadTime, "Weapon.Stat.Reload.Time");
	APPLY_ATTACHMENT_MODIFIER(TacticalReloadTime, "Weapon.Stat.Reload.TacticalTime");

	// 扩散参数
	APPLY_ATTACHMENT_MODIFIER(HipFireBaseSpread, "Weapon.Stat.Spread.HipFire.Base");
	APPLY_ATTACHMENT_MODIFIER(HipFireMaxSpread, "Weapon.Stat.Spread.HipFire.Max");
	APPLY_ATTACHMENT_MODIFIER(HipFireSpreadPerShot, "Weapon.Stat.Spread.HipFire.PerShot");
	APPLY_ATTACHMENT_MODIFIER(ADSBaseSpread, "Weapon.Stat.Spread.ADS.Base");
	APPLY_ATTACHMENT_MODIFIER(ADSMaxSpread, "Weapon.Stat.Spread.ADS.Max");
	APPLY_ATTACHMENT_MODIFIER(ADSSpreadPerShot, "Weapon.Stat.Spread.ADS.PerShot");
	APPLY_ATTACHMENT_MODIFIER(SpreadRecoveryRate, "Weapon.Stat.Spread.Recovery");

	// 后坐力参数
	APPLY_ATTACHMENT_MODIFIER(VerticalRecoilMin, "Weapon.Stat.Recoil.Vertical.Min");
	APPLY_ATTACHMENT_MODIFIER(VerticalRecoilMax, "Weapon.Stat.Recoil.Vertical.Max");
	APPLY_ATTACHMENT_MODIFIER(HorizontalRecoilMin, "Weapon.Stat.Recoil.Horizontal.Min");
	APPLY_ATTACHMENT_MODIFIER(HorizontalRecoilMax, "Weapon.Stat.Recoil.Horizontal.Max");
	APPLY_ATTACHMENT_MODIFIER(ADSRecoilMultiplier, "Weapon.Stat.Recoil.ADSMultiplier");
	APPLY_ATTACHMENT_MODIFIER(HipFireRecoilMultiplier, "Weapon.Stat.Recoil.HipFireMultiplier");
	APPLY_ATTACHMENT_MODIFIER(RecoilRecoveryRate, "Weapon.Stat.Recoil.Recovery");

	// 瞄准参数
	APPLY_ATTACHMENT_MODIFIER(AimDownSightTime, "Weapon.Stat.ADS.Time");
	APPLY_ATTACHMENT_MODIFIER(ADSFOVMultiplier, "Weapon.Stat.ADS.FOVMultiplier");
	APPLY_ATTACHMENT_MODIFIER(ADSMoveSpeedMultiplier, "Weapon.Stat.ADS.MoveSpeed");

	#undef APPLY_ATTACHMENT_MODIFIER
}

void UYcHitScanWeaponInstance::SetAttachmentComponent(UYcWeaponAttachmentComponent* InComponent)
{
	AttachmentComponent = InComponent;
	
	// 如果有配件配置，初始化配件组件
	if (AttachmentComponent && AttachmentsFragment)
	{
		AttachmentComponent->Initialize(this, *AttachmentsFragment);
	}
	RecalculateStats();
}


void UYcHitScanWeaponInstance::Tick(float DeltaSeconds)
{
	UpdateSpread(DeltaSeconds);
	UpdateRecoilRecovery(DeltaSeconds);
	UpdateFirstShotAccuracy(DeltaSeconds);
	UpdateStateMultipliers();
}

void UYcHitScanWeaponInstance::OnFired(bool bIsAiming)
{
	const UWorld* World = GetWorld();
	if (!World) return;

	LastFireTime = World->GetTimeSeconds();
	StationaryTime = 0.0f;
	bHasFirstShotAccuracy = false;

	// 增加射击计数（用于弹道轨迹）
	CurrentShotCount++;

	// 增加扩散
	if (bIsAiming)
	{
		CurrentADSSpread = FMath::Min(
			CurrentADSSpread + ComputedStats.ADSSpreadPerShot,
			ComputedStats.ADSMaxSpread
		);
	}
	else
	{
		CurrentHipFireSpread = FMath::Min(
			CurrentHipFireSpread + ComputedStats.HipFireSpreadPerShot,
			ComputedStats.HipFireMaxSpread
		);
	}

	// 调用基类更新开火时间
	UpdateFiringTime();
}

FVector2D UYcHitScanWeaponInstance::GetRecoilForCurrentShot(bool bIsAiming, bool bIsCrouching) const
{
	FVector2D Recoil;

	// 获取基础后坐力
	if (UsesRecoilPattern())
	{
		// 使用固定弹道轨迹
		const FYcRecoilPatternPoint Point = GetRecoilPatternPoint(CurrentShotCount - 1);
		Recoil = FVector2D(Point.Pitch, Point.Yaw);
	}
	else
	{
		// 使用随机后坐力
		Recoil = GenerateRandomRecoil();
	}

	// 应用状态乘数
	float Multiplier = 1.0f;

	if (bIsAiming)
	{
		Multiplier *= ComputedStats.ADSRecoilMultiplier;
	}
	else
	{
		Multiplier *= ComputedStats.HipFireRecoilMultiplier;
	}

	if (bIsCrouching)
	{
		Multiplier *= ComputedStats.CrouchingRecoilMultiplier;
	}

	Recoil *= Multiplier;

	// 累计后坐力（用于恢复）
	const_cast<UYcHitScanWeaponInstance*>(this)->AccumulatedRecoil += Recoil;

	return Recoil;
}


float UYcHitScanWeaponInstance::GetCurrentSpreadAngle(bool bIsAiming) const
{
	if (bIsAiming)
	{
		return CurrentADSSpread;
	}
	return CurrentHipFireSpread;
}

float UYcHitScanWeaponInstance::GetCurrentSpreadMultiplier() const
{
	return CurrentSpreadMultiplier;
}

bool UYcHitScanWeaponInstance::HasFirstShotAccuracy(bool bIsAiming) const
{
	if (!WeaponStatsFragment || !WeaponStatsFragment->bEnableFirstShotAccuracy)
	{
		return false;
	}

	// 如果配置为仅腰射时有首发精准度，且当前在瞄准，则返回false
	if (WeaponStatsFragment->bFirstShotAccuracyHipFireOnly && bIsAiming)
	{
		return false;
	}

	return bHasFirstShotAccuracy;
}

bool UYcHitScanWeaponInstance::UsesRecoilPattern() const
{
	return WeaponStatsFragment && WeaponStatsFragment->bUseRecoilPattern;
}

const TArray<FYcRecoilPatternPoint>* UYcHitScanWeaponInstance::GetRecoilPattern() const
{
	if (WeaponStatsFragment && WeaponStatsFragment->bUseRecoilPattern)
	{
		return &WeaponStatsFragment->RecoilPattern;
	}
	return nullptr;
}

void UYcHitScanWeaponInstance::ConsumeAccumulatedRecoil(FVector2D Amount)
{
	AccumulatedRecoil -= Amount;

	// 防止过度恢复
	if (AccumulatedRecoil.X < 0.0f) AccumulatedRecoil.X = 0.0f;
	// Yaw可以是负值，不需要限制
}


void UYcHitScanWeaponInstance::UpdateSpread(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (!World) return;

	const double CurrentTime = World->GetTimeSeconds();
	const float TimeSinceLastFire = static_cast<float>(CurrentTime - LastFireTime);

	// 检查是否过了恢复延迟
	if (TimeSinceLastFire > ComputedStats.SpreadRecoveryDelay)
	{
		// 恢复腰射扩散
		CurrentHipFireSpread = FMath::Max(
			CurrentHipFireSpread - ComputedStats.SpreadRecoveryRate * DeltaSeconds,
			ComputedStats.HipFireBaseSpread
		);

		// 恢复瞄准扩散
		CurrentADSSpread = FMath::Max(
			CurrentADSSpread - ComputedStats.SpreadRecoveryRate * DeltaSeconds,
			ComputedStats.ADSBaseSpread
		);

		// 如果扩散恢复到基础值，重置射击计数
		if (FMath::IsNearlyEqual(CurrentHipFireSpread, ComputedStats.HipFireBaseSpread) &&
			FMath::IsNearlyEqual(CurrentADSSpread, ComputedStats.ADSBaseSpread))
		{
			CurrentShotCount = 0;
		}
	}
}

void UYcHitScanWeaponInstance::UpdateRecoilRecovery(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (!World) return;

	const double CurrentTime = World->GetTimeSeconds();
	const float TimeSinceLastFire = static_cast<float>(CurrentTime - LastFireTime);

	// 检查是否过了恢复延迟
	if (TimeSinceLastFire > ComputedStats.RecoilRecoveryDelay)
	{
		// 计算可恢复的最大后坐力
		const float MaxRecoverablePitch = AccumulatedRecoil.X * ComputedStats.RecoilRecoveryPercent;
		const float MaxRecoverableYaw = AccumulatedRecoil.Y * ComputedStats.RecoilRecoveryPercent;

		// 恢复后坐力
		const float RecoveryAmount = ComputedStats.RecoilRecoveryRate * DeltaSeconds;

		if (AccumulatedRecoil.X > 0.0f)
		{
			const float PitchRecovery = FMath::Min(RecoveryAmount, AccumulatedRecoil.X);
			AccumulatedRecoil.X -= PitchRecovery;
		}

		if (FMath::Abs(AccumulatedRecoil.Y) > 0.01f)
		{
			const float YawRecovery = FMath::Min(RecoveryAmount, FMath::Abs(AccumulatedRecoil.Y));
			AccumulatedRecoil.Y -= FMath::Sign(AccumulatedRecoil.Y) * YawRecovery;
		}
	}
}


void UYcHitScanWeaponInstance::UpdateFirstShotAccuracy(float DeltaSeconds)
{
	if (!WeaponStatsFragment || !WeaponStatsFragment->bEnableFirstShotAccuracy)
	{
		bHasFirstShotAccuracy = false;
		return;
	}

	const APawn* OwnerPawn = GetPawn();
	if (!OwnerPawn)
	{
		return;
	}

	// 检查玩家是否静止
	const float SpeedSquared = OwnerPawn->GetVelocity().SizeSquared();
	const bool bIsStationary = SpeedSquared < 100.0f; // 近似静止

	if (bIsStationary)
	{
		StationaryTime += DeltaSeconds;

		// 检查是否达到首发精准度激活时间
		if (StationaryTime >= WeaponStatsFragment->FirstShotAccuracyTime)
		{
			// 还需要检查扩散是否恢复到基础值
			const bool bSpreadRecovered = 
				FMath::IsNearlyEqual(CurrentHipFireSpread, ComputedStats.HipFireBaseSpread) &&
				FMath::IsNearlyEqual(CurrentADSSpread, ComputedStats.ADSBaseSpread);

			if (bSpreadRecovered)
			{
				bHasFirstShotAccuracy = true;
			}
		}
	}
	else
	{
		StationaryTime = 0.0f;
		bHasFirstShotAccuracy = false;
	}
}

void UYcHitScanWeaponInstance::UpdateStateMultipliers()
{
	const APawn* OwnerPawn = GetPawn();
	if (!OwnerPawn)
	{
		CurrentSpreadMultiplier = 1.0f;
		return;
	}

	float Multiplier = 1.0f;

	// 移动状态
	const float Speed = OwnerPawn->GetVelocity().Size();
	if (Speed > 10.0f)
	{
		Multiplier *= ComputedStats.MovingSpreadMultiplier;
	}

	// 跳跃/下落状态
	const UCharacterMovementComponent* MovementComp = 
		OwnerPawn->FindComponentByClass<UCharacterMovementComponent>();
	if (MovementComp)
	{
		if (MovementComp->IsFalling())
		{
			Multiplier *= ComputedStats.JumpingSpreadMultiplier;
		}
		else if (MovementComp->IsCrouching())
		{
			Multiplier *= ComputedStats.CrouchingSpreadMultiplier;
		}
	}

	CurrentSpreadMultiplier = Multiplier;
}


FYcRecoilPatternPoint UYcHitScanWeaponInstance::GetRecoilPatternPoint(int32 ShotIndex) const
{
	if (!WeaponStatsFragment || !WeaponStatsFragment->bUseRecoilPattern)
	{
		return FYcRecoilPatternPoint();
	}

	const TArray<FYcRecoilPatternPoint>& Pattern = WeaponStatsFragment->RecoilPattern;
	if (Pattern.Num() == 0)
	{
		return FYcRecoilPatternPoint();
	}

	// 如果索引在数组范围内，直接返回
	if (ShotIndex < Pattern.Num())
	{
		return Pattern[ShotIndex];
	}

	// 超出范围，从LoopStart开始循环
	const int32 LoopStart = FMath::Clamp(WeaponStatsFragment->RecoilPatternLoopStart, 0, Pattern.Num() - 1);
	const int32 LoopLength = Pattern.Num() - LoopStart;

	if (LoopLength <= 0)
	{
		return Pattern.Last();
	}

	const int32 LoopIndex = (ShotIndex - Pattern.Num()) % LoopLength;
	return Pattern[LoopStart + LoopIndex];
}

FVector2D UYcHitScanWeaponInstance::GenerateRandomRecoil() const
{
	const float Pitch = FMath::RandRange(ComputedStats.VerticalRecoilMin, ComputedStats.VerticalRecoilMax);
	const float Yaw = FMath::RandRange(ComputedStats.HorizontalRecoilMin, ComputedStats.HorizontalRecoilMax);
	return FVector2D(Pitch, Yaw);
}

EYcFireMode UYcHitScanWeaponInstance::GetFireMode() const
{
	if (WeaponStatsFragment)
	{
		return WeaponStatsFragment->FireMode;
	}
	return EYcFireMode::Auto;
}

int32 UYcHitScanWeaponInstance::GetBurstCount() const
{
	if (WeaponStatsFragment)
	{
		return WeaponStatsFragment->BurstCount;
	}
	return 3;
}

float UYcHitScanWeaponInstance::GetBurstIntervalMultiplier() const
{
	if (WeaponStatsFragment)
	{
		return WeaponStatsFragment->BurstIntervalMultiplier;
	}
	return 2.0f;
}

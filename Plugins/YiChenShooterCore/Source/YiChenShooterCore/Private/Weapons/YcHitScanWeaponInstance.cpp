// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcHitScanWeaponInstance.h"

#include "YcAbilitySystemComponent.h"
#include "Weapons/YcWeaponMessages.h"
#include "Weapons/Fragments/YcFragment_WeaponStats.h"
#include "Weapons/Attachments/YcFragment_WeaponAttachments.h"
#include "Weapons/Attachments/YcWeaponAttachmentComponent.h"
#include "Weapons/Attachments/YcAttachmentTypes.h"
#include "YiChenEquipment/Public/YcEquipmentDefinition.h"
#include "Physics/YcPhysicalMaterialWithTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHitScanWeaponInstance)

UYcHitScanWeaponInstance::UYcHitScanWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UYcHitScanWeaponInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	
	// WeaponTag变化在对局中不会很频繁使用Push Model优化, 减少对比开销
	DOREPLIFETIME_WITH_PARAMS_FAST(UYcHitScanWeaponInstance, WeaponTags, SharedParams);
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
	
	// 应用配件的 BlockedAbilityTags 到 ASC
	ApplyAttachmentTagsToASC();
}

void UYcHitScanWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();

	// 卸下时重置扩散
	CurrentHipFireSpread = ComputedStats.HipFireBaseSpread;
	CurrentADSSpread = ComputedStats.ADSBaseSpread;
	CurrentShotCount = 0;
	AccumulatedRecoil = FVector2D::ZeroVector;
	
	// 移除配件的 BlockedAbilityTags 从 ASC
	RemoveAttachmentTagsFromASC();
}

float UYcHitScanWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags) const
{
	if (!WeaponStatsFragment)
	{
		return 1.0f;
	}

	// 如果没有配置伤害衰减曲线，返回 1.0（无衰减）
	if (WeaponStatsFragment->DamageFalloffCurve.GetRichCurveConst()->IsEmpty())
	{
		return 1.0f;
	}

	// 计算距离百分比（相对于最大射程）
	const float MaxRange = ComputedStats.MaxDamageRange;
	if (MaxRange <= 0.0f)
	{
		return 1.0f;
	}

	const float DistancePercent = FMath::Clamp(Distance / MaxRange, 0.0f, 1.0f);

	// 从曲线中采样伤害倍率
	const float DamageMultiplier = WeaponStatsFragment->DamageFalloffCurve.GetRichCurveConst()->Eval(DistancePercent);

	// 确保倍率在合理范围内
	return FMath::Clamp(DamageMultiplier, 0.0f, 1.0f);
}

float UYcHitScanWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags) const
{
	// 默认倍率为 1.0（基础伤害）
	float DamageMultiplier = 1.0f;

	// 尝试转换为带 Tag 的物理材质
	const UYcPhysicalMaterialWithTags* TaggedMaterial = Cast<UYcPhysicalMaterialWithTags>(PhysicalMaterial);
	if (!TaggedMaterial || TaggedMaterial->Tags.IsEmpty())
	{
		// 如果不是带 Tag 的物理材质，或者没有 Tag，返回默认倍率
		return DamageMultiplier;
	}

	// 遍历命中区域伤害倍率映射表，查找匹配的 Tag
	for (const auto& Pair : ComputedStats.HitZoneDamageMultipliers)
	{
		const FGameplayTag& ZoneTag = Pair.Key;
		const float Multiplier = Pair.Value;

		// 检查物理材质是否包含此区域 Tag
		if (TaggedMaterial->Tags.HasTag(ZoneTag))
		{
			// 找到匹配的 Tag，使用对应的倍率
			DamageMultiplier = Multiplier;
			break; // 使用第一个匹配的倍率
		}
	}

	return DamageMultiplier;
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

	// 复制命中区域伤害倍率映射表
	ComputedStats.HitZoneDamageMultipliers = Stats.HitZoneDamageMultipliers;

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

	// 广播属性变化消息（供 UI 和其他解耦系统监听）
	BroadcastStatsChangedMessage();
}

void UYcHitScanWeaponInstance::BroadcastStatsChangedMessage() const
{
	UGameplayMessageSubsystem& MsgSubsystem = UGameplayMessageSubsystem::Get(this);
	
	FYcWeaponStatsChangedMessage Message;
	Message.WeaponInstance = const_cast<UYcHitScanWeaponInstance*>(this);
	Message.OwnerPawn = GetPawn();
	
	MsgSubsystem.BroadcastMessage(TAG_Message_Weapon_StatsChanged, Message);
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

	// 命中区域伤害倍率（批量应用配件修正）
	// 为每个命中区域倍率应用配件修改器
	// 例如：穿甲弹可以提升护甲部位的伤害倍率
	if (!ComputedStats.HitZoneDamageMultipliers.IsEmpty())
	{
		// 构建基础值映射表
		TMap<FGameplayTag, float> BaseMultipliers;
		for (const auto& Pair : ComputedStats.HitZoneDamageMultipliers)
		{
			// 为每个命中区域构建对应的 Stat Tag
			// 例如：Gameplay.Character.Zone.Head -> Weapon.Stat.Damage.Zone.Head
			const FString ZoneTagString = Pair.Key.ToString();
			const FString StatTagString = ZoneTagString.Replace(TEXT("Gameplay.Character.Zone"), TEXT("Weapon.Stat.Damage.Zone"));
			const FGameplayTag StatTag = FGameplayTag::RequestGameplayTag(FName(*StatTagString), false);
			
			if (StatTag.IsValid())
			{
				BaseMultipliers.Add(StatTag, Pair.Value);
			}
		}

		// 批量应用配件修改器
		if (!BaseMultipliers.IsEmpty())
		{
			const TMap<FGameplayTag, float> ModifiedMultipliers = AttachmentComponent->CalculateFinalStatValues(BaseMultipliers);
			
			// 更新命中区域倍率
			for (const auto& Pair : ModifiedMultipliers)
			{
				// 将 Stat Tag 转换回 Zone Tag
				// 例如：Weapon.Stat.Damage.Zone.Head -> Gameplay.Character.Zone.Head
				const FString StatTagString = Pair.Key.ToString();
				const FString ZoneTagString = StatTagString.Replace(TEXT("Weapon.Stat.Damage.Zone"), TEXT("Gameplay.Character.Zone"));
				const FGameplayTag ZoneTag = FGameplayTag::RequestGameplayTag(FName(*ZoneTagString), false);
				
				if (ZoneTag.IsValid())
				{
					ComputedStats.HitZoneDamageMultipliers.Add(ZoneTag, Pair.Value);
				}
			}
		}
	}

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

// ==================== GameplayTag 系统实现 ====================

bool UYcHitScanWeaponInstance::HasWeaponTag(FGameplayTag Tag) const
{
	return WeaponTags.HasTag(Tag);
}

bool UYcHitScanWeaponInstance::HasAnyWeaponTags(const FGameplayTagContainer& Tags) const
{
	return WeaponTags.HasAny(Tags);
}

bool UYcHitScanWeaponInstance::HasAllWeaponTags(const FGameplayTagContainer& Tags) const
{
	return WeaponTags.HasAll(Tags);
}

FGameplayTagContainer UYcHitScanWeaponInstance::GetWeaponTags() const
{
	return WeaponTags;
}

FGameplayTagContainer UYcHitScanWeaponInstance::GetWeaponTagsByCategory(FGameplayTag CategoryTag) const
{
	FGameplayTagContainer FilteredTags;
	
	for (const FGameplayTag& Tag : WeaponTags)
	{
		if (Tag.MatchesTag(CategoryTag))
		{
			FilteredTags.AddTag(Tag);
		}
	}
	
	return FilteredTags;
}

void UYcHitScanWeaponInstance::ApplyAttachmentTagsToASC()
{
	// 获取 Pawn 和 ASC
	APawn* OwnerPawn = GetPawn();
	if (!OwnerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyAttachmentTagsToASC: No owner pawn"));
		return;
	}
	
	UYcAbilitySystemComponent* ASC = UYcAbilitySystemComponent::GetAbilitySystemComponentFromActor(OwnerPawn);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyAttachmentTagsToASC: Failed to get ASC from pawn %s"), *OwnerPawn->GetName());
		return;
	}
	
	// 收集所有配件的 BlockedAbilityTags
	FGameplayTagContainer AllBlockedAbilityTags;
	
	if (AttachmentComponent)
	{
		const TArray<FYcAttachmentInstance>& Attachments = AttachmentComponent->GetAllAttachments();
		
		for (const FYcAttachmentInstance& Attachment : Attachments)
		{
			if (!Attachment.IsValid())
			{
				continue;
			}
			
			// 获取配件定义
			const FYcAttachmentDefinition* AttachmentDef = Attachment.GetDefinition();
			if (!AttachmentDef)
			{
				UE_LOG(LogTemp, Warning, TEXT("ApplyAttachmentTagsToASC: Failed to get attachment definition for %s"), 
					*Attachment.AttachmentRegistryId.ToString());
				continue;
			}
			
			// 收集 BlockedAbilityTags
			AllBlockedAbilityTags.AppendTags(AttachmentDef->BlockedAbilityTags);
		}
	}
	
	// 如果有需要阻止的能力 Tag，添加到 ASC
	if (AllBlockedAbilityTags.Num() > 0)
	{
		// 使用 Loose GameplayTag 机制添加阻止标签
		// 这些标签会被技能的 ActivationBlockedTags 检查，阻止技能激活
		ASC->K2_YcAddLooseGameplayTags(AllBlockedAbilityTags);
		
		// 缓存已应用的 Tag，以便卸载时清理
		CachedBlockedAbilityTags = AllBlockedAbilityTags;
		
		UE_LOG(LogTemp, Verbose, TEXT("ApplyAttachmentTagsToASC: Applied blocked ability tags: %s"), 
			*AllBlockedAbilityTags.ToStringSimple());
	}
}

void UYcHitScanWeaponInstance::RemoveAttachmentTagsFromASC()
{
	// 如果没有缓存的阻止能力 Tag，直接返回
	if (CachedBlockedAbilityTags.Num() == 0)
	{
		return;
	}
	
	// 获取 Pawn 和 ASC
	APawn* OwnerPawn = GetPawn();
	if (!OwnerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("RemoveAttachmentTagsFromASC: No owner pawn"));
		// 即使没有 Pawn，也清空缓存
		CachedBlockedAbilityTags.Reset();
		return;
	}
	
	UYcAbilitySystemComponent* ASC = UYcAbilitySystemComponent::GetAbilitySystemComponentFromActor(OwnerPawn);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("RemoveAttachmentTagsFromASC: Failed to get ASC from pawn %s"), *OwnerPawn->GetName());
		// 即使没有 ASC，也清空缓存
		CachedBlockedAbilityTags.Reset();
		return;
	}
	
	// 从 ASC 移除之前添加的阻止标签
	ASC->K2_YcRemoveLooseGameplayTags(CachedBlockedAbilityTags);
	
	UE_LOG(LogTemp, Verbose, TEXT("RemoveAttachmentTagsFromASC: Removed blocked ability tags: %s"), 
		*CachedBlockedAbilityTags.ToStringSimple());
	
	// 清空缓存
	CachedBlockedAbilityTags.Reset();
}

void UYcHitScanWeaponInstance::UpdateWeaponTags()
{
	// 保存旧的 Tag 容器用于变化检测
	FGameplayTagContainer OldTags = WeaponTags;
	
	// 从 BaseTags 开始
	FGameplayTagContainer NewTags = BaseTags;
	
	// 收集所有配件的 GrantedTags 和 RemovedTags
	FGameplayTagContainer AllGrantedTags;
	FGameplayTagContainer AllRemovedTags;
	
	if (AttachmentComponent)
	{
		const TArray<FYcAttachmentInstance>& Attachments = AttachmentComponent->GetAllAttachments();
		
		for (const FYcAttachmentInstance& Attachment : Attachments)
		{
			if (!Attachment.IsValid())
			{
				continue;
			}
			
			// 获取配件定义
			const FYcAttachmentDefinition* AttachmentDef = Attachment.GetDefinition();
			if (!AttachmentDef)
			{
				UE_LOG(LogTemp, Warning, TEXT("UpdateWeaponTags: Failed to get attachment definition for %s"), 
					*Attachment.AttachmentRegistryId.ToString());
				continue;
			}
			
			// 收集 GrantedTags
			AllGrantedTags.AppendTags(AttachmentDef->GrantedTags);
			
			// 收集 RemovedTags
			AllRemovedTags.AppendTags(AttachmentDef->RemovedTags);
		}
	}
	
	// 计算最终 Tag: BaseTags + GrantedTags - RemovedTags
	NewTags.AppendTags(AllGrantedTags);
	NewTags.RemoveTags(AllRemovedTags);
	
	// 检查是否实际发生变化
	if (!NewTags.HasAllExact(OldTags) || !OldTags.HasAllExact(NewTags))
	{
		// Tag 发生了变化
		WeaponTags = NewTags;
		
		// 标记属性为脏以触发网络复制
		MARK_PROPERTY_DIRTY_FROM_NAME(UYcHitScanWeaponInstance, WeaponTags, this);
		
		// 广播事件
		OnWeaponTagsChanged.Broadcast(WeaponTags);
		
		UE_LOG(LogTemp, Verbose, TEXT("UpdateWeaponTags: %s, Tags: %s"), 
			*GetName(), *WeaponTags.ToStringSimple());
	}
}

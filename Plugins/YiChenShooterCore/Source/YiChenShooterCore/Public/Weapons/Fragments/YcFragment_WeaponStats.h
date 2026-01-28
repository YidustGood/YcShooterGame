// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Fragments/YcEquipmentFragment.h"
#include "Curves/CurveFloat.h"
#include "NativeGameplayTags.h"
#include "YcFragment_WeaponStats.generated.h"

/**
 * EYcFireMode - 射击模式枚举
 */
UENUM(BlueprintType)
enum class EYcFireMode : uint8
{
	/** 全自动 - 按住扳机持续射击 */
	Auto        UMETA(DisplayName="全自动"),
	/** 半自动 - 每次扣扳机射击一发 */
	SemiAuto    UMETA(DisplayName="半自动"),
	/** 连射(按住自动) - 按住扳机自动进行多轮连射（如M16三连发） */
	BurstAuto   UMETA(DisplayName="连射(按住自动)"),
	/** 连射(单次) - 每次扣扳机射击固定数量子弹，需松开再按 */
	BurstSingle UMETA(DisplayName="连射(单次)"),
};

/**
 * FYcRecoilPatternPoint - 后坐力轨迹点
 * 定义弹道轨迹中每一发子弹的后坐力偏移
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcRecoilPatternPoint
{
	GENERATED_BODY()

	FYcRecoilPatternPoint()
		: Pitch(0.0f)
		, Yaw(0.0f)
	{
	}

	FYcRecoilPatternPoint(float InPitch, float InYaw)
		: Pitch(InPitch)
		, Yaw(InYaw)
	{
	}

	/** 垂直后坐力（Pitch，正值向上） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(DisplayName="垂直(Pitch)"))
	float Pitch;

	/** 水平后坐力（Yaw，正值向右，负值向左） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil", meta=(DisplayName="水平(Yaw)"))
	float Yaw;
};


/**
 * FYcFragment_WeaponStats - 武器核心数值配置Fragment
 * 
 * 这是武器系统的核心配置Fragment，包含了绝大部分武器需要的属性。
 * 设计参考了COD系列的射击手感：
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
 * - 所有数值都可以被配件修改器影响
 * - 支持加法、乘法、覆盖三种修正方式
 */
USTRUCT(BlueprintType, meta=(DisplayName="武器数值配置"))
struct YICHENSHOOTERCORE_API FYcFragment_WeaponStats : public FYcEquipmentFragment
{
	GENERATED_BODY()

	FYcFragment_WeaponStats();

	// ════════════════════════════════════════════════════════════════════════
	// 基础射击参数
	// ════════════════════════════════════════════════════════════════════════
	
	/** 
	 * 单发子弹产生的弹丸数量
	 * - 普通武器：1
	 * - 霰弹枪：8-12
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|弹丸", 
		meta=(DisplayName="弹丸数量", ClampMin=1, ClampMax=32))
	int32 BulletsPerCartridge;

	/** 
	 * 最大射程（厘米）
	 * 射线检测的最大距离，超过此距离无法命中
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|射程", 
		meta=(DisplayName="最大射程", ForceUnits=cm, ClampMin=100))
	float MaxDamageRange;

	/** 
	 * 子弹吸附半径（厘米）
	 * 0 = 精确射线检测
	 * >0 = 球形扫描检测，提供子弹吸附效果
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|射程", 
		meta=(DisplayName="子弹吸附半径", ForceUnits=cm, ClampMin=0))
	float BulletTraceSweepRadius;

	/** 射速（发/分钟） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|射速", 
		meta=(DisplayName="射速(发/分钟)", ClampMin=1, ClampMax=2000))
	float FireRate;

	/** 射击模式 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|射速", 
		meta=(DisplayName="射击模式"))
	EYcFireMode FireMode;

	/** 连射数量（仅连射模式有效） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|射速", 
		meta=(DisplayName="连射数量", EditCondition="FireMode==EYcFireMode::BurstAuto||FireMode==EYcFireMode::BurstSingle", ClampMin=2, ClampMax=10))
	int32 BurstCount;

	/** 连射间隔倍率（连射组之间的间隔 = 射击间隔 × 此倍率） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础射击|射速", 
		meta=(DisplayName="连射间隔倍率", EditCondition="FireMode==EYcFireMode::BurstAuto||FireMode==EYcFireMode::BurstSingle", ClampMin=1.0, ClampMax=10.0))
	float BurstIntervalMultiplier;


	// ════════════════════════════════════════════════════════════════════════
	// 伤害参数
	// ════════════════════════════════════════════════════════════════════════
	
	/** 基础伤害值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.伤害|基础", 
		meta=(DisplayName="基础伤害", ClampMin=0.0))
	float BaseDamage;
	
	/** 
	 * 伤害衰减曲线
	 * X轴：距离百分比（0-1，相对于MaxDamageRange）
	 * Y轴：伤害百分比（0-1）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.伤害|衰减", 
		meta=(DisplayName="伤害衰减曲线"))
	FRuntimeFloatCurve DamageFalloffCurve;

	// ════════════════════════════════════════════════════════════════════════
	// 命中区域伤害倍率（基于 GameplayTag）
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 命中区域伤害倍率映射表
	 * Key: 命中区域 GameplayTag（如 Gameplay.Character.Zone.Head）
	 * Value: 伤害倍率（1.0 = 基础伤害，2.0 = 双倍伤害）
	 * 
	 * 工作原理：
	 * 1. 射线检测命中后，从 HitResult.PhysMaterial 获取物理材质
	 * 2. 转换为 UYcPhysicalMaterialWithTags，读取其 Tags
	 * 3. 遍历此映射表，查找匹配的 Tag
	 * 4. 应用对应的伤害倍率
	 * 
	 * 配置示例：
	 * - Gameplay.Character.Zone.Head      : 2.0  (爆头双倍伤害)
	 * - Gameplay.Character.Zone.Body      : 1.0  (身体基础伤害)
	 * - Gameplay.Character.Zone.Limb      : 0.8  (四肢减少伤害)
	 * - Gameplay.Character.Zone.Weakness  : 3.0  (弱点三倍伤害)
	 * 
	 * 注意：
	 * - 如果物理材质的 Tag 不在映射表中，使用默认倍率 1.0
	 * - 如果物理材质有多个匹配的 Tag，使用第一个匹配的倍率
	 * - HeadshotMultiplier 已废弃，请使用此映射表配置头部伤害
	 * - 配件可以修改这些倍率（如穿甲弹提升护甲部位伤害）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.伤害|命中区域倍率", 
		meta=(DisplayName="命中区域伤害倍率表", Categories="Gameplay.Character.Zone"))
	TMap<FGameplayTag, float> HitZoneDamageMultipliers;

	// ════════════════════════════════════════════════════════════════════════
	// 弹药参数
	// ════════════════════════════════════════════════════════════════════════
	
	/** 弹匣容量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="3.弹药|弹匣", 
		meta=(DisplayName="弹匣容量", ClampMin=1))
	int32 MagazineSize;

	/** 最大备用弹药 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="3.弹药|弹匣", 
		meta=(DisplayName="最大备弹", ClampMin=0))
	int32 MaxReserveAmmo;

	/** 换弹时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="3.弹药|换弹", 
		meta=(DisplayName="换弹时间(秒)", ClampMin=0.1))
	float ReloadTime;

	/** 战术换弹时间（弹匣未空时，秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="3.弹药|换弹", 
		meta=(DisplayName="战术换弹时间(秒)", ClampMin=0.1))
	float TacticalReloadTime;


	// ════════════════════════════════════════════════════════════════════════
	// 腰射扩散参数 (Hip Fire Spread)
	// 腰射时子弹有随机扩散，扩散角度会随连续射击增加
	// ════════════════════════════════════════════════════════════════════════
	
	/** 
	 * 腰射基础扩散角度（度）
	 * 静止站立时的最小扩散
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|腰射", 
		meta=(DisplayName="基础扩散(度)", ClampMin=0.0, ClampMax=45.0))
	float HipFireBaseSpread;

	/** 
	 * 腰射最大扩散角度（度）
	 * 连续射击时扩散的上限
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|腰射", 
		meta=(DisplayName="最大扩散(度)", ClampMin=0.0, ClampMax=45.0))
	float HipFireMaxSpread;

	/** 
	 * 每发子弹增加的扩散角度（度）
	 * 连续射击时扩散累积速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|腰射", 
		meta=(DisplayName="每发增加扩散(度)", ClampMin=0.0))
	float HipFireSpreadPerShot;

	// ════════════════════════════════════════════════════════════════════════
	// 瞄准扩散参数 (ADS Spread)
	// 瞄准时子弹无扩散（或极小扩散），瞄哪打哪
	// ════════════════════════════════════════════════════════════════════════
	
	/** 
	 * 瞄准时基础扩散角度（度）
	 * COD风格通常为0，实现瞄哪打哪
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|瞄准(ADS)", 
		meta=(DisplayName="基础扩散(度)", ClampMin=0.0, ClampMax=10.0))
	float ADSBaseSpread;

	/** 
	 * 瞄准时最大扩散角度（度）
	 * 连续射击时的扩散上限，通常很小或为0
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|瞄准(ADS)", 
		meta=(DisplayName="最大扩散(度)", ClampMin=0.0, ClampMax=10.0))
	float ADSMaxSpread;

	/** 
	 * 瞄准时每发增加的扩散（度）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|瞄准(ADS)", 
		meta=(DisplayName="每发增加扩散(度)", ClampMin=0.0))
	float ADSSpreadPerShot;


	// ════════════════════════════════════════════════════════════════════════
	// 扩散恢复与状态乘数
	// ════════════════════════════════════════════════════════════════════════

	/** 
	 * 扩散恢复速度（度/秒）
	 * 停止射击后扩散恢复到基础值的速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|恢复", 
		meta=(DisplayName="恢复速度(度/秒)", ClampMin=0.0))
	float SpreadRecoveryRate;

	/** 
	 * 扩散恢复延迟（秒）
	 * 停止射击后多久开始恢复扩散
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|恢复", 
		meta=(DisplayName="恢复延迟(秒)", ClampMin=0.0))
	float SpreadRecoveryDelay;

	/** 
	 * 扩散指数
	 * 控制弹着点在扩散范围内的分布：
	 * - 1.0：均匀分布
	 * - >1.0：更集中于中心
	 * - <1.0：更分散于边缘
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|分布", 
		meta=(DisplayName="扩散指数", ClampMin=0.1, ClampMax=10.0))
	float SpreadExponent;

	// ════════════════════════════════════════════════════════════════════════
	// 状态扩散乘数（仅影响腰射）
	// ════════════════════════════════════════════════════════════════════════

	/** 移动时的扩散乘数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|状态乘数", 
		meta=(DisplayName="移动乘数", ClampMin=1.0, ClampMax=5.0))
	float MovingSpreadMultiplier;

	/** 跳跃/下落时的扩散乘数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|状态乘数", 
		meta=(DisplayName="跳跃乘数", ClampMin=1.0, ClampMax=10.0))
	float JumpingSpreadMultiplier;

	/** 蹲下时的扩散乘数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|状态乘数", 
		meta=(DisplayName="蹲下乘数", ClampMin=0.1, ClampMax=1.0))
	float CrouchingSpreadMultiplier;


	// ════════════════════════════════════════════════════════════════════════
	// 首发精准度
	// ════════════════════════════════════════════════════════════════════════
	
	/** 是否启用首发精准度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|首发精准", 
		meta=(DisplayName="启用首发精准"))
	bool bEnableFirstShotAccuracy;

	/** 
	 * 首发精准度激活所需的静止时间（秒）
	 * 玩家静止多久后激活首发精准度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|首发精准", 
		meta=(DisplayName="激活时间(秒)", EditCondition="bEnableFirstShotAccuracy", ClampMin=0.0))
	float FirstShotAccuracyTime;

	/** 
	 * 首发精准度是否仅在腰射时生效
	 * true：仅腰射时有首发精准度
	 * false：瞄准和腰射都有首发精准度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.扩散|首发精准", 
		meta=(DisplayName="仅腰射生效", EditCondition="bEnableFirstShotAccuracy"))
	bool bFirstShotAccuracyHipFireOnly;

	// ════════════════════════════════════════════════════════════════════════
	// 后坐力 - 弹道轨迹模式
	// 后坐力影响玩家视角，有固定的弹道轨迹
	// 瞄准时后坐力更明显，腰射时后坐力较小
	// ════════════════════════════════════════════════════════════════════════
	
	/** 
	 * 是否使用固定弹道轨迹
	 * true：使用RecoilPattern数组定义的固定轨迹（可学习的后坐力）
	 * false：使用随机后坐力范围
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|模式", 
		meta=(DisplayName="使用固定弹道轨迹"))
	bool bUseRecoilPattern;

	/** 
	 * 固定弹道轨迹
	 * 每个元素定义一发子弹的后坐力偏移（Pitch, Yaw）
	 * 超出数组长度后循环使用最后几个点
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|弹道轨迹", 
		meta=(DisplayName="弹道轨迹点", EditCondition="bUseRecoilPattern", TitleProperty="Pitch={Pitch}, Yaw={Yaw}"))
	TArray<FYcRecoilPatternPoint> RecoilPattern;

	/** 
	 * 弹道轨迹循环起始索引
	 * 当射击次数超过RecoilPattern长度时，从此索引开始循环
	 * 例如：前10发有固定轨迹，之后循环第8-10发的轨迹
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|弹道轨迹", 
		meta=(DisplayName="循环起始索引", EditCondition="bUseRecoilPattern", ClampMin=0))
	int32 RecoilPatternLoopStart;


	// ════════════════════════════════════════════════════════════════════════
	// 后坐力 - 随机模式
	// 当不使用固定弹道轨迹时，使用随机范围
	// ════════════════════════════════════════════════════════════════════════
	
	/** 垂直后坐力最小值（度，正值向上） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|随机范围", 
		meta=(DisplayName="垂直最小(度)", EditCondition="!bUseRecoilPattern", ClampMin=0.0))
	float VerticalRecoilMin;

	/** 垂直后坐力最大值（度） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|随机范围", 
		meta=(DisplayName="垂直最大(度)", EditCondition="!bUseRecoilPattern", ClampMin=0.0))
	float VerticalRecoilMax;

	/** 水平后坐力最小值（度，负值向左） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|随机范围", 
		meta=(DisplayName="水平最小(度)", EditCondition="!bUseRecoilPattern"))
	float HorizontalRecoilMin;

	/** 水平后坐力最大值（度，正值向右） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|随机范围", 
		meta=(DisplayName="水平最大(度)", EditCondition="!bUseRecoilPattern"))
	float HorizontalRecoilMax;

	// ════════════════════════════════════════════════════════════════════════
	// 后坐力 - 状态乘数
	// ════════════════════════════════════════════════════════════════════════
	
	/** 
	 * 瞄准时后坐力乘数
	 * COD风格通常为1.0或更高，瞄准时后坐力更明显
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|状态乘数", 
		meta=(DisplayName="瞄准乘数", ClampMin=0.1, ClampMax=3.0))
	float ADSRecoilMultiplier;

	/** 
	 * 腰射时后坐力乘数
	 * 通常小于1.0，腰射时后坐力较小
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|状态乘数", 
		meta=(DisplayName="腰射乘数", ClampMin=0.1, ClampMax=2.0))
	float HipFireRecoilMultiplier;

	/** 蹲下时后坐力乘数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|状态乘数", 
		meta=(DisplayName="蹲下乘数", ClampMin=0.1, ClampMax=1.0))
	float CrouchingRecoilMultiplier;


	// ════════════════════════════════════════════════════════════════════════
	// 后坐力 - 恢复
	// ════════════════════════════════════════════════════════════════════════
	
	/** 
	 * 后坐力恢复速度（度/秒）
	 * 停止射击后视角恢复的速度
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|恢复", 
		meta=(DisplayName="恢复速度(度/秒)", ClampMin=0.0))
	float RecoilRecoveryRate;

	/** 
	 * 后坐力恢复延迟（秒）
	 * 停止射击后多久开始恢复
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|恢复", 
		meta=(DisplayName="恢复延迟(秒)", ClampMin=0.0))
	float RecoilRecoveryDelay;

	/** 
	 * 后坐力恢复百分比（0-1）
	 * 自动恢复的后坐力比例，1.0表示完全恢复
	 * COD风格通常不完全恢复，需要玩家手动压枪
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.后坐力|恢复", 
		meta=(DisplayName="恢复百分比", ClampMin=0.0, ClampMax=1.0))
	float RecoilRecoveryPercent;

	// ════════════════════════════════════════════════════════════════════════
	// 瞄准参数 (ADS)
	// ════════════════════════════════════════════════════════════════════════
	
	/** 瞄准速度（进入ADS的时间，秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6.瞄准(ADS)", 
		meta=(DisplayName="瞄准时间(秒)", ClampMin=0.05, ClampMax=1.0))
	float AimDownSightTime;

	/** 瞄准时的FOV倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6.瞄准(ADS)", 
		meta=(DisplayName="FOV倍率", ClampMin=0.1, ClampMax=1.0))
	float ADSFOVMultiplier;

	/** 瞄准时的移动速度乘数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6.瞄准(ADS)", 
		meta=(DisplayName="移动速度乘数", ClampMin=0.1, ClampMax=1.0))
	float ADSMoveSpeedMultiplier;

	// ════════════════════════════════════════════════════════════════════════
	// Fragment 接口
	// ════════════════════════════════════════════════════════════════════════
	
	virtual FString GetDebugString() const override
	{
		return FString::Printf(TEXT("WeaponStats: Damage=%.1f, FireRate=%.0f, MagSize=%d"), 
			BaseDamage, FireRate, MagazineSize);
	}
};

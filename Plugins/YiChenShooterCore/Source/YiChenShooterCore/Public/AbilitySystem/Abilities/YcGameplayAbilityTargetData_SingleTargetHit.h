// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "YcGameplayAbilityTargetData_SingleTargetHit.generated.h"

/**
 * FYcGameplayAbilityTargetData_SingleTargetHit - 单目标命中数据结构体
 * 
 * 继承自 GAS 基类 FGameplayAbilityTargetData_SingleTargetHit，用于封装单次命中的完整信息。
 * 这可用于武器射击系统中服务端与客户端间数据传递的核心结构体，承担着从射线检测到伤害应用的数据桥梁作用。
 * 核心是通过ASC组件配合PredictionKey和RPC实现的数据同步, 详情可见ASC的AbilityTargetDataMap属性相关代码
 * 
 * 主要职责：
 * 1. 存储射线检测的命中结果 (HitResult，继承自基类)
 * 2. 存储弹匣ID，用于识别来自同一次射击的多个弹丸（如霰弹枪）
 * 3. 支持网络序列化，确保客户端和服务器之间正确传输数据
 * 4. 将目标数据传递到 GameplayEffect 上下文中
 * 
 * 继承关系：
 * FGameplayAbilityTargetData (GAS基类)
 *     └── FGameplayAbilityTargetData_SingleTargetHit (单目标命中基类)
 *             ├── FHitResult HitResult      // 命中结果
 *             └── bool bHitReplaced         // 服务器替换标记
 *                     └── FYcGameplayAbilityTargetData_SingleTargetHit (项目扩展)
 *                             └── int32 CartridgeID  // 弹匣ID
 * 
 * 使用场景：
 * - 在 UYcGameplayAbility_HitScanWeapon::StartHitScanWeaponTargeting() 中创建，封装射线检测结果
 * - 通过 FGameplayAbilityTargetDataHandle 容器传递
 * - 在 OnRangedWeaponTargetDataReady() 蓝图事件中使用
 * - 应用 GameplayEffect 时，通过 AddTargetDataToContext() 传递到效果上下文
 */
USTRUCT()
struct YICHENSHOOTERCORE_API FYcGameplayAbilityTargetData_SingleTargetHit : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	/**
	 * 默认构造函数
	 * 将 CartridgeID 初始化为 -1，表示无效/未设置
	 */
	FYcGameplayAbilityTargetData_SingleTargetHit()
		: CartridgeID(-1)
	{ }

	// ==================== 必须重写的接口 ====================
	
	/**
	 * 将目标数据添加到 GameplayEffect 上下文中
	 * 
	 * 这是最关键的函数，它在 GAS 应用 GameplayEffect 时被自动调用。
	 * 负责将目标数据中的信息（如 HitResult、CartridgeID）传递到 FYcGameplayEffectContext 中，
	 * 使得后续的伤害计算、效果应用等逻辑可以访问这些数据。
	 * 
	 * 调用时机：
	 * - 当技能调用 ApplyGameplayEffectToTarget() 时
	 * - GAS 内部会自动调用此函数来填充效果上下文
	 * 
	 * @param Context - GameplayEffect 上下文句柄，用于存储传递的数据
	 * @param bIncludeActorArray - 是否包含 Actor 数组（通常为 true）
	 */
	virtual void AddTargetDataToContext(FGameplayEffectContextHandle& Context, bool bIncludeActorArray) const override;
	
	/**
	 * 获取此结构体的脚本结构体信息
	 * 
	 * 用于 UE 反射系统和序列化系统识别此结构体的类型。
	 * 必须重写以返回正确的类型信息，否则网络序列化和类型转换会失败。
	 * 
	 * @return 此结构体的 UScriptStruct 指针
	 */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FYcGameplayAbilityTargetData_SingleTargetHit::StaticStruct();
	}

	// ==================== 项目特定数据 ====================
	
	/**
	 * 弹匣ID - 用于识别来自同一次射击的多个弹丸
	 * 
	 * 应用场景：
	 * 1. 霰弹枪：单次射击产生多颗弹丸，它们共享相同的 CartridgeID
	 * 2. 伤害计算：可以根据 CartridgeID 判断是否为同一次射击，避免重复计算某些效果
	 * 3. 统计和日志：追踪单次射击的所有命中结果
	 * 
	 * 值说明：
	 * - -1：无效/未设置
	 * - >= 0：有效的弹匣ID（通常使用 FMath::Rand() 生成）
	 */
	UPROPERTY()
	int32 CartridgeID;

	// ==================== 网络序列化 ====================
	
	/**
	 * 网络序列化函数
	 * 
	 * 确保此结构体可以在客户端和服务器之间正确传输。
	 * 会先调用基类的序列化函数处理 HitResult 等基础数据，
	 * 然后序列化项目特定的 CartridgeID。
	 * 
	 * @param Ar - 序列化存档
	 * @param Map - 包映射表
	 * @param bOutSuccess - 输出参数，表示序列化是否成功
	 * @return 序列化是否成功
	 */
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

/**
 * 类型特性声明
 * 
 * 告诉 UE 反射系统此结构体支持网络序列化。
 * WithNetSerializer = true 是必须的，否则 FGameplayAbilityTargetDataHandle 
 * 的网络序列化将无法正常工作。
 */
template<>
struct TStructOpsTypeTraits<FYcGameplayAbilityTargetData_SingleTargetHit> : public TStructOpsTypeTraitsBase2<FYcGameplayAbilityTargetData_SingleTargetHit>
{
	enum
	{
		/** 
		 * 启用网络序列化支持
		 * 这是 FGameplayAbilityTargetDataHandle 网络序列化正常工作的必要条件
		 */
		WithNetSerializer = true,
	};
};

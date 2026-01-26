// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Components/ControllerComponent.h"
#include "YcWeaponStateComponent.generated.h"

class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;
struct FGameplayEffectContextHandle;
struct FHitResult;

/**
 * 屏幕空间命中位置
 * 用于在准星上显示HitScan武器命中标记
 * 成功命中标记表示对敌人造成了伤害
 */
USTRUCT(BlueprintType)
struct FYcScreenSpaceHitLocation
{
	GENERATED_BODY()
	
	/** 视口屏幕空间中的命中位置 */
	UPROPERTY(BlueprintReadOnly)
	FVector2D Location = FVector2D::ZeroVector;
	
	/** 命中区域（头部/身体/四肢等） */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag HitZone;
	
	/** 是否显示为成功命中 */
	UPROPERTY(BlueprintReadOnly)
	bool bShowAsSuccess = false;
};

/**
 * 服务器端命中标记批次
 * 存储未经服务器确认的命中标记数据
 * 用于客户端预测和服务器验证的同步机制
 */
USTRUCT(BlueprintType)
struct FYcServerSideHitMarkerBatch
{
	GENERATED_BODY()
	
	FYcServerSideHitMarkerBatch() { }

	FYcServerSideHitMarkerBatch(uint8 InUniqueId) :
		UniqueId(InUniqueId)
	{ }

	/** 该批次中的所有命中标记 */
	UPROPERTY(BlueprintReadOnly)
	TArray<FYcScreenSpaceHitLocation> Markers;
	
	/** 唯一标识符，用于网络同步时匹配客户端和服务器数据 */
	UPROPERTY()
	uint8 UniqueId = 0;
};


/**
 * 武器状态组件
 * 
 * 挂载在 PlayerController 上，负责：
 * - 跟踪武器状态
 * - 管理命中标记的显示（用于准星反馈）
 * - 处理客户端预测命中与服务器确认的同步
 * - 驱动射线武器实例的 Tick 更新
 * 
 * 工作流程：
 * 1. 客户端射击时，AddUnconfirmedServerSideHitMarkers 添加未确认的命中标记
 * 2. 服务器验证后，通过 ClientConfirmTargetData RPC 确认或拒绝命中
 * 3. 确认的命中会更新 LastWeaponDamageScreenLocations 用于 UI 显示
 */
UCLASS(ClassGroup=(YiChenShooterCore), meta=(BlueprintSpawnableComponent))
class YICHENSHOOTERCORE_API UYcWeaponStateComponent : public UControllerComponent
{
	GENERATED_BODY()
	
public:
	UYcWeaponStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 客户端 RPC：服务器确认目标数据
	 * @param UniqueId - 命中批次的唯一标识符
	 * @param bSuccess - 服务器是否确认命中有效
	 * @param HitReplaces - 需要替换/移除的命中索引数组
	 */
	UFUNCTION(Client, Reliable)
	void ClientConfirmTargetData(uint16 UniqueId, bool bSuccess, const TArray<uint8>& HitReplaces);

	/**
	 * 添加未确认的服务器端命中标记
	 * 在客户端射击时调用，将命中结果转换为屏幕空间坐标并存储
	 * @param InTargetData - 技能目标数据句柄，包含唯一标识符
	 * @param FoundHits - 射线检测到的命中结果数组
	 */
	void AddUnconfirmedServerSideHitMarkers(const FGameplayAbilityTargetDataHandle& InTargetData, const TArray<FHitResult>& FoundHits);

	/**
	 * 更新伤害造成时间
	 * 用于追踪玩家最后一次造成伤害的时间，驱动命中反馈 UI
	 * @param EffectContext - 游戏效果上下文，包含伤害来源信息
	 */
	void UpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext);

	/**
	 * 获取最近造成伤害的屏幕位置数组
	 * 用于 UI 显示命中标记
	 * @param WeaponDamageScreenLocations - 输出参数，填充命中位置数组
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	void GetLastWeaponDamageScreenLocations(TArray<FYcScreenSpaceHitLocation>& WeaponDamageScreenLocations) const
	{
		WeaponDamageScreenLocations = LastWeaponDamageScreenLocations;
	}

	/**
	 * 获取自上次命中通知以来经过的时间
	 * @return 经过的秒数
	 */
	double GetTimeSinceLastHitNotification() const;

	/**
	 * 获取未确认的服务器端命中标记数量
	 * @return 当前等待服务器确认的命中批次数量
	 */
	int32 GetUnconfirmedServerSideHitMarkerCount() const
	{
		return UnconfirmedServerSideHitMarkers.Num();
	}

protected:
	/**
	 * 判断命中是否应显示为成功
	 * 默认行为：如果命中的是与控制器 Pawn 不同队伍的角色，则视为成功
	 * @param Hit - 命中结果
	 * @return 是否应显示为成功命中
	 */
	virtual bool ShouldShowHitAsSuccess(const FHitResult& Hit) const;

	/**
	 * 判断是否应更新伤害造成时间
	 * 用于过滤只关心武器或武器发射的投射物造成的伤害
	 * @param EffectContext - 游戏效果上下文
	 * @return 是否应更新时间
	 */
	virtual bool ShouldUpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext) const;

	/** 实际执行伤害造成时间的更新 */
	void ActuallyUpdateDamageInstigatedTime();

private:
	/** 控制器最后一次造成武器伤害的时间 */
	double LastWeaponDamageInstigatedTime = 0.0;

	/**
	 * 最近造成武器伤害的屏幕空间位置（已确认的命中）
	 * 在 ActuallyUpdateDamageInstigatedTime 中检测时间是否超过 0.1 秒
	 * 超过后会自动清除
	 */
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TArray<FYcScreenSpaceHitLocation> LastWeaponDamageScreenLocations;

	/** 未被服务端确认的命中标记批次数组 */
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TArray<FYcServerSideHitMarkerBatch> UnconfirmedServerSideHitMarkers;
};

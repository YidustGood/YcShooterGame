// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YiChenEquipment/Public/YcEquipmentInstance.h"
#include "YcWeaponInstance.generated.h"

/**
 * UYcWeaponInstance - 武器实例基类
 * 
 * 该类继承自装备实例(UYcEquipmentInstance)，作为所有武器类型的基类。
 * 主要职责：
 * - 管理武器的装备/卸载生命周期
 * - 追踪武器的运行时动态数据（如:装备时间、开火时间、实时后坐力数据等）
 * - 支持网络复制，确保多人游戏中武器状态同步
 * - 管理输入设备属性（如手柄震动等）
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcWeaponInstance : public UYcEquipmentInstance
{
	GENERATED_BODY()
public:
	UYcWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 重写网络支持检查
	 * 返回true表示该对象支持网络复制，这对于多人游戏中的武器状态同步至关重要
	 * @return 始终返回true，表示支持网络复制
	 */
	virtual bool IsSupportedForNetworking() const override { return true; };

	//~UYcEquipmentInstance 接口
	/**
	 * 武器被装备时调用
	 * 可在子类中重写以添加装备时的特殊逻辑（如播放装备动画、应用设备属性等）
	 */
	virtual void OnEquipped() override;

	/**
	 * 武器被卸下时调用
	 * 可在子类中重写以添加卸载时的清理逻辑（如移除设备属性、重置状态等）
	 */
	virtual void OnUnequipped() override;
	//~End of UYcEquipmentInstance 接口

	/**
	 * 更新开火时间记录
	 * 每次武器开火时调用此函数，记录当前世界时间作为最后开火时间
	 * 用于计算武器空闲时间，可用于实现武器扩散恢复等机制
	 */
	UFUNCTION(BlueprintCallable)
	void UpdateFiringTime();

	/**
	 * 获取自上次与武器交互以来经过的时间
	 * 交互包括：射击、装备等操作
	 * 可用于实现武器空闲动画、扩散恢复等功能
	 * @return 自上次交互以来的秒数
	 */
	UFUNCTION(BlueprintPure)
	float GetTimeSinceLastInteractedWith() const;
	
private:
	/**
	 * 由该武器激活的输入设备属性句柄集合
	 * 用于管理手柄震动、触觉反馈等输入设备特性
	 * 由ApplyDeviceProperties函数填充
	 * Transient标记表示该属性不会被序列化保存
	 */
	UPROPERTY(Transient)
	TSet<FInputDevicePropertyHandle> DevicePropertyHandles;

	/** 武器被装备的时间戳（世界时间秒数） */
	double TimeLastEquipped = 0.0;

	/** 武器最后一次开火的时间戳（世界时间秒数） */
	double TimeLastFired = 0.0;
};

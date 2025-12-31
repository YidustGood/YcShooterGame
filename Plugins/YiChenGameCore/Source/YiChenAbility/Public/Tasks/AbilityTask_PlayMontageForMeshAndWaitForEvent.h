// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_PlayMontageForMeshAndWaitForEvent.generated.h"

class UYcAbilitySystemComponent;

/**
 * 蒙太奇播放事件委托类型
 * 用于在蒙太奇播放的各个阶段（完成、混出、中断、取消）以及接收到GameplayEvent时触发回调
 * @param EventTag 触发的GameplayTag，如果来自蒙太奇回调则可能为空
 * @param EventData 事件数据载荷，如果来自蒙太奇回调则可能为空
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSPlayMontageForMeshAndWaitForEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
 * 在指定骨骼网格上播放蒙太奇并等待事件的AbilityTask
 * 
 * 功能概述：
 * 该Task将PlayMontageAndWait和WaitForEvent两个Task的功能合并为一个，
 * 支持在播放蒙太奇的同时监听GameplayEvent，非常适合实现近战连击等需要多种激活方式的技能。
 * 
 * 网络复制机制：
 * - 服务器(Authority)：播放蒙太奇后更新RepAnimMontageInfoForMeshes复制属性，自动同步到所有客户端
 * - 主控端(AutonomousProxy)：本地预测播放，通过PredictionKey机制处理服务器拒绝时的回滚
 * - 模拟端(SimulatedProxy)：通过OnRep_ReplicatedAnimMontageForMesh接收复制数据并播放
 * 
 * 使用场景：
 * - 近战攻击动画播放
 * - 技能释放动画
 * - 需要在动画特定时机触发效果的技能（通过AnimNotify发送GameplayEvent）
 * 
 * 这是一个很好的自定义AbilityTask示例，每个游戏项目都应该根据需求创建自己的Task集合
 */
UCLASS()
class YICHENABILITY_API UAbilityTask_PlayMontageForMeshAndWaitForEvent : public UAbilityTask
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * @param ObjectInitializer 对象初始化器
	 */
	UAbilityTask_PlayMontageForMeshAndWaitForEvent(const FObjectInitializer& ObjectInitializer);

	/**
	 * 激活Task，开始播放蒙太奇
	 * 
	 * 执行流程：
	 * 1. 绑定GameplayEvent监听器，用于接收技能事件
	 * 2. 调用ASC的PlayMontageForMesh在指定Mesh上播放蒙太奇
	 * 3. 绑定蒙太奇的BlendingOut和Ended回调
	 * 4. 设置RootMotion缩放（仅在Authority或LocalPredicted的AutonomousProxy上）
	 * 
	 * 注意：蓝图节点会通过K2Node_LatentAbilityCall自动调用Activate()，
	 * 但C++代码中使用此Task时需要手动调用Activate()
	 */
	virtual void Activate() override;
	
	/**
	 * 外部取消Task
	 * 当技能被外部取消时调用，会停止蒙太奇播放并触发OnCancelled委托
	 */
	virtual void ExternalCancel() override;
	
	/**
	 * 获取调试字符串
	 * @return 包含当前播放蒙太奇信息的调试字符串
	 */
	virtual FString GetDebugString() const override;
	
	/**
	 * Task销毁时的清理函数
	 * 负责解绑所有委托、停止蒙太奇（如果bStopWhenAbilityEnds为true）、移除GameplayEvent监听
	 * @param AbilityEnded 技能是否已结束
	 */
	virtual void OnDestroy(bool AbilityEnded) override;

	/**
	 * 蒙太奇完全播放完成时触发
	 * 当蒙太奇正常播放到结束（非中断、非取消）时调用
	 * EventTag和EventData参数为空
	 */
	UPROPERTY(BlueprintAssignable)
	FGSPlayMontageForMeshAndWaitForEventDelegate OnCompleted;

	/**
	 * 蒙太奇开始混出时触发
	 * 当蒙太奇进入BlendOut阶段时调用（正常结束前的过渡）
	 * EventTag和EventData参数为空
	 */
	UPROPERTY(BlueprintAssignable)
	FGSPlayMontageForMeshAndWaitForEventDelegate OnBlendOut;

	/**
	 * 蒙太奇被中断时触发
	 * 当另一个蒙太奇覆盖当前蒙太奇时调用
	 * EventTag和EventData参数为空
	 */
	UPROPERTY(BlueprintAssignable)
	FGSPlayMontageForMeshAndWaitForEventDelegate OnInterrupted;

	/**
	 * Task被显式取消时触发
	 * 当技能被其他技能取消或调用ExternalCancel时调用
	 * EventTag和EventData参数为空
	 */
	UPROPERTY(BlueprintAssignable)
	FGSPlayMontageForMeshAndWaitForEventDelegate OnCancelled;

	/**
	 * 接收到匹配的GameplayEvent时触发
	 * 当收到与EventTags匹配的GameplayEvent时调用
	 * 常用于AnimNotify发送的事件，实现动画特定时机的效果触发
	 * @param EventTag 触发的事件标签
	 * @param EventData 事件携带的数据
	 */
	UPROPERTY(BlueprintAssignable)
	FGSPlayMontageForMeshAndWaitForEventDelegate EventReceived;

	/**
	 * 在指定骨骼网格上播放蒙太奇并等待事件, Mesh和MestTag两种指定目标骨骼网格组件的参数二选一即可
	 * 
	 * 功能说明：
	 * 播放蒙太奇并等待其结束。如果收到与EventTags匹配的GameplayEvent（或EventTags为空时任意事件），
	 * 将触发EventReceived委托并传递事件标签和数据。
	 * 
	 * 网络行为：
	 * - bReplicateMontage=true时，服务器会将蒙太奇信息复制到所有模拟客户端
	 * - 主控客户端使用预测机制，本地立即播放，服务器拒绝时自动回滚
	 * 
	 * 回调触发时机：
	 * - OnBlendOut: 蒙太奇开始混出时
	 * - OnCompleted: 蒙太奇完全播放完成时
	 * - OnInterrupted: 被其他蒙太奇覆盖时
	 * - OnCancelled: 技能或Task被取消时
	 * - EventReceived: 收到匹配的GameplayEvent时
	 *
	 * @param OwningAbility 拥有此Task的技能实例
	 * @param TaskInstanceName Task实例名称，用于后续查询
	 * @param Mesh 要播放蒙太奇的骨骼网格组件，必须属于AvatarActor, Mesh和MestTag两种指定目标组件的参数二选一即可
	 * @param MeshTag 播放蒙太奇的目标骨骼网格组件Tag, 如果不直接传入Mesh也可以选择传入目标Mesh组件拥有的Tag, 然后在AvatarActor上通过Tag查找到Mesh组件
	 * @param MontageToPlay 要播放的蒙太奇资源
	 * @param EventTags 要监听的GameplayEvent标签容器，为空时监听所有事件
	 * @param Rate 播放速率，大于1加速，小于1减速
	 * @param StartSection 起始Section名称，NAME_None从头开始播放
	 * @param bStopWhenAbilityEnds 技能正常结束时是否停止蒙太奇（技能被取消时总是会停止）
	 * @param AnimRootMotionTranslationScale RootMotion位移缩放，设为0可完全禁用RootMotion
	 * @param bReplicateMontage 是否复制蒙太奇到其他客户端（网络同步）
	 * @param OverrideBlendOutTimeForCancelAbility 技能取消时的混出时间覆盖，-1使用蒙太奇默认值
	 * @param OverrideBlendOutTimeForStopWhenEndAbility 技能结束停止时的混出时间覆盖，-1使用蒙太奇默认值
	 * @return 创建的Task实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_PlayMontageForMeshAndWaitForEvent* PlayMontageForMeshAndWaitForEvent(
			UGameplayAbility* OwningAbility,
			FName TaskInstanceName,
			USkeletalMeshComponent* Mesh,
			FName MeshTag,
			UAnimMontage* MontageToPlay,
			FGameplayTagContainer EventTags,
			float Rate = 1.f,
			FName StartSection = NAME_None,
			bool bStopWhenAbilityEnds = true,
			float AnimRootMotionTranslationScale = 1.f,
			bool bReplicateMontage = true,
			float OverrideBlendOutTimeForCancelAbility = -1.f,
			float OverrideBlendOutTimeForStopWhenEndAbility = -1.f);

private:
	///////////////// 蒙太奇播放参数 /////////////////
	
	/**
	 * 播放蒙太奇的目标骨骼网格组件
	 * 必须属于AvatarActor，用于支持多Mesh角色（如FPS第一人称角色Mesh）
	 */
	UPROPERTY()
	USkeletalMeshComponent* Mesh;
	
	/**
	 * 播放蒙太奇的目标骨骼网格组件Tag, 如果不直接传入Mesh也可以选择传入目标Mesh组件拥有的Tag, 然后通过Tag查找到Mesh组件
	 * 必须属于AvatarActor，用于支持多Mesh角色（如FPS第一人称角色Mesh）
	 */
	UPROPERTY()
	FName MeshTag;

	/** 要播放的蒙太奇资源 */
	UPROPERTY()
	UAnimMontage* MontageToPlay;

	/**
	 * 要监听的GameplayEvent标签容器
	 * 当收到匹配的事件时触发EventReceived委托
	 * 为空时监听所有GameplayEvent
	 */
	UPROPERTY()
	FGameplayTagContainer EventTags;

	/** 蒙太奇播放速率，1.0为正常速度 */
	UPROPERTY()
	float Rate;

	/** 蒙太奇起始Section名称，NAME_None从头开始 */
	UPROPERTY()
	FName StartSection;

	/**
	 * RootMotion位移缩放系数
	 * 用于调整RootMotion的移动距离，设为0可完全禁用RootMotion
	 */
	UPROPERTY()
	float AnimRootMotionTranslationScale;

	/**
	 * 技能正常结束时是否停止蒙太奇
	 * 注意：技能被显式取消时总是会停止蒙太奇，此选项仅影响正常结束的情况
	 */
	UPROPERTY()
	bool bStopWhenAbilityEnds;

	/**
	 * 是否将蒙太奇复制到其他客户端
	 * true: 服务器会更新RepAnimMontageInfoForMeshes，自动同步到模拟客户端
	 * false: 蒙太奇仅在本地播放，不进行网络同步
	 */
	UPROPERTY()
	bool bReplicateMontage;

	/**
	 * 技能取消时的混出时间覆盖
	 * -1: 使用蒙太奇资源的默认BlendOut时间
	 * >=0: 使用指定的混出时间（秒）
	 */
	UPROPERTY()
	float OverrideBlendOutTimeForCancelAbility;

	/**
	 * 技能结束停止蒙太奇时的混出时间覆盖
	 * -1: 使用蒙太奇资源的默认BlendOut时间
	 * >=0: 使用指定的混出时间（秒）
	 */
	UPROPERTY()
	float OverrideBlendOutTimeForStopWhenEndAbility;

	///////////////// 内部辅助函数 /////////////////
	
	/**
	 * 停止当前播放的蒙太奇
	 * 检查技能是否正在播放蒙太奇，如果是则停止它
	 * @param OverrideBlendOutTime 混出时间覆盖，-1使用默认值
	 * @return 是否成功停止了蒙太奇
	 */
	bool StopPlayingMontage(float OverrideBlendOutTime = -1.f);

	/**
	 * 获取目标AbilitySystemComponent
	 * @return 转换为UYcAbilitySystemComponent的ASC指针
	 */
	UYcAbilitySystemComponent* GetTargetASC();

	///////////////// 蒙太奇回调函数 /////////////////
	
	/**
	 * 蒙太奇开始混出时的回调
	 * 根据是否被中断触发OnInterrupted或OnBlendOut委托
	 * @param Montage 混出的蒙太奇
	 * @param bInterrupted 是否因中断而混出
	 */
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	
	/**
	 * 技能被取消时的回调
	 * 停止蒙太奇播放并触发OnCancelled委托
	 */
	void OnAbilityCancelled();
	
	/**
	 * 蒙太奇播放结束时的回调
	 * 如果非中断结束则触发OnCompleted委托，然后结束Task
	 * @param Montage 结束的蒙太奇
	 * @param bInterrupted 是否因中断而结束
	 */
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	/**
	 * 收到GameplayEvent时的回调
	 * 触发EventReceived委托，传递事件标签和数据
	 * @param EventTag 事件标签
	 * @param Payload 事件数据载荷
	 */
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	///////////////// 委托句柄 /////////////////
	
	/** 蒙太奇混出开始委托，绑定到AnimInstance */
	FOnMontageBlendingOutStarted BlendingOutDelegate;
	
	/** 蒙太奇结束委托，绑定到AnimInstance */
	FOnMontageEnded MontageEndedDelegate;
	
	/** 技能取消委托句柄，用于解绑 */
	FDelegateHandle CancelledHandle;
	
	/** GameplayEvent委托句柄，用于解绑 */
	FDelegateHandle EventHandle;
	
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

/**
 * AbilityTask_PlayMontageForMeshAndWaitForEvent 实现文件
 * 
 * 网络复制架构说明：
 * ┌─────────────────────────────────────────────────────────────────┐
 * │  服务器 (Authority)                                             │
 * │  - 调用PlayMontageForMesh播放蒙太奇                              │
 * │  - 更新RepAnimMontageInfoForMeshes复制属性                       │
 * │  - 调用ForceNetUpdate强制网络同步                                │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  主控端 (AutonomousProxy)                                       │
 * │  - 本地预测播放蒙太奇                                            │
 * │  - 通过PredictionKey注册拒绝回调                                 │
 * │  - 服务器拒绝时调用OnPredictiveMontageRejectedForMesh回滚        │
 * ├─────────────────────────────────────────────────────────────────┤
 * │  模拟端 (SimulatedProxy)                                        │
 * │  - 通过OnRep_ReplicatedAnimMontageForMesh接收复制数据            │
 * │  - 调用PlayMontageSimulatedForMesh播放同步动画                   │
 * │  - 自动进行位置校正防止漂移                                      │
 * └─────────────────────────────────────────────────────────────────┘
 */

#include "Tasks/AbilityTask_PlayMontageForMeshAndWaitForEvent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PlayMontageForMeshAndWaitForEvent)

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "YcAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"

UAbilityTask_PlayMontageForMeshAndWaitForEvent::UAbilityTask_PlayMontageForMeshAndWaitForEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Rate = 1.f;
	bStopWhenAbilityEnds = true;
}

/**
 * 获取目标AbilitySystemComponent
 * @return 转换为UYcAbilitySystemComponent的ASC指针，用于访问扩展的蒙太奇播放功能
 */
UYcAbilitySystemComponent* UAbilityTask_PlayMontageForMeshAndWaitForEvent::GetTargetASC()
{
	return Cast<UYcAbilitySystemComponent>(AbilitySystemComponent);
}

/**
 * 蒙太奇开始混出时的回调处理
 * 
 * 执行逻辑：
 * 1. 检查当前技能是否正在播放此蒙太奇
 * 2. 如果是，清除ASC的AnimatingAbility引用
 * 3. 重置RootMotion缩放（仅在Authority或LocalPredicted的AutonomousProxy上）
 * 4. 根据是否被中断触发相应的委托
 * 
 * @param Montage 开始混出的蒙太奇
 * @param bInterrupted true表示被其他蒙太奇中断，false表示正常混出
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// 验证当前技能是否正在播放此蒙太奇
	if (Ability && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			// 清除ASC中的AnimatingAbility引用
			AbilitySystemComponent->ClearAnimatingAbility(Ability);

			// 重置RootMotion缩放
			// 只在Authority或使用LocalPredicted策略的AutonomousProxy上执行
			// 这确保了RootMotion的正确网络同步
			ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
			if (Character && (Character->GetLocalRole() == ROLE_Authority ||
				(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
			{
				Character->SetAnimRootMotionTranslationScale(1.f);
			}

		}
	}

	// 根据中断状态触发相应委托
	if (bInterrupted)
	{
		// 被其他蒙太奇中断
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
	else
	{
		// 正常混出
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

/**
 * 技能被取消时的回调处理
 * 
 * 当技能被其他技能取消或显式调用CancelAbility时触发
 * 停止蒙太奇播放并通知蓝图层处理取消逻辑
 * 
 * 注意：这里修复了引擎原版的一个bug，原版错误地调用了OnInterrupted而不是OnCancelled
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnAbilityCancelled()
{
	// TODO: Merge this fix back to engine, it was calling the wrong callback

	// 停止蒙太奇播放，使用取消专用的混出时间
	if (StopPlayingMontage(OverrideBlendOutTimeForCancelAbility))
	{
		// 通知蓝图层处理取消逻辑
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

/**
 * 蒙太奇播放结束时的回调处理
 * 
 * 当蒙太奇完全播放完成时触发（包括混出完成后）
 * 如果是正常结束（非中断），触发OnCompleted委托
 * 无论如何都会结束Task
 * 
 * @param Montage 结束的蒙太奇
 * @param bInterrupted true表示被中断结束，false表示正常播放完成
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 只有正常结束时才触发OnCompleted
	// 中断情况已在OnMontageBlendingOut中处理
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	// 结束Task，触发清理流程
	EndTask();
}

/**
 * 收到GameplayEvent时的回调处理
 * 
 * 当ASC收到与EventTags匹配的GameplayEvent时触发
 * 常用于AnimNotify发送的事件，实现动画特定时机的效果触发
 * 例如：攻击动画的命中判定点、施法动画的释放点等
 * 
 * @param EventTag 触发的事件标签
 * @param Payload 事件携带的数据，包含Instigator、Target、Magnitude等信息
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		// 复制Payload数据并设置EventTag
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		// 触发EventReceived委托，传递事件数据给蓝图
		EventReceived.Broadcast(EventTag, TempData);
	}
}

/**
 * 静态工厂函数：创建并配置Task实例
 * 
 * 使用NewAbilityTask模板函数创建Task，这是GAS推荐的Task创建方式
 * 会自动处理Task的生命周期管理和与技能的关联
 * 
 * @param OwningAbility 拥有此Task的技能实例
 * @param TaskInstanceName Task实例名称
 * @param InMesh 目标骨骼网格
 * @param MontageToPlay 要播放的蒙太奇
 * @param EventTags 要监听的事件标签
 * @param Rate 播放速率
 * @param StartSection 起始Section
 * @param bStopWhenAbilityEnds 技能结束时是否停止
 * @param AnimRootMotionTranslationScale RootMotion缩放
 * @param bReplicateMontage 是否网络复制
 * @param OverrideBlendOutTimeForCancelAbility 取消时混出时间
 * @param OverrideBlendOutTimeForStopWhenEndAbility 结束时混出时间
 * @return 配置好的Task实例
 */
UAbilityTask_PlayMontageForMeshAndWaitForEvent* UAbilityTask_PlayMontageForMeshAndWaitForEvent::PlayMontageForMeshAndWaitForEvent(UGameplayAbility* OwningAbility,
	FName TaskInstanceName, USkeletalMeshComponent* InMesh, FName InMeshTag, UAnimMontage* MontageToPlay, FGameplayTagContainer EventTags, float Rate, FName StartSection,
	bool bStopWhenAbilityEnds, float AnimRootMotionTranslationScale, bool bReplicateMontage, float OverrideBlendOutTimeForCancelAbility,
	float OverrideBlendOutTimeForStopWhenEndAbility)
{
	// 应用全局技能速率缩放（如果配置了的话）
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	// 使用模板函数创建Task实例
	UAbilityTask_PlayMontageForMeshAndWaitForEvent* MyObj = NewAbilityTask<UAbilityTask_PlayMontageForMeshAndWaitForEvent>(OwningAbility, TaskInstanceName);
	
	// 配置Task参数
	MyObj->Mesh = InMesh;
	MyObj->MeshTag = InMeshTag;
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->EventTags = EventTags;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	MyObj->bReplicateMontage = bReplicateMontage;
	MyObj->OverrideBlendOutTimeForCancelAbility = OverrideBlendOutTimeForCancelAbility;
	MyObj->OverrideBlendOutTimeForStopWhenEndAbility = OverrideBlendOutTimeForStopWhenEndAbility;

	return MyObj;
}

/**
 * 激活Task，开始播放蒙太奇
 * 
 * 这是Task的核心执行函数，完成以下工作：
 * 1. 验证技能和Mesh的有效性
 * 2. 绑定GameplayEvent监听器
 * 3. 调用ASC的PlayMontageForMesh播放蒙太奇（处理网络复制）
 * 4. 绑定蒙太奇的各种回调（BlendingOut、Ended）
 * 5. 绑定技能取消回调
 * 6. 设置RootMotion缩放
 * 
 * 网络行为：
 * - 服务器：播放蒙太奇并更新复制属性
 * - 主控端：本地预测播放，注册拒绝回调
 * - 模拟端：不会直接调用此函数，通过OnRep接收复制数据
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}
	
	// 如果没有指定Mesh那么就尝试通过MeshTag在AvatarActor上查找Mesh
	Mesh = Mesh ? Mesh : Cast<USkeletalMeshComponent>(GetAvatarActor()->FindComponentByTag(USkeletalMeshComponent::StaticClass(), MeshTag));

	// 如果Mesh依旧无效就直接返回
	if (Mesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%s invalid Mesh"), *FString(__FUNCTION__));
		return;
	}

	bool bPlayedMontage = false;
	UYcAbilitySystemComponent* YcAbilitySystemComponent = GetTargetASC();

	if (YcAbilitySystemComponent)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// 步骤1：绑定GameplayEvent监听器
			// 当收到匹配EventTags的事件时，会调用OnGameplayEvent
			// 这允许AnimNotify通过SendGameplayEvent触发技能逻辑
			EventHandle = YcAbilitySystemComponent->AddGameplayEventTagContainerDelegate(EventTags, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnGameplayEvent));

			// 步骤2：调用ASC播放蒙太奇
			// PlayMontageForMesh内部处理了网络复制逻辑：
			// - 服务器：更新RepAnimMontageInfoForMeshes并ForceNetUpdate
			// - 客户端：注册PredictionKey的拒绝回调
			if (YcAbilitySystemComponent->PlayMontageForMesh(Ability, Mesh, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection, bReplicateMontage) > 0.f)
			{
				// 播放蒙太奇可能触发游戏代码回调，可能导致技能被销毁
				// 检查Task是否仍然有效
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				// 步骤3：绑定技能取消回调
				// 当技能被取消时，会调用OnAbilityCancelled停止蒙太奇
				CancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnAbilityCancelled);

				// 步骤4：绑定蒙太奇回调
				// BlendingOut：蒙太奇开始混出时触发
				BlendingOutDelegate.BindUObject(this, &UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				// Ended：蒙太奇完全结束时触发
				MontageEndedDelegate.BindUObject(this, &UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				// 步骤5：设置RootMotion缩放
				// 只在Authority或LocalPredicted的AutonomousProxy上设置
				// 这确保了RootMotion在网络游戏中的正确行为
				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
					(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UAbilityTask_PlayMontageForMeshAndWaitForEvent call to PlayMontage failed!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UAbilityTask_PlayMontageForMeshAndWaitForEvent called on invalid AbilitySystemComponent"));
	}

	// 如果播放失败，触发OnCancelled委托通知蓝图
	if (!bPlayedMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAbilityTask_PlayMontageForMeshAndWaitForEvent called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay), *InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			//ABILITY_LOG(Display, TEXT("%s: OnCancelled"), *GetName());
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	// 设置Task为等待Avatar状态
	// 这告诉GAS系统此Task正在等待Avatar相关的操作完成
	SetWaitingOnAvatar();
}

/**
 * 外部取消Task
 * 
 * 当技能被外部系统取消时调用（如被其他技能打断）
 * 先调用OnAbilityCancelled处理蒙太奇停止和委托触发
 * 然后调用父类的ExternalCancel完成Task的清理
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::ExternalCancel()
{
	check(AbilitySystemComponent.IsValid());

	OnAbilityCancelled();

	Super::ExternalCancel();
}

/**
 * Task销毁时的清理函数
 * 
 * 负责清理所有绑定的委托和资源：
 * 1. 解绑技能取消委托
 * 2. 如果技能结束且bStopWhenAbilityEnds为true，停止蒙太奇
 * 3. 移除GameplayEvent监听器
 * 
 * 注意：蒙太奇的BlendingOut和Ended委托不需要手动清理，
 * 因为它们不是多播委托，会在下一个蒙太奇播放时自动覆盖
 * 
 * @param AbilityEnded 技能是否已结束
 */
void UAbilityTask_PlayMontageForMeshAndWaitForEvent::OnDestroy(bool AbilityEnded)
{
	// 注意：清除蒙太奇结束委托并非必要操作，因为其并非多播委托，且会在下一次蒙太奇播放时自动清除。
	// （如果程序被销毁，它会检测到这一情况并不会执行任何操作）
	// 但此委托应当被清除，因为它属于多播类型。
	if (Ability)
	{
		// 解绑技能取消委托
		Ability->OnGameplayAbilityCancelled.Remove(CancelledHandle);
		
		// 如果技能结束且配置了结束时停止蒙太奇，则停止它
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage(OverrideBlendOutTimeForStopWhenEndAbility);
		}
	}

	// 移除GameplayEvent监听器
	UYcAbilitySystemComponent* YcAbilitySystemComponent = GetTargetASC();
	if (YcAbilitySystemComponent)
	{
		YcAbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

/**
 * 停止当前播放的蒙太奇
 * 
 * 检查技能是否正在播放蒙太奇，如果是则停止它
 * 在停止前会解绑蒙太奇的回调委托，避免触发不必要的回调
 * 
 * @param OverrideBlendOutTime 混出时间覆盖，-1使用蒙太奇默认值
 * @return 是否成功停止了蒙太奇
 */
bool UAbilityTask_PlayMontageForMeshAndWaitForEvent::StopPlayingMontage(float OverrideBlendOutTime)
{
	if (Mesh == nullptr)
	{
		return false;
	}

	UYcAbilitySystemComponent* YcAbilitySystemComponent = GetTargetASC();
	if (!YcAbilitySystemComponent)
	{
		return false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return false;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return false;
	}

	// 检查蒙太奇是否仍在播放
	// 技能可能已被中断，此时应该自动停止蒙太奇
	if (YcAbilitySystemComponent && Ability)
	{
		if (YcAbilitySystemComponent->GetAnimatingAbilityFromAnyMesh() == Ability
			&& YcAbilitySystemComponent->GetCurrentMontageForMesh(Mesh) == MontageToPlay)
		{
			// 解绑委托，避免停止时触发回调
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			// 调用ASC停止蒙太奇
			// 这会处理网络复制，更新RepAnimMontageInfoForMeshes
			YcAbilitySystemComponent->CurrentMontageStopForMesh(Mesh, OverrideBlendOutTime);
			return true;
		}
	}

	return false;
}

/**
 * 获取调试字符串
 * 
 * 返回当前Task状态的调试信息，包括：
 * - 配置要播放的蒙太奇名称
 * - 当前实际正在播放的蒙太奇名称
 * 
 * @return 格式化的调试字符串
 */
FString UAbilityTask_PlayMontageForMeshAndWaitForEvent::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability && Mesh)
	{
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			// 检查配置的蒙太奇是否正在播放，否则获取当前活动的蒙太奇
			PlayingMontage = AnimInstance->Montage_IsActive(MontageToPlay) ? MontageToPlay : AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWaitForEvent. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}
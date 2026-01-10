// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcGameplayAbility_HitScanWeapon.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Physics/YcCollisionChannels.h"
#include "Weapons/YcWeaponInstance.h"
#include "NativeGameplayTags.h"
#include "YiChenShooterCore.h"
#include "AbilitySystem/Abilities/YcGameplayAbilityTargetData_SingleTargetHit.h"
#include "Weapons/YcHitScanWeaponInstance.h"
#include "Weapons/YcWeaponStateComponent.h"
#include "Weapons/Fragments/YcFragment_WeaponStats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility_HitScanWeapon)

// ==================== 控制台变量定义 ====================
// 用于调试射线检测的可视化参数
namespace YcConsoleVariables
{
	// 子弹射线绘制持续时间（秒），0表示不绘制
	static float DrawBulletTracesDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletTraceDuraton(
		TEXT("Yc.Weapon.DrawBulletTraceDuration"),
		DrawBulletTracesDuration,
		TEXT("是否绘制子弹射线调试线（如果大于0，设置持续时间秒数）"),
		ECVF_Default);

	// 子弹命中点绘制持续时间（秒），0表示不绘制
	static float DrawBulletHitDuration = 0.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHits(
		TEXT("Yc.Weapon.DrawBulletHitDuration"),
		DrawBulletHitDuration,
		TEXT("是否绘制子弹命中点调试（如果大于0，设置持续时间秒数）"),
		ECVF_Default);

	// 子弹命中点调试球体半径
	static float DrawBulletHitRadius = 8.0f;
	static FAutoConsoleVariableRef CVarDrawBulletHitRadius(
		TEXT("Yc.Weapon.DrawBulletHitRadius"),
		DrawBulletHitRadius,
		TEXT("当启用子弹命中调试绘制时，命中点球体的半径（单位：uu）"),
		ECVF_Default);
}


// 武器射击阻止标签 - 如果玩家拥有此标签，武器射击将被阻止/取消
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WeaponFireBlocked, "Ability.Weapon.NoFiring");

/**
 * 基于正态分布的锥形随机方向生成函数
 * 用于计算子弹在扩散范围内的随机偏移方向
 * 
 * @param Dir - 基准方向（瞄准方向）
 * @param ConeHalfAngleRad - 锥形半角（弧度）
 * @param Exponent - 指数，控制分布密度（越大越集中于中心）
 * @return 在锥形范围内的随机方向向量
 * 
 * 算法说明：
 * 1. 将锥形视为两个旋转的组合：一个是"远离中心线"的旋转，另一个是"绕圆周"的旋转
 * 2. 对"远离中心"的旋转应用指数变换，较大的指数会使弹着点更集中于中心
 * 3. 默认指数1.0表示在扩散范围内均匀分布
 */
FVector VRandConeNormalDistribution(const FVector& Dir, const float ConeHalfAngleRad, const float Exponent)
{
	if (ConeHalfAngleRad > 0.f)
	{
		const float ConeHalfAngleDegrees = FMath::RadiansToDegrees(ConeHalfAngleRad);

		// 将锥形视为两个旋转的组合：
		// 1. FromCenter: 远离中心线的角度（应用指数变换）
		// 2. AngleAround: 绕中心线的旋转角度（0-360度均匀分布）
		const float FromCenter = FMath::Pow(FMath::FRand(), Exponent);
		const float AngleFromCenter = FromCenter * ConeHalfAngleDegrees;
		const float AngleAround = FMath::FRand() * 360.0f;

		// 构建旋转四元数
		FRotator Rot = Dir.Rotation();
		FQuat DirQuat(Rot);
		FQuat FromCenterQuat(FRotator(0.0f, AngleFromCenter, 0.0f));
		FQuat AroundQuat(FRotator(0.0f, 0.0, AngleAround));
		
		// 组合旋转：基准方向 * 绕圆周旋转 * 远离中心旋转
		FQuat FinalDirectionQuat = DirQuat * AroundQuat * FromCenterQuat;
		FinalDirectionQuat.Normalize();

		return FinalDirectionQuat.RotateVector(FVector::ForwardVector);
	}
	else
	{
		// 无扩散时直接返回归一化的基准方向
		return Dir.GetSafeNormal();
	}
}


// ==================== 构造函数和基础接口 ====================

UYcGameplayAbility_HitScanWeapon::UYcGameplayAbility_HitScanWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 添加阻止射击的标签到来源阻止标签列表
	// 当技能拥有者拥有此标签时，技能无法激活
	SourceBlockedTags.AddTag(TAG_WeaponFireBlocked);
}

UYcHitScanWeaponInstance* UYcGameplayAbility_HitScanWeapon::GetWeaponInstance() const
{
	// 从关联的装备实例中获取并转换为HitScan武器实例
	return Cast<UYcHitScanWeaponInstance>(GetAssociatedEquipment());
}

bool UYcGameplayAbility_HitScanWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 首先调用基类检查
	bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	
	if (bResult)
	{
		// 验证武器实例类型是否为UYcHitScanWeaponInstance或其子类
		// 如果类型不匹配，记录错误并阻止技能激活
		if (GetWeaponInstance() == nullptr)	
		{
			UE_LOG(LogYcShooterCore, Error, TEXT("武器技能 %s 无法激活，因为没有关联的远程武器（装备实例=%s，但需要派生自 %s）"),
				*GetPathName(),
				*GetPathNameSafe(GetAssociatedEquipment()),
				*UYcHitScanWeaponInstance::StaticClass()->GetName());
			bResult = false;
		}
	}
	

	return bResult;
}


// ==================== 技能生命周期 ====================

void UYcGameplayAbility_HitScanWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                      const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                      const FGameplayEventData* TriggerEventData)
{
	// 绑定TargetData回调委托
	// 当TargetData准备就绪时（本地或从服务器接收），会触发OnTargetDataReadyCallback
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);
	
	OnTargetDataReadyCallbackDelegateHandle = MyAbilityComponent->AbilityTargetDataSetDelegate(
		CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(
		this, &ThisClass::OnTargetDataReadyCallback);
	
	// 更新武器开火时间记录
	// 用于计算武器空闲时间，影响扩散恢复等机制
	UYcHitScanWeaponInstance* WeaponInst = GetWeaponInstance();
	check(WeaponInst);
	WeaponInst->UpdateFiringTime();
	
	// 调用基类激活逻辑
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UYcGameplayAbility_HitScanWeapon::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 验证技能结束是否有效
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		// 如果当前有作用域锁定，延迟执行结束逻辑
		if (ScopeLockCount > 0)
		{
			WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
			return;
		}

		UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
		check(MyAbilityComponent);

		// 技能结束时的清理工作：
		// 1. 移除目标数据回调委托
		// 2. 消耗已复制的客户端目标数据
		MyAbilityComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}


// ==================== 射击瞄准入口 ====================

void UYcGameplayAbility_HitScanWeapon::StartHitScanWeaponTargeting(bool bIsAiming, bool bIsCrouching)
{
	check(CurrentActorInfo);

	const AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	check(AvatarActor);

	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);

	// 获取武器状态组件，用于管理命中标记
	const AController* Controller = GetControllerFromActorInfo();
	check(Controller);
	UYcWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<UYcWeaponStateComponent>();

	// 缓存当前射击状态
	bCurrentShotIsAiming = bIsAiming;
	bCurrentShotIsCrouching = bIsCrouching;

	// 创建网络预测窗口
	// 这允许客户端在等待服务器确认之前预测性地执行操作
	FScopedPredictionWindow ScopedPrediction(MyAbilityComponent, CurrentActivationInfo.GetActivationPredictionKey());

	TArray<FHitResult> FoundHits;
	// 在本地执行射线检测
	PerformLocalTargeting(/*out*/ FoundHits, bIsAiming);

	// 将命中结果转换为TargetData
	FGameplayAbilityTargetDataHandle TargetData;
	// UniqueId用于服务器确认命中标记
	TargetData.UniqueId = WeaponStateComponent ? WeaponStateComponent->GetUnconfirmedServerSideHitMarkerCount() : 0;

	if (FoundHits.Num() > 0)
	{
		// 为这次射击生成唯一的弹药ID
		// 同一次射击的所有弹丸共享相同的CartridgeID
		const int32 CartridgeID = FMath::Rand();

		for (const FHitResult& FoundHit : FoundHits)
		{
			// 为每个命中结果创建TargetData, 这里使用我们自定义扩展的TargetData可以携带CartridgeID的信息, 以便知道这个TargetData属于哪颗子弹(哪次射击)产生的
			FYcGameplayAbilityTargetData_SingleTargetHit* NewTargetData = new FYcGameplayAbilityTargetData_SingleTargetHit();
			NewTargetData->HitResult = FoundHit;
			NewTargetData->CartridgeID = CartridgeID;

			TargetData.Add(NewTargetData);
		}
	}

	// 发送命中标记信息到武器状态组件
	// 用于在UI上显示命中反馈（等待服务器确认）
	const bool bProjectileWeapon = false;
	if (!bProjectileWeapon && (WeaponStateComponent != nullptr))
	{
		WeaponStateComponent->AddUnconfirmedServerSideHitMarkers(TargetData, FoundHits);
	}

	// 立即处理目标数据（触发回调）
	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}


// ==================== 本地射线检测 ====================

void UYcGameplayAbility_HitScanWeapon::PerformLocalTargeting(TArray<FHitResult>& OutHits, bool bIsAiming)
{
	APawn* const AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	
	UYcHitScanWeaponInstance* WeaponData = GetWeaponInstance();
	
	// 仅在本地控制的Pawn上执行射线检测
	if (AvatarPawn && AvatarPawn->IsLocallyControlled() && WeaponData)
	{
		// 构建射线检测输入数据
		FHitScanWeaponFiringInput InputData;
		InputData.WeaponData = WeaponData;
		// 专用服务器上不播放子弹特效
		InputData.bCanPlayBulletFX = (AvatarPawn->GetNetMode() != NM_DedicatedServer);

		// 获取瞄准变换（位置和方向）
		// 使用CameraTowardsFocus模式：从摄像机位置出发，朝向摄像机焦点
		// @TODO: 当玩家靠近墙壁时应该有更复杂的逻辑处理
		const FTransform TargetTransform = GetTargetingTransform(AvatarPawn, EYcAbilityTargetingSource::CameraTowardsFocus);
		InputData.AimDir = TargetTransform.GetUnitAxis(EAxis::X);	// X轴为瞄准方向
		InputData.StartTrace = TargetTransform.GetTranslation();	// 变换位置为射线起点

		// 计算射线终点 = 起点 + 方向 × 最大射程
		InputData.EndAim = InputData.StartTrace + InputData.AimDir * WeaponData->GetMaxDamageRange();

		// 调试绘制：显示瞄准方向
#if ENABLE_DRAW_DEBUG
		if (YcConsoleVariables::DrawBulletTracesDuration > 0.0f)
		{
			// 绘制100单位长度的黄色线段表示瞄准方向
			static float DebugThickness = 2.0f;
			DrawDebugLine(GetWorld(), InputData.StartTrace, InputData.StartTrace + (InputData.AimDir * 100.0f), FColor::Yellow, false, YcConsoleVariables::DrawBulletTracesDuration, 0, DebugThickness);
		}
#endif
		// 执行子弹射线检测
		// 当配置了单发多弹丸时（如霰弹枪），会进行多条射线检测
		TraceBulletsInCartridge(InputData, /*out*/ OutHits);

		// 通知武器实例射击事件，更新扩散和后坐力状态
		WeaponData->OnFired(bIsAiming);

		// 获取并缓存本次射击的后坐力
		LastShotRecoil = WeaponData->GetRecoilForCurrentShot(bCurrentShotIsAiming, bCurrentShotIsCrouching);
	}
}


// ==================== 弹丸射线检测 ====================

void UYcGameplayAbility_HitScanWeapon::TraceBulletsInCartridge(const FHitScanWeaponFiringInput& InputData,
                                                              TArray<FHitResult>& OutHits)
{
	UYcHitScanWeaponInstance* WeaponData = InputData.WeaponData;
	check(WeaponData);
	
	// 获取单发子弹产生的弹丸数量（普通武器为1，霰弹枪可能为8-12）
	const int32 BulletsPerCartridge = WeaponData->GetBulletsPerCartridge();

	// 获取当前扩散参数
	// 瞄准时使用ADS扩散（通常为0），腰射时使用HipFire扩散
	const float BaseSpreadAngle = WeaponData->GetCurrentSpreadAngle(bCurrentShotIsAiming);
	const float SpreadMultiplier = bCurrentShotIsAiming ? 1.0f : WeaponData->GetCurrentSpreadMultiplier();
	const bool bHasFirstShotAccuracy = WeaponData->HasFirstShotAccuracy(bCurrentShotIsAiming);

	// 计算实际扩散角度
	float ActualSpreadAngle = 0.0f;
	if (!bHasFirstShotAccuracy)
	{
		ActualSpreadAngle = BaseSpreadAngle * SpreadMultiplier;
	}
	
	// 为每颗弹丸执行射线检测
	for (int32 BulletIndex = 0; BulletIndex < BulletsPerCartridge; ++BulletIndex)
	{
		// 将扩散角度转换为弧度（半角）
		const float HalfSpreadAngleInRadians = FMath::DegreesToRadians(ActualSpreadAngle * 0.5f);

		// 使用正态分布在扩散锥内生成随机方向
		// SpreadExponent控制分布密度，值越大弹着点越集中于中心
		const FVector BulletDir = VRandConeNormalDistribution(InputData.AimDir, HalfSpreadAngleInRadians, WeaponData->GetSpreadExponent());

		// 计算这颗弹丸的射线终点
		const FVector EndTrace = InputData.StartTrace + (BulletDir * WeaponData->GetMaxDamageRange());
		FVector HitLocation = EndTrace;

		TArray<FHitResult> AllImpacts;

		// 执行单条射线检测
		FHitResult Impact = DoSingleBulletTrace(InputData.StartTrace, EndTrace, WeaponData->GetBulletTraceSweepRadius(), /*bIsSimulated=*/ false, /*out*/ AllImpacts);

		const AActor* HitActor = Impact.GetActor();

		if (HitActor)
		{
			// 调试绘制：显示命中点
#if ENABLE_DRAW_DEBUG
			if (YcConsoleVariables::DrawBulletHitDuration > 0.0f)
			{
				DrawDebugPoint(GetWorld(), Impact.ImpactPoint, YcConsoleVariables::DrawBulletHitRadius, FColor::Red, false, YcConsoleVariables::DrawBulletHitDuration);
			}
#endif

			// 将所有命中结果添加到输出列表
			if (AllImpacts.Num() > 0)
			{
				OutHits.Append(AllImpacts);
			}

			HitLocation = Impact.ImpactPoint;
		}

		// 确保OutHits中至少有一个条目
		// 即使没有命中任何目标，也需要记录射线方向用于弹道特效等
		if (OutHits.Num() == 0)
		{
			if (!Impact.bBlockingHit)
			{
				// 将"假命中"位置设置为射线终点
				Impact.Location = EndTrace;
				Impact.ImpactPoint = EndTrace;
			}

			OutHits.Add(Impact);
		}
	}
}


// ==================== 单条射线检测（两阶段策略） ====================

FHitResult UYcGameplayAbility_HitScanWeapon::DoSingleBulletTrace(const FVector& StartTrace, const FVector& EndTrace,
                                                                float SweepRadius, bool bIsSimulated, TArray<FHitResult>& OutHits) const
{
	// 调试绘制：显示射线轨迹
#if ENABLE_DRAW_DEBUG
	if (YcConsoleVariables::DrawBulletTracesDuration > 0.0f)
	{
		static float DebugThickness = 1.0f;
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Green, false, YcConsoleVariables::DrawBulletTracesDuration, 0, DebugThickness);
	}
#endif // ENABLE_DRAW_DEBUG
	
	FHitResult Impact;
	
	/**
	 * 两阶段射线检测策略：
	 * 1. 首先进行精确的线性射线检测（SweepRadius=0）
	 * 2. 如果未命中Pawn且SweepRadius>0，则进行球形扫描检测
	 * 
	 * 这种策略的优点：
	 * - 优先使用精确检测，保证命中判定的准确性
	 * - 球形扫描作为后备，提供"子弹吸附"效果，改善射击手感
	 */
	
	// 第一阶段：精确线性射线检测
	if (FindFirstPawnHitResult(OutHits) == INDEX_NONE)
	{
		Impact = WeaponTrace(StartTrace, EndTrace, /*SweepRadius=*/ 0.0f, bIsSimulated, /*out*/ OutHits);
	}
	
	// 第二阶段：如果第一阶段未命中Pawn且支持球形扫描，则进行扫描检测
	if (FindFirstPawnHitResult(OutHits) == INDEX_NONE)
	{
		if (SweepRadius > 0.0f)
		{
			TArray<FHitResult> SweepHits;
			Impact = WeaponTrace(StartTrace, EndTrace, SweepRadius, bIsSimulated, /*out*/ SweepHits);

			// 检查球形扫描是否命中了Pawn
			const int32 FirstPawnIdx = FindFirstPawnHitResult(SweepHits);
			if (SweepHits.IsValidIndex(FirstPawnIdx))
			{
				/**
				 * 重要的遮挡判断逻辑：
				 * 
				 * 如果在SweepHits中命中Pawn之前存在阻挡物，且该阻挡物也存在于OutHits中，
				 * 则说明Pawn被遮挡了，应该使用原始的线性检测结果。
				 * 
				 * 示例场景：
				 * 1. Pawn在墙后：线性检测命中墙，球形扫描"穿墙"命中Pawn
				 *    -> 因为墙同时存在于两个结果中，所以使用线性检测结果（Pawn被遮挡）
				 * 
				 * 2. Pawn站在墙边缘：线性检测命中墙，球形扫描命中墙边的Pawn
				 *    -> 如果墙不在Pawn之前，则可以命中Pawn（子弹吸附效果）
				 */
				bool bUseSweepHits = true;
				for (int32 Idx = 0; Idx < FirstPawnIdx; ++Idx)
				{
					const FHitResult& CurHitResult = SweepHits[Idx];

					// 检查当前命中物是否也存在于线性检测结果中
					auto Pred = [&CurHitResult](const FHitResult& Other)
					{
						return Other.HitObjectHandle == CurHitResult.HitObjectHandle;
					};
					
					// 如果是阻挡命中且存在于线性检测结果中，则不使用球形扫描结果
					if (CurHitResult.bBlockingHit && OutHits.ContainsByPredicate(Pred))
					{
						bUseSweepHits = false;
						break;
					}
				}

				if (bUseSweepHits)
				{
					OutHits = SweepHits;
				}
			}
		}
	}

	return Impact;
}


// ==================== 底层射线检测实现 ====================

FHitResult UYcGameplayAbility_HitScanWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace,
                                                        float SweepRadius, bool bIsSimulated, TArray<FHitResult>& OutHitResults) const
{
	TArray<FHitResult> HitResults;
	
	// 配置碰撞查询参数
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), /*bTraceComplex=*/ true, /*IgnoreActor=*/ GetAvatarActorFromActorInfo());
	TraceParams.bReturnPhysicalMaterial = true;  // 返回物理材质信息（用于判断命中部位、播放对应特效等）
	AddAdditionalTraceIgnoreActors(TraceParams);  // 添加额外的忽略Actor
	
#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING)
	TraceParams.bDebugQuery = bDebugQuery;  // 调试模式下启用查询调试
#endif
	
	// 获取射线检测使用的碰撞通道
	const ECollisionChannel TraceChannel = DetermineTraceChannel(TraceParams, bIsSimulated);
	
	// 根据SweepRadius决定检测方式
	if (SweepRadius > 0.0f)
	{
		// 球形扫描检测：使用指定半径的球体进行扫描
		// 可以命中在球体半径范围内的目标（子弹吸附效果）
		GetWorld()->SweepMultiByChannel(HitResults, StartTrace, EndTrace, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(SweepRadius), TraceParams);
	}
	else
	{
		// 线性射线检测：精确的直线检测
		GetWorld()->LineTraceMultiByChannel(HitResults, StartTrace, EndTrace, TraceChannel, TraceParams);
	}
	
	FHitResult Hit(ForceInit);
	if (HitResults.Num() > 0)
	{
		// 过滤输出列表，防止对同一Actor多次命中
		// 这是为了防止单颗子弹对同一目标造成多次伤害
		// （使用重叠检测时可能会对同一Actor产生多个命中结果）
		for (FHitResult& CurHitResult : HitResults)
		{
			// 检查当前命中的Actor是否已经在输出列表中
			auto Pred = [&CurHitResult](const FHitResult& Other)
			{
				return Other.HitObjectHandle == CurHitResult.HitObjectHandle;
			};

			// 只添加未重复的命中结果
			if (!OutHitResults.ContainsByPredicate(Pred))
			{
				OutHitResults.Add(CurHitResult);
			}
		}

		// 返回最后一个命中结果
		Hit = OutHitResults.Last();
	}
	else
	{
		// 没有命中任何目标，记录射线的起点和终点
		Hit.TraceStart = StartTrace;
		Hit.TraceEnd = EndTrace;
	}

	return Hit;
}

void UYcGameplayAbility_HitScanWeapon::AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		// 忽略附加在Avatar上的所有Actor
		// 例如：武器模型、装备、特效等
		// 防止射线检测命中自己的装备
		TArray<AActor*> AttachedActors;
		Avatar->GetAttachedActors(/*out*/ AttachedActors);
		TraceParams.AddIgnoredActors(AttachedActors);
	}
}

ECollisionChannel UYcGameplayAbility_HitScanWeapon::DetermineTraceChannel(FCollisionQueryParams& TraceParams,
	bool bIsSimulated) const
{
	// 返回武器专用碰撞通道
	// 子类可重写此函数以使用不同的碰撞通道
	return Yc_TraceChannel_Weapon;
}


// ==================== 目标数据回调和网络同步 ====================

void UYcGameplayAbility_HitScanWeapon::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData,
                                                                FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(MyAbilityComponent);
	
	// 获取当前技能规格
	const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle);

	if(!AbilitySpec){
		UE_LOG(LogYcShooterCore, Warning, TEXT("获取当前技能规格失败"));
		// 消耗已处理的目标数据
		MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
		return;
	}

	// 创建预测窗口
	FScopedPredictionWindow	ScopedPrediction(MyAbilityComponent);

	// 获取目标数据的所有权
	// 使用MoveTemp确保游戏代码中的回调不会使数据失效
	FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

	// 判断是否需要通知服务器
	// 条件：本地控制 且 没有网络权威（即客户端玩家）
	// Listen Server的玩家拥有网络权威，不需要发送RPC
	const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
	if (bShouldNotifyServer)
	{
		// 将目标数据加入客户端->服务器的RPC队列, 也就是通过ServerRPC把LocalTargetDataHandle数据传给服务器
		// 服务器收到后会触发相同的OnTargetDataReadyCallback函数
		MyAbilityComponent->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), LocalTargetDataHandle, ApplicationTag, MyAbilityComponent->ScopedPredictionKey);
	}

	// @TODO: 实现TargetData验证逻辑
	const bool bIsTargetDataValid = true;

#if WITH_SERVER_CODE
	// 服务器端逻辑：验证命中并发送确认给客户端
	if (const AController* Controller = GetControllerFromActorInfo();
		Controller->GetLocalRole() == ROLE_Authority)
	{
		// 确认命中标记
		if (UYcWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<UYcWeaponStateComponent>())
		{
			TArray<uint8> HitReplaces;
			for (uint8 i = 0; (i < LocalTargetDataHandle.Num()) && (i < 255); ++i)
			{
				if (const FGameplayAbilityTargetData_SingleTargetHit* SingleTargetHit = static_cast<FGameplayAbilityTargetData_SingleTargetHit*>(LocalTargetDataHandle.Get(i)))
				{
					// 如果命中被验证替换，加入确认列表
					// @TODO: 实现真正的服务器端命中验证逻辑
					if (SingleTargetHit->bHitReplaced)
					{
						HitReplaces.Add(i);
					}
				}
			}
			// 服务器发起Client RPC，确认客户端的目标数据
			WeaponStateComponent->ClientConfirmTargetData(LocalTargetDataHandle.UniqueId, bIsTargetDataValid, HitReplaces);
		}
	}
#endif //WITH_SERVER_CODE

	// 检查是否还有弹药，并提交技能
	if (bIsTargetDataValid && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		// 调用蓝图事件，让蓝图处理后续逻辑
		// 例如：应用伤害效果、播放特效、显示伤害数字
		OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
	}
	else
	{
		UE_LOG(LogYcShooterCore, Warning, TEXT("武器技能 %s 提交失败 (bIsTargetDataValid=%d)"), *GetPathName(), bIsTargetDataValid ? 1 : 0);
		K2_EndAbility();
	}

	// 消耗已处理的目标数据
	MyAbilityComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}


// ==================== 后坐力应用 ====================

void UYcGameplayAbility_HitScanWeapon::ApplyRecoilToController(float RecoilPitch, float RecoilYaw)
{
	// 仅在本地控制端应用后坐力
	if (!CurrentActorInfo || !CurrentActorInfo->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetControllerFromActorInfo());
	if (!PC)
	{
		return;
	}

	// 使用 AddPitchInput/AddYawInput 应用后坐力
	// 这些函数直接累加到 RotationInput，不经过灵敏度处理（后坐力不应受灵敏度影响）
	// 注意：AddPitchInput 正值向下看，我们的后坐力Pitch正值表示向上抬，所以取负
	PC->AddPitchInput(-RecoilPitch);
	PC->AddYawInput(RecoilYaw);

	// 触发蓝图事件
	OnRecoilApplied(RecoilPitch, RecoilYaw);
}

void UYcGameplayAbility_HitScanWeapon::ApplyLastShotRecoil()
{
	ApplyRecoilToController(LastShotRecoil.X, LastShotRecoil.Y);
}


// ==================== 辅助函数 ====================

int32 UYcGameplayAbility_HitScanWeapon::FindFirstPawnHitResult(const TArray<FHitResult>& HitResults)
{
	// 遍历命中结果列表，查找第一个Pawn命中
	for (int32 Idx = 0; Idx < HitResults.Num(); ++Idx)
	{
		const FHitResult& CurHitResult = HitResults[Idx];
		
		// 检查是否直接命中Pawn
		if (CurHitResult.HitObjectHandle.DoesRepresentClass(APawn::StaticClass()))
		{
			return Idx;
		}
		else
		{
			// 检查是否命中了附加在Pawn上的Actor（如武器、装备等）
			AActor* HitActor = CurHitResult.HitObjectHandle.FetchActor();
			if ((HitActor != nullptr) && (HitActor->GetAttachParentActor() != nullptr) && (Cast<APawn>(HitActor->GetAttachParentActor()) != nullptr))
			{
				return Idx;
			}
		}
	}

	return INDEX_NONE;
}

FVector UYcGameplayAbility_HitScanWeapon::GetWeaponTargetingSourceLocation() const
{
	// 使用Pawn的位置作为基准
	APawn* const AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	check(AvatarPawn);

	const FVector SourceLoc = AvatarPawn->GetActorLocation();
	const FQuat SourceRot = AvatarPawn->GetActorQuat();

	FVector TargetingSourceLocation = SourceLoc;

	// @TODO: 添加武器实例的偏移量，并根据Pawn的蹲下/瞄准状态进行调整

	return TargetingSourceLocation;
}


FTransform UYcGameplayAbility_HitScanWeapon::GetTargetingTransform(const APawn* SourcePawn,
                                                                  EYcAbilityTargetingSource Source) const
{
	check(SourcePawn);

	// Custom模式应该由调用者自行处理，不应该调用此函数
	check(Source != EYcAbilityTargetingSource::Custom);

	const FVector ActorLoc = SourcePawn->GetActorLocation();
	const FQuat AimQuat = SourcePawn->GetActorQuat();
	AController* Controller = SourcePawn->Controller;
	FVector SourceLoc;

	// 瞄准焦点距离（用于计算焦点位置）
	constexpr double FocalDistance = 1024.0f;
	FVector FocalLoc;

	FVector CamLoc;
	FRotator CamRot;
	bool bFoundFocus = false;

	// 处理需要焦点的瞄准模式
	if ((Controller != nullptr) && ((Source == EYcAbilityTargetingSource::CameraTowardsFocus) || (Source == EYcAbilityTargetingSource::PawnTowardsFocus) || (Source == EYcAbilityTargetingSource::WeaponTowardsFocus)))
	{
		bFoundFocus = true;

		// 获取摄像机位置和旋转
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC != nullptr)
		{
			// 玩家控制器：使用玩家视角
			PC->GetPlayerViewPoint(/*out*/ CamLoc, /*out*/ CamRot);
		}
		else
		{
			// AI控制器：使用武器位置和控制器旋转
			SourceLoc = GetWeaponTargetingSourceLocation();
			CamLoc = SourceLoc;
			CamRot = Controller->GetControlRotation();
		}

		// 计算初始焦点位置
		FVector AimDir = CamRot.Vector().GetSafeNormal();	// 获取旋转的方向向量
		FocalLoc = CamLoc + (AimDir * FocalDistance);		// 焦点 = 起点 + 方向 × 距离

		// 将起点和焦点移到Pawn前方
		// 这样可以避免射线从摄像机位置穿过Pawn身体
		if (PC)
		{
			const FVector WeaponLoc = GetWeaponTargetingSourceLocation();
			// 将摄像机位置投影到武器位置所在的平面上
			CamLoc = FocalLoc + (((WeaponLoc - FocalLoc) | AimDir) * AimDir);
			FocalLoc = CamLoc + (AimDir * FocalDistance);
		}
		// AI控制器：将起点移到头部位置
		else if (AAIController* AIController = Cast<AAIController>(Controller))
		{
			CamLoc = SourcePawn->GetActorLocation() + FVector(0, 0, SourcePawn->BaseEyeHeight);
		}

		// CameraTowardsFocus模式：直接返回摄像机变换
		if (Source == EYcAbilityTargetingSource::CameraTowardsFocus)
		{
			return FTransform(CamRot, CamLoc);
		}
	}

	// 确定源位置
	if ((Source == EYcAbilityTargetingSource::WeaponForward) || (Source == EYcAbilityTargetingSource::WeaponTowardsFocus))
	{
		// 武器模式：使用武器位置
		SourceLoc = GetWeaponTargetingSourceLocation();
	}
	else
	{
		// Pawn模式或未找到摄像机：使用Pawn位置
		SourceLoc = ActorLoc;
	}

	// 处理朝向焦点的模式
	if (bFoundFocus && ((Source == EYcAbilityTargetingSource::PawnTowardsFocus) || (Source == EYcAbilityTargetingSource::WeaponTowardsFocus)))
	{
		// 返回从源位置指向焦点的变换
		return FTransform((FocalLoc - SourceLoc).Rotation(), SourceLoc);
	}

	// 默认：使用Pawn的朝向
	return FTransform(AimQuat, SourceLoc);
}


// ==================== 射击控制 ====================

void UYcGameplayAbility_HitScanWeapon::StartFiring(bool bIsAiming, bool bIsCrouching)
{
	// 如果已经在射击中，忽略
	if (bIsFiring)
	{
		return;
	}

	UYcHitScanWeaponInstance* WeaponData = GetWeaponInstance();
	if (!WeaponData)
	{
		return;
	}

	// 缓存射击状态
	bCurrentShotIsAiming = bIsAiming;
	bCurrentShotIsCrouching = bIsCrouching;
	bIsFiring = true;
	bWantsToStopFiring = false;
	CurrentBurstShotCount = 0;

	// 触发射击开始事件
	OnFiringStarted();

	// 检查是否可以立即射击（射速限制）
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();
	const float FireInterval = GetFireInterval();
	const double TimeSinceLastShot = CurrentTime - LastShotTime;

	if (TimeSinceLastShot >= FireInterval)
	{
		// 可以立即射击
		FireShot();
	}
	else
	{
		// 需要等待射速冷却，设置Timer
		const float RemainingTime = FireInterval - static_cast<float>(TimeSinceLastShot);
		World->GetTimerManager().SetTimer(
			FireTimerHandle,
			this,
			&UYcGameplayAbility_HitScanWeapon::OnFireTimerTick,
			RemainingTime,
			false
		);
	}
}

void UYcGameplayAbility_HitScanWeapon::StopFiring(bool bEndAbilityWhenDone)
{
	if (!bIsFiring)
	{
		// 如果没在射击但请求结束技能，直接结束
		if (bEndAbilityWhenDone)
		{
			K2_EndAbility();
		}
		return;
	}

	UYcHitScanWeaponInstance* WeaponData = GetWeaponInstance();
	if (!WeaponData)
	{
		bIsFiring = false;
		if (bEndAbilityWhenDone)
		{
			K2_EndAbility();
		}
		return;
	}

	const EYcFireMode FireMode = WeaponData->GetFireMode();

	// 记录是否需要在射击结束后结束技能
	bPendingEndAbility = bEndAbilityWhenDone;

	switch (FireMode)
	{
	case EYcFireMode::Auto:
		// 全自动：立即停止
		bIsFiring = false;
		if (const UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FireTimerHandle);
		}
		OnFiringStopped();
		if (bPendingEndAbility)
		{
			bPendingEndAbility = false;
			K2_EndAbility();
		}
		break;

	case EYcFireMode::SemiAuto:
		// 半自动：立即停止
		bIsFiring = false;
		if (const UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FireTimerHandle);
		}
		OnFiringStopped();
		if (bPendingEndAbility)
		{
			bPendingEndAbility = false;
			K2_EndAbility();
		}
		break;

	case EYcFireMode::BurstAuto:
	case EYcFireMode::BurstSingle:
		// 连射模式：标记想要停止，等当前连射完成后停止
		// 技能会在连射完成后自动结束（如果bPendingEndAbility为true）
		bWantsToStopFiring = true;
		break;
	}
}

void UYcGameplayAbility_HitScanWeapon::FireShot()
{
	UYcHitScanWeaponInstance* WeaponData = GetWeaponInstance();
	if (!WeaponData)
	{
		bIsFiring = false;
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		bIsFiring = false;
		return;
	}

	// 记录射击时间
	LastShotTime = World->GetTimeSeconds();

	// 触发单次射击事件
	OnShotFired(CurrentBurstShotCount);

	// 执行实际射击
	StartHitScanWeaponTargeting(bCurrentShotIsAiming, bCurrentShotIsCrouching);

	// 更新连射计数
	CurrentBurstShotCount++;

	// 根据射击模式决定后续行为
	const EYcFireMode FireMode = WeaponData->GetFireMode();
	const float FireInterval = GetFireInterval();
	const float BurstInterval = FireInterval * WeaponData->GetBurstIntervalMultiplier();

	switch (FireMode)
	{
	case EYcFireMode::Auto:
		// 全自动：继续射击
		if (bIsFiring && !bWantsToStopFiring)
		{
			World->GetTimerManager().SetTimer(
				FireTimerHandle,
				this,
				&UYcGameplayAbility_HitScanWeapon::OnFireTimerTick,
				FireInterval,
				false
			);
		}
		break;

	case EYcFireMode::SemiAuto:
		// 半自动：射击一发后停止
		bIsFiring = false;
		CurrentBurstShotCount = 0;
		OnFiringStopped();
		break;

	case EYcFireMode::BurstAuto:
		// 连射(按住自动)：检查是否完成当前连射
		if (CurrentBurstShotCount >= WeaponData->GetBurstCount())
		{
			// 当前连射完成，重置计数
			CurrentBurstShotCount = 0;
			
			if (bWantsToStopFiring)
			{
				// 玩家已松开射击键，停止射击
				bIsFiring = false;
				bWantsToStopFiring = false;
				World->GetTimerManager().ClearTimer(FireTimerHandle);
				OnFiringStopped();
				
				// 如果需要结束技能
				if (bPendingEndAbility)
				{
					bPendingEndAbility = false;
					K2_EndAbility();
				}
			}
			else
			{
				// 玩家仍在按住射击键，等待连射间隔后开始下一轮连射
				World->GetTimerManager().SetTimer(
					FireTimerHandle,
					this,
					&UYcGameplayAbility_HitScanWeapon::OnFireTimerTick,
					BurstInterval,
					false
				);
			}
		}
		else
		{
			// 继续当前连射
			World->GetTimerManager().SetTimer(
				FireTimerHandle,
				this,
				&UYcGameplayAbility_HitScanWeapon::OnFireTimerTick,
				FireInterval,
				false
			);
		}
		break;

	case EYcFireMode::BurstSingle:
		// 连射(单次)：检查是否完成当前连射
		if (CurrentBurstShotCount >= WeaponData->GetBurstCount())
		{
			// 当前连射完成，停止射击（需要松开再按才能继续）
			bIsFiring = false;
			bWantsToStopFiring = false;
			World->GetTimerManager().ClearTimer(FireTimerHandle);
			CurrentBurstShotCount = 0;
			OnFiringStopped();
			
			// 如果需要结束技能
			if (bPendingEndAbility)
			{
				bPendingEndAbility = false;
				K2_EndAbility();
			}
		}
		else
		{
			// 继续当前连射
			World->GetTimerManager().SetTimer(
				FireTimerHandle,
				this,
				&UYcGameplayAbility_HitScanWeapon::OnFireTimerTick,
				FireInterval,
				false
			);
		}
		break;
	}
}

void UYcGameplayAbility_HitScanWeapon::OnFireTimerTick()
{
	if (bIsFiring)
	{
		FireShot();
	}
}

float UYcGameplayAbility_HitScanWeapon::GetFireInterval() const
{
	const UYcHitScanWeaponInstance* WeaponData = GetWeaponInstance();
	if (!WeaponData)
	{
		return 0.1f; // 默认值
	}

	// 射速（发/分钟）转换为射击间隔（秒）
	const float FireRate = WeaponData->GetFireRate();
	if (FireRate <= 0.0f)
	{
		return 0.1f;
	}

	return 60.0f / FireRate;
}

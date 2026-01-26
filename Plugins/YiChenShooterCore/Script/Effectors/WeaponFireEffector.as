/** 用于生成武器开火的特效器 */
class AWeaponFireEffector : AActor
{
	UPROPERTY(DefaultComponent, RootComponent)
	USceneComponent DefaultSceneRoot;

	/**
	 * 是否为第一人称模式,如果是会设置特效的FirstPersonMode=1.0,会传递给材质这样特效位置才能正确匹配第一人称独立FOV
	 */
	UPROPERTY(ExposeOnSpawn)
	bool bFirstPersonMode;

	// 抛壳相关变量
	UPROPERTY(ExposeOnSpawn)
	bool bEnableShellEject = true;

	/** 抛壳插槽 */
	UPROPERTY()
	FName ShellEjectSocketName = n"ShellEject";

	bool bShellEjectTrigger;
	UNiagaraComponent NS_ShellEject;

	// 枪口火焰相关变量

	UPROPERTY(ExposeOnSpawn)
	bool bEnableMuzzleFlash = true;

	/** 枪口火焰插槽 */
	UPROPERTY()
	FName MuzzleFlashSocketName = n"SOCKET_Muzzle";

	bool bMuzzleFlashTrigger;
	UNiagaraComponent NS_MuzzleFlash;

	// 弹道特效相关变量

	UPROPERTY(ExposeOnSpawn)
	bool bEnableTracer = true;

	bool bTracerTrigger;
	UNiagaraComponent NS_Tracer;

	// 武器的信息,在创建本对象时从外部传入
	UPROPERTY(ExposeOnSpawn)
	FTransform MuzzleTransform;

	UPROPERTY(ExposeOnSpawn)
	FTransform ShellEjectTransform;

	TArray<FVector> ImpactPositions;
	TArray<FVector> ImpactNormals;
	TArray<EPhysicalSurface> ImpactSurfaceTypes;

	// 关联的武器实例, 获取相关数据资产做特效表现
	UPROPERTY(ExposeOnSpawn, VisibleInstanceOnly)
	UYcEquipmentInstance AssociatedWeaponInst;

	// 系统内部变量
	FTimerHandle AutoDestroyTimer;
	FRotator AttachRotation = FRotator(0, 90, 0);
	const UYcWeaponVisualData CachedWeaponVisualData;

	/** 生成武器开火相关特效：枪口火焰、抛壳、弹道追踪 */
	UFUNCTION()
	void SpawnFireEffects()
	{
		// 提前缓存武器资产
		CachedWeaponVisualData = YcWeapon::GetWeaponVisualData(AssociatedWeaponInst);

		ShellEject();
		MuzzleFlash();
		Tracer();

		// 若所有系统均处于非活动状态，则每隔 N 秒检查一次以销毁这个Acotr。
		AutoDestroyTimer = System::SetTimer(this, n"CheckToDestroy", 3, true);
	}

	// 抛壳
	private void ShellEject()
	{
		if (!bEnableShellEject)
			return;

		// 检查是否已存在有效的ShellEject组件,如不存在则生成
		if (!IsValid(NS_ShellEject) || !NS_ShellEject.IsActive())
		{
			// 从 WeaponVisualData 获取粒子特效
			UNiagaraSystem ShellEjectSystem = GetShellEjectSystem();
			if (ShellEjectSystem == nullptr)
			{
				Print("WeaponFireEffector: 未找到抛壳特效，请在 WeaponVisualData 中配置 Asset.Weapon.VFX.Shell", 3.0f, FLinearColor::Yellow);
				return;
			}
			
			FVector ShellEjectLocation = ShellEjectTransform.GetLocation();
			NS_ShellEject = Niagara::SpawnSystemAttached(ShellEjectSystem, DefaultSceneRoot, n"None", ShellEjectLocation, AttachRotation, EAttachLocation::KeepRelativeOffset, true);

			// 设置ShellEjectMesh,有有效的模型才设置,否则不设置这个变量会使用粒子系统内部默认设置的弹壳模型
			auto ShellEjectMesh = GetBulletShellMesh();
			if (ShellEjectMesh != nullptr)
			{
				NS_ShellEject.SetVariableStaticMesh(n"User.ShellEjectStaticMesh", ShellEjectMesh);
			}

			bShellEjectTrigger = false;
		}

		// Update trigger
		bShellEjectTrigger = !bShellEjectTrigger;
		// Send trigger update to systems
		NS_ShellEject.SetNiagaraVariableBool("User.Trigger", bShellEjectTrigger);
	}

	// 枪口火焰
	private void MuzzleFlash()
	{
		if (!bEnableMuzzleFlash)
			return;

		// 检查是否已存在有效的MuzzleFlash组件,如不存在则生成
		if (!IsValid(NS_MuzzleFlash) || !NS_MuzzleFlash.IsActive())
		{
			// 从 WeaponVisualData 获取粒子特效
			UNiagaraSystem MuzzleFlashSystem = GetMuzzleFlashSystem();
			if (MuzzleFlashSystem == nullptr)
			{
				Print("WeaponFireEffector: 未找到枪口火焰特效，请在 WeaponVisualData 中配置 Asset.Weapon.VFX.MuzzleFlash", 3.0f, FLinearColor::Yellow);
				return;
			}
			
			NS_MuzzleFlash = Niagara::SpawnSystemAttached(MuzzleFlashSystem, DefaultSceneRoot, n"None", MuzzleTransform.Location, AttachRotation, EAttachLocation::KeepRelativeOffset, true);
			bMuzzleFlashTrigger = false;
		}
		// set first person mode
		NS_MuzzleFlash.SetNiagaraVariableFloat("User.FirstPersonMode", bFirstPersonMode ? 1 : 0);
		// Update trigger
		bMuzzleFlashTrigger = !bMuzzleFlashTrigger;
		// Send trigger update to systems
		NS_MuzzleFlash.SetNiagaraVariableBool("User.Trigger", bMuzzleFlashTrigger);
		// 设置特效朝向
		auto MuzzleFlashDirection = MuzzleTransform.GetLocation() - ImpactPositions[0];
		MuzzleFlashDirection.Normalize();
		NS_MuzzleFlash.SetNiagaraVariableVec3("User.Direction", MuzzleFlashDirection);
	}

	// 弹道特效
	private void Tracer()
	{
		if (!bEnableTracer)
			return;

		// 检查是否已存在有效的Tracer组件,如不存在则生成
		if (!IsValid(NS_Tracer) || !NS_Tracer.IsActive())
		{
			// 从 WeaponVisualData 获取粒子特效
			UNiagaraSystem TracerSystem = GetTracerSystem();
			if (TracerSystem == nullptr)
			{
				Print("WeaponFireEffector: 未找到弹道特效，请在 WeaponVisualData 中配置 Asset.Weapon.VFX.Tracer", 3.0f, FLinearColor::Yellow);
				return;
			}
			
			FVector Loc = MuzzleTransform.Location + FVector(1, -35, 0); // 起点偏移,根据实际效果适当调整
			NS_Tracer = Niagara::SpawnSystemAttached(TracerSystem, DefaultSceneRoot, n"None", Loc, AttachRotation, EAttachLocation::KeepRelativeOffset, true);
			bTracerTrigger = false;
		}
		// Update trigger
		bTracerTrigger = !bTracerTrigger;
		// Send trigger update to systems
		NS_Tracer.SetNiagaraVariableBool("User.Trigger", bTracerTrigger);
		// send impact position array to niagara system
		NiagaraDataInterfaceArray::SetNiagaraArrayVector(NS_Tracer, n"User.ImpactPositions", ImpactPositions);
	}

	// Destroy actor If all systems are inactive only check every N seconds.
	UFUNCTION()
	private void CheckToDestroy()
	{
		if (!IsNiagaraSystemActive(NS_ShellEject) &&
			!IsNiagaraSystemActive(NS_MuzzleFlash) &&
			!IsNiagaraSystemActive(NS_Tracer))
		{
			System::ClearAndInvalidateTimerHandle(AutoDestroyTimer);
			DestroyActor();
		}
	}

	private bool IsNiagaraSystemActive(UNiagaraComponent System)
	{
		return System != nullptr && System.IsActive();
	}

	/** 由创建者调用,传递最新的射击/命中信息,以便根据这些数据正确生成特效*/
	UFUNCTION()
	void UpdateFireInfo(TArray<FVector> InImpactPositions, TArray<FVector> InImpactNormals, TArray<EPhysicalSurface> InImpactSurfaceTypes)
	{
		ImpactPositions = InImpactPositions;
		ImpactNormals = InImpactNormals;
		ImpactSurfaceTypes = InImpactSurfaceTypes;
	}

	// 获取弹壳静态网格体
	private UStaticMesh GetBulletShellMesh()
	{
		if (CachedWeaponVisualData == nullptr)
			return nullptr;

		// 从通用扩展资产中获取
		FYcEquipmentVisualAssetEntry AssetEntry;
		if (!CachedWeaponVisualData.GetAsset(GameplayTags::Asset_Weapon_Mesh_Shell, AssetEntry))
			return nullptr;

		return Cast<UStaticMesh>(AssetEntry.Asset.Get());
	}

	// 获取抛壳粒子特效系统
	private UNiagaraSystem GetShellEjectSystem()
	{
		return CachedWeaponVisualData != nullptr ?
				   CachedWeaponVisualData.GetVFXEffect(GameplayTags::Asset_Weapon_VFX_Shell).Get() :
				   nullptr;
	}

	// 获取枪口火焰粒子特效系统
	private UNiagaraSystem GetMuzzleFlashSystem()
	{
		return CachedWeaponVisualData != nullptr ?
				   CachedWeaponVisualData.GetVFXEffect(GameplayTags::Asset_Weapon_VFX_MuzzleFlash).Get() :
				   nullptr;
	}

	// 获取弹道追踪粒子特效系统
	private UNiagaraSystem GetTracerSystem()
	{
		return CachedWeaponVisualData != nullptr ?
				   CachedWeaponVisualData.GetVFXEffect(GameplayTags::Asset_Weapon_VFX_Tracer).Get() :
				   nullptr;
	}
}
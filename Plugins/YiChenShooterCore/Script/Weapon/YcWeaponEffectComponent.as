// 武器特效管理组件 - 负责管理所有武器相关的视觉特效
class UYcWeaponEffectComponent : UActorComponent
{
	// ========================================
	// 配置参数
	// ========================================
	
	UPROPERTY(Category = "Effect Settings")
	bool bFirstPersonMode = false;
	
	UPROPERTY(Category = "Effect Settings")
	bool bEnableShellEject = true;
	
	UPROPERTY(Category = "Effect Settings")
	bool bEnableMuzzleFlash = true;
	
	UPROPERTY(Category = "Effect Settings")
	bool bEnableTracer = true;
	
	UPROPERTY(Category = "Effect Settings")
	FName MuzzleFlashSocketName = n"SOCKET_Muzzle";
	
	UPROPERTY(Category = "Effect Settings")
	FName ShellEjectSocketName = n"SOCKET_Eject";
	
	/** 是否为霰弹枪伪造弹丸数据 */
	UPROPERTY(Category = "Effect Settings")
	bool bNeedsFakeProjectileData = false;
	
	// ========================================
	// Effector 类配置
	// ========================================
	
	UPROPERTY(Category = "Effector Classes")
	TSubclassOf<AWeaponFireEffector> WeaponFireEffectorClass;
	
	UPROPERTY(Category = "Effector Classes")
	TSubclassOf<AWeaponImpactsEffector> WeaponImpactsEffectorClass;
	
	UPROPERTY(Category = "Effector Classes")
	TSubclassOf<AWeaponDecalsEffector> WeaponDecalsEffectorClass;
	
	// ========================================
	// 内部状态
	// ========================================
	
	private AWeaponFireEffector WeaponFireEffector;
	private AWeaponImpactsEffector WeaponImpactsEffector;
	private AWeaponDecalsEffector WeaponDecalsEffector;
	
	private AYcWeaponActor CachedWeaponActor;
	
	// ========================================
	// 生命周期
	// ========================================
	
	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		CachedWeaponActor = Cast<AYcWeaponActor>(Owner);
		if (!IsValid(CachedWeaponActor))
		{
			Print("YcWeaponEffectComponent: Owner is not a YcWeaponActor!", 5.0f, FLinearColor::Red);
		}
	}
	
	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason Reason)
	{
		CleanupAllEffectors();
	}
	
	// ========================================
	// 公共接口
	// ========================================
	
	/** 主入口：生成所有射击特效 */
	UFUNCTION()
	void SpawnFireEffects(
		TArray<FVector> ImpactPositions, 
		TArray<FVector> ImpactNormals, 
		TArray<EPhysicalSurface> ImpactSurfaceTypes)
	{
		if (!IsValid(CachedWeaponActor))
		{
			return;
		}
		
		// 根据第一人称/第三人称模式决定是否生成枪口特效
		if (ShouldSpawnFireEffector())
		{
			SpawnWeaponFireEffector(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
		}
		
		// 弹孔和撞击特效总是生成
		SpawnWeaponImpactsEffector(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
		SpawnWeaponDecalsEffector(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
	}
	
	/** 清理所有特效 Actor */
	UFUNCTION()
	void CleanupAllEffectors()
	{
		if (IsValid(WeaponFireEffector))
		{
			WeaponFireEffector.DestroyActor();
			WeaponFireEffector = nullptr;
		}
		
		if (IsValid(WeaponImpactsEffector))
		{
			WeaponImpactsEffector.DestroyActor();
			WeaponImpactsEffector = nullptr;
		}
		
		if (IsValid(WeaponDecalsEffector))
		{
			WeaponDecalsEffector.DestroyActor();
			WeaponDecalsEffector = nullptr;
		}
	}
	
	/** 停止所有持续特效（如循环粒子） */
	UFUNCTION()
	void StopAllEffects()
	{
		// 可以在这里添加停止逻辑，例如停止循环粒子
		// 目前 Effector 是一次性的，所以暂时不需要
	}
	
	// ========================================
	// 内部实现
	// ========================================
	
	private bool ShouldSpawnFireEffector()
	{
		auto PC = UYcGameVerbMessageHelpers::GetPlayerControllerFromObject(CachedWeaponActor);
		bool bIsLocalController = IsValid(PC) && PC.IsLocalController();
		
		// 第一人称模式：只在本地生成
		if (bFirstPersonMode)
		{
			return bIsLocalController;
		}
		// 第三人称模式：跳过本地生成（避免重复）
		else
		{
			return !bIsLocalController;
		}
	}
	
	private void SpawnWeaponFireEffector(
		TArray<FVector> ImpactPositions, 
		TArray<FVector> ImpactNormals, 
		TArray<EPhysicalSurface> ImpactSurfaceTypes)
	{
		if (!IsValid(WeaponFireEffector))
		{
			WeaponFireEffector = SpawnActor(WeaponFireEffectorClass);
			if (!IsValid(WeaponFireEffector))
			{
				Print("Failed to spawn WeaponFireEffector!", 3.0f, FLinearColor::Red);
				return;
			}
			
			WeaponFireEffector.AttachToComponent(CachedWeaponActor.WeaponMesh);
		}
		
		// 配置 Effector 参数
		WeaponFireEffector.bFirstPersonMode = bFirstPersonMode;
		WeaponFireEffector.bEnableShellEject = bEnableShellEject;
		WeaponFireEffector.bEnableMuzzleFlash = bEnableMuzzleFlash;
		WeaponFireEffector.bEnableTracer = bEnableTracer;
		WeaponFireEffector.AssociatedWeaponInst = CachedWeaponActor.GetWeaponInstance();
		WeaponFireEffector.MuzzleTransform = GetMuzzleTransform();
		WeaponFireEffector.ShellEjectTransform = GetEjectTransform();
		
		if (bNeedsFakeProjectileData)
		{
			// @TODO: 为霰弹枪伪造弹丸数据
		}
		
		// 更新并生成特效
		WeaponFireEffector.UpdateFireInfo(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
		WeaponFireEffector.SpawnFireEffects();
	}
	
	private void SpawnWeaponImpactsEffector(
		TArray<FVector> ImpactPositions, 
		TArray<FVector> ImpactNormals, 
		TArray<EPhysicalSurface> ImpactSurfaceTypes)
	{
		if (!IsValid(WeaponImpactsEffector))
		{
			WeaponImpactsEffector = SpawnActor(WeaponImpactsEffectorClass);
			if (!IsValid(WeaponImpactsEffector))
			{
				Print("Failed to spawn WeaponImpactsEffector!", 3.0f, FLinearColor::Red);
				return;
			}
			
			WeaponImpactsEffector.AttachToComponent(CachedWeaponActor.WeaponMesh);
			
			// 设置关联的武器实例，用于获取 VisualData
			WeaponImpactsEffector.AssociatedWeaponInst = CachedWeaponActor.GetWeaponInstance();
		}
		
		WeaponImpactsEffector.MuzzlePosition = GetMuzzleTransform().Location;
		WeaponImpactsEffector.UpdateFireInfo(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
		WeaponImpactsEffector.SpawnFireImpactsEffects();
	}
	
	private void SpawnWeaponDecalsEffector(
		TArray<FVector> ImpactPositions, 
		TArray<FVector> ImpactNormals, 
		TArray<EPhysicalSurface> ImpactSurfaceTypes)
	{
		if (!IsValid(WeaponDecalsEffector))
		{
			WeaponDecalsEffector = SpawnActor(WeaponDecalsEffectorClass);
			if (!IsValid(WeaponDecalsEffector))
			{
				Print("Failed to spawn WeaponDecalsEffector!", 3.0f, FLinearColor::Red);
				return;
			}
			
			WeaponDecalsEffector.AttachToComponent(CachedWeaponActor.WeaponMesh);
			
			// 设置关联的武器实例，用于获取 VisualData
			WeaponDecalsEffector.AssociatedWeaponInst = CachedWeaponActor.GetWeaponInstance();
		}
		
		WeaponDecalsEffector.UpdateFireInfo(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
		WeaponDecalsEffector.SpawnDecals();
	}
	
	// ========================================
	// 辅助函数：获取变换
	// ========================================
	
	private FTransform GetMuzzleTransform()
	{
		// 优先级：消音器 > 枪管 > 武器主网格体
		auto MuzzleComp = CachedWeaponActor.GetAttachmentMeshForSlot(GameplayTags::Attachment_Slot_Muzzle);
		if (IsValid(MuzzleComp))
		{
			return MuzzleComp.GetSocketTransform(MuzzleFlashSocketName, ERelativeTransformSpace::RTS_Actor);
		}
		
		auto BarrelComp = CachedWeaponActor.GetAttachmentMeshForSlot(GameplayTags::Attachment_Slot_Barrel);
		if (IsValid(BarrelComp))
		{
			return BarrelComp.GetSocketTransform(MuzzleFlashSocketName, ERelativeTransformSpace::RTS_Actor);
		}
		
		return CachedWeaponActor.WeaponMesh.GetSocketTransform(MuzzleFlashSocketName, ERelativeTransformSpace::RTS_Actor);
	}
	
	private FTransform GetEjectTransform()
	{
		return CachedWeaponActor.WeaponMesh.GetSocketTransform(ShellEjectSocketName, ERelativeTransformSpace::RTS_Actor);
	}
}

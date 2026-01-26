/** 用于生成武器命中贴花的特效器 */
class AWeaponDecalsEffector : AActor
{
	UNiagaraComponent Decal;
	bool bImpactTrigger;
	TArray<FVector> ImpactPositions;
	TArray<FVector> ImpactNormals;
	TArray<EPhysicalSurface> ImpactSurfaceTypes;
	TArray<int32> SurfaceTypes;
	FTimerHandle AutoDestroyTimer;

	// 关联的武器实例，用于获取 VisualData
	UPROPERTY(ExposeOnSpawn, VisibleInstanceOnly)
	UYcEquipmentInstance AssociatedWeaponInst;

	const UYcWeaponVisualData CachedWeaponVisualData;

	default ImpactSurfaceTypes.Add(EPhysicalSurface::SurfaceType_Default);

	/** 生成贴花特效 */
	UFUNCTION()
	void SpawnDecals()
	{
		// 缓存 WeaponVisualData
		CachedWeaponVisualData = YcWeapon::GetWeaponVisualData(AssociatedWeaponInst);

		if (!IsValid(Decal) || !Decal.IsActive())
		{
			// 从 WeaponVisualData 获取贴花特效
			UNiagaraSystem DecalSystem = GetDecalSystem();
			if (DecalSystem == nullptr)
			{
				Print("WeaponDecalsEffector: 未找到贴花特效，请在 WeaponVisualData 中配置 Asset.Weapon.VFX.Decal", 3.0f, FLinearColor::Yellow);
				return;
			}

			Decal = Niagara::SpawnSystemAtLocation(DecalSystem, FVector(0), FRotator(0), FVector(1));
			bImpactTrigger = false;
		}

		SendTriggerUpdateToSystems();

		// Destroy actor If all systems are inactive only check every N seconds.
		AutoDestroyTimer = System::SetTimer(this, n"CheckToDestroy", 3, true);
	}

	UFUNCTION()
	void SendTriggerUpdateToSystems()
	{
		bImpactTrigger = !bImpactTrigger;
		Decal.SetNiagaraVariableBool("User.Trigger", bImpactTrigger);

		// 遍历ImpactSurfaceTypes转换为int32存放到SurfaceTypes
		SurfaceTypes.Empty();
		for (int32 i = 0; i < ImpactSurfaceTypes.Num(); i++)
		{
			int32 SurfaceType = int32(ImpactSurfaceTypes[i]);
			// @TODO 这里在正规的逻辑中应该取消掉,目前先这样用着
			// 为默认物理表面替换为水泥表面,这样至少默认能获得一个弹孔贴画效果
			SurfaceType = SurfaceType == 0 ? 2 : SurfaceType;
			SurfaceTypes.Add(SurfaceType);
		}

		// Set Niagara variables
		NiagaraDataInterfaceArray::SetNiagaraArrayInt32(Decal, n"User.ImpactSurfaces", SurfaceTypes);
		NiagaraDataInterfaceArray::SetNiagaraArrayPosition(Decal, n"User.ImpactPositions", ImpactPositions);
		NiagaraDataInterfaceArray::SetNiagaraArrayVector(Decal, n"User.ImpactNormals", ImpactNormals);
		// 设置User.NumberOfHits
		Decal.SetNiagaraVariableInt("User.NumberOfHits", ImpactPositions.Num());
	}

	UFUNCTION()
	void CheckToDestroy()
	{
		if (IsValid(Decal))
			return;
		System::ClearAndInvalidateTimerHandle(AutoDestroyTimer);
		DestroyActor();
	}

	UFUNCTION()
	void UpdateFireInfo(TArray<FVector> InImpactPositions, TArray<FVector> InImpactNormals, TArray<EPhysicalSurface> InImpactSurfaceTypes)
	{
		ImpactPositions = InImpactPositions;
		ImpactNormals = InImpactNormals;
		ImpactSurfaceTypes = InImpactSurfaceTypes;
	}

	// ========================================
	// 从 WeaponVisualData 获取特效
	// ========================================

	/** 获取贴花特效系统 */
	private UNiagaraSystem GetDecalSystem()
	{
		if (CachedWeaponVisualData == nullptr)
			return nullptr;

		return CachedWeaponVisualData.GetVFXEffect(GameplayTags::Asset_Weapon_VFX_Decal).Get();
	}
}
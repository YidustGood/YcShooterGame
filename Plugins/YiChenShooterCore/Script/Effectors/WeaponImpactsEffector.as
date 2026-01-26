USTRUCT()
struct FSurfaceImpacts
{
	TArray<FVector> ImpactPositions;
	TArray<FVector> ImpcatNormals;
}

/**
 * AWeaponImpactsEffector
 * 打击特效效果器，用于生成打击特效，可以根据不同物理表面生成不同的打击特效
 * 注意：特效资产从 WeaponVisualData 中获取，不再使用硬引用
 */
class AWeaponImpactsEffector : AActor
{
	// The Distance Threshold between 2 hits to spawn a new system
	UPROPERTY(EditDefaultsOnly)
	float DistanceThreshold = 500.0f;

	UPROPERTY(EditDefaultsOnly)
	UNiagaraDataChannel NiagaraDataChannel;

	// A toggle to switch spawning strategies. When true this blueprint will spawn impacts using a data channel instead of an array.
	bool bUseDataChannels;
	
	UPROPERTY(NotVisible, BlueprintReadOnly)
	TArray<UNiagaraComponent> ImpactNiagaraSysComps;

	TArray<FSurfaceImpacts> ImpactBuckets;
	TArray<EPhysicalSurface> ImpactSurfaceTypes;
	TArray<FVector> ImpactPositions;
	TArray<FVector> ImpactNormals;
	FVector MuzzlePosition;
	
	// 关联的武器实例，用于获取 VisualData
	UPROPERTY(ExposeOnSpawn, VisibleInstanceOnly)
	UYcEquipmentInstance AssociatedWeaponInst;
	
	const UYcWeaponVisualData CachedWeaponVisualData;

	/** 由创建者调用,传递最新的射击/命中信息,以便根据这些数据正确生成特效*/
	UFUNCTION()
	void UpdateFireInfo(TArray<FVector> InImpactPositions, TArray<FVector> InImpactNormals, TArray<EPhysicalSurface> InImpactSurfaceTypes)
	{
		ImpactPositions = InImpactPositions;
		ImpactNormals = InImpactNormals;
		ImpactSurfaceTypes = InImpactSurfaceTypes;
	}

	// 在射击后调用并传递击中数据以生产打击特效
	void SpawnFireImpactsEffects()
	{
		// 缓存 WeaponVisualData
		CachedWeaponVisualData = YcWeapon::GetWeaponVisualData(AssociatedWeaponInst);
		
		AddImpactsToBuckets();
		SpawnParticlesFromImpacts();
		ImpactPositions.Empty();
		ImpactNormals.Empty();
		ImpactSurfaceTypes.Empty();
		// Destroy actor If all systems are inactive only check every N seconds.
		// RetriggerableCheckDestroy();
	}

	void AddImpactsToBuckets()
	{
		// Clear previous impact data from buckets. / 清除桶中先前的击中数据。
		ImpactBuckets.Reset(ImpactSurfaceTypes.Num());
		for (int i = 0; i < ImpactSurfaceTypes.Num(); i++)
		{
			FSurfaceImpacts NewImpact;
			NewImpact.ImpactPositions = TArray<FVector>();
			NewImpact.ImpcatNormals = TArray<FVector>();
			ImpactBuckets.Add(NewImpact);
			// Impact.ImpactPositions = TArray<FVector>();
			// Impact.ImpcatNormals = TArray<FVector>();
		}

		// Bucket by surface type, to facilitate spawning different systems for different surfaces
		for (int i = 0; i < ImpactSurfaceTypes.Num(); i++)
		{
			// The surface enum can be used as an index directly, and will handle new entries being added to the enum automatically.
			if (ImpactBuckets.IsValidIndex(i))
			{
				// Add to existing arrays if a bucket is found.
				ImpactBuckets[i].ImpactPositions.Add(ImpactPositions[i]);
				ImpactBuckets[i].ImpcatNormals.Add(ImpactNormals[i]);
			}
			else
			{
				// If no bucket is found for this impact surface, initialize and add one.
				FSurfaceImpacts NewImpact;
				NewImpact.ImpactPositions.Add(ImpactPositions[i]);
				NewImpact.ImpcatNormals.Add(ImpactNormals[i]);
				ImpactBuckets[i] = NewImpact;
			}
		}
	}

	void SpawnParticlesFromImpacts()
	{
		for (int i = 0; i < ImpactBuckets.Num(); i++)
		{
			if (ImpactBuckets[i].ImpactPositions.IsEmpty() || ImpactBuckets[i].ImpcatNormals.IsEmpty())
				continue;

			/**
			 * The Use Data Channels flag needs to have its default set and compiled to change what spawn method we use. If you want a runtime toggle, you can edit the game mode or game state to have a similar flag, so its state can be changed globally.

			  *  Data channel support has only been added for Concrete and Glass impacts. So even if the flag is set to true, we deffer to array based spawning for other surfaces.
			*/
			// 转换为物理表面枚举
			EPhysicalSurface CurrentSurface = EPhysicalSurface(i);
			if ((CurrentSurface == EPhysicalSurface::SurfaceType2 || CurrentSurface == EPhysicalSurface::SurfaceType3) && bUseDataChannels)
			{
				SpawnFromDataChannels(CurrentSurface);
			}
			else
			{
				SpawnFromArrays(CurrentSurface);
			}
		}
	}

	void SpawnFromArrays(EPhysicalSurface SurfaceType)
	{
		ImpactNiagaraSysComps.Reserve(SurfaceType);
		if (!ImpactNiagaraSysComps.IsValidIndex(SurfaceType))
		{
			ImpactNiagaraSysComps.Add(nullptr);
		}
		
		// 从 WeaponVisualData 获取对应表面的特效
		UNiagaraSystem CurrentSystemTemplate = GetImpactSystemForSurface(SurfaceType);
		if (CurrentSystemTemplate == nullptr)
		{
			// 如果没有配置特定表面的特效，尝试使用默认特效
			CurrentSystemTemplate = GetDefaultImpactSystem();
			if (CurrentSystemTemplate == nullptr)
			{
				Print("WeaponImpactsEffector: 未找到表面 " + SurfaceType + " 的撞击特效，请在 WeaponVisualData 中配置", 3.0f, FLinearColor::Yellow);
				return;
			}
		}

		FVector CurrentPosition = ImpactBuckets[SurfaceType].ImpactPositions[0];

		// If no valid system is found, or if it's too far away, spawn a new one.
		bool bNeedSpawnSystem = !ImpactNiagaraSysComps.IsValidIndex(SurfaceType) ||
								!IsValid(ImpactNiagaraSysComps[SurfaceType]) ||
								CurrentPosition.Distance(ImpactNiagaraSysComps[SurfaceType].GetWorldLocation()) > DistanceThreshold;

		UNiagaraComponent ImpactSystem;
		if (bNeedSpawnSystem)
		{
			ImpactSystem = Niagara::SpawnSystemAtLocation(CurrentSystemTemplate, CurrentPosition);
			ImpactNiagaraSysComps[SurfaceType] = ImpactSystem;
		}
		else
		{
			ImpactSystem = ImpactNiagaraSysComps[SurfaceType];
		}
		ImpactSystem.SetWorldLocation(CurrentPosition);
		NiagaraDataInterfaceArray::SetNiagaraArrayPosition(ImpactSystem, n"User.ImpactPositions", ImpactBuckets[SurfaceType].ImpactPositions);
		NiagaraDataInterfaceArray::SetNiagaraArrayVector(ImpactSystem, n"User.ImpactNormals", ImpactBuckets[SurfaceType].ImpcatNormals);
		ImpactSystem.SetNiagaraVariableInt("NumberOfHits", ImpactBuckets[SurfaceType].ImpactPositions.Num());
		ImpactSystem.SetNiagaraVariableVec3("User.MuzzlePosition", MuzzlePosition);
		ImpactSystem.Activate();
	}

	// @TODO SpawnFromDataChannels
	void SpawnFromDataChannels(EPhysicalSurface SurfaceType)
	{
		// Assuming impacts are clustered. We search based on the first position, and write all positions to the same writer.
		// NiagaraDataChannel::WriteToNiagaraDataChannel();
	}

	// 定时重复检查销毁这个Actor的事件, 交由蓝图实现更便捷一些
	UFUNCTION(BlueprintEvent)
	void RetriggerableCheckDestroy()
	{
		// 蓝图中实现
	}
	
	// ========================================
	// 从 WeaponVisualData 获取特效
	// ========================================
	
	/** 根据物理表面类型获取对应的撞击特效 */
	private UNiagaraSystem GetImpactSystemForSurface(EPhysicalSurface SurfaceType)
	{
		if (CachedWeaponVisualData == nullptr)
			return nullptr;
		
		// 根据表面类型构建 Tag（需要在 GameplayTags 中定义对应的 Tag）
		// 例如: Asset.Weapon.VFX.Impact.Concrete, Asset.Weapon.VFX.Impact.Metal 等
		// 这里使用通用的 Impact Tag，实际项目中可能需要更细粒度的 Tag
		return CachedWeaponVisualData.GetVFXEffect(GameplayTags::Asset_Weapon_VFX_Impact).Get();
	}
	
	/** 获取默认撞击特效（当没有配置特定表面特效时使用） */
	private UNiagaraSystem GetDefaultImpactSystem()
	{
		if (CachedWeaponVisualData == nullptr)
			return nullptr;
		
		return CachedWeaponVisualData.GetVFXEffect(GameplayTags::Asset_Weapon_VFX_Impact).Get();
	}
}
// 射击AIBot角色基础类
class AYcShooterAICharacter : AYcSimpleAICharacter
{
	// 装备管理组件, 必须要有这个组件角色才能拥有装备物品的功能
	UPROPERTY(DefaultComponent)
	UYcEquipmentManagerComponent EquipmentManagerComp;

	// 上下文效果组件, 用于播放上下文效果(例如踩中不同物理材质表面播放不同的脚步声,生成不同的脚印等功能), 需要设置DefaultContextEffectsLibraries
	UPROPERTY(DefaultComponent)
	UYcContextEffectComponent ContextEffectComp;

	// 默认装备的武器物品ID
	UPROPERTY(Category = "Equipment")
	FDataRegistryId DefaultToEquipWeaponItemId;

	UPROPERTY(Category = "Equipment")
	int32 DefaultToEquipWeaponQuickSlotIndex = 0;

	// 是否开启无限子弹模式
	UPROPERTY(Category = "Equipment")
	bool bEnableUnlimitedAmmo = true;

	private AYcShooterAIController ShooterAIController;

	UFUNCTION(BlueprintOverride)
	void Possessed(AController NewController)
	{
		ShooterAIController = Cast<AYcShooterAIController>(NewController);
		if (ShooterAIController == nullptr)
		{
			Error(f"{GetName()}: ShooterAIController is nullptr! Check the controller class is YcShooterAIController.");
			return;
		}
		DelayUntilNextTickForAs(n"InitializeShooterBot");
	}

	UFUNCTION(NotBlueprintCallable)
	void InitializeShooterBot()
	{
		// 如果配置了默认武器就装备默认武器
		if (DefaultToEquipWeaponItemId.IsValid())
		{
			EquipWeapon(DefaultToEquipWeaponItemId, DefaultToEquipWeaponQuickSlotIndex);
		}
	}

	// 为AIBot装备指定武器
	UFUNCTION()
	bool EquipWeapon(FDataRegistryId WeaponItemId, int32 QuickSlotIndex)
	{
		if (ShooterAIController == nullptr)
		{
			Error(f"{GetName()}: ShooterAIController is nullptr! Check the controller class is YcShooterAIController.");
			return false;
		}

		if (!WeaponItemId.IsValid())
		{
			Error(f"{GetName()}: WeaponItemId is invalid!");
			return false;
		}

		auto WeaponItemInst = ShooterAIController.InventoryManager.AddItem(WeaponItemId, 1);
		if (WeaponItemInst == nullptr)
		{
			Error(f"{GetName()}: AddItem failed!");
			return false;
		}

		if (!ShooterAIController.QuickBar.AddItemToSlot(QuickSlotIndex, WeaponItemInst))
		{
			Error(f"{GetName()}: AddItemToSlot failed!");
			return false;
		}
		ShooterAIController.QuickBar.SetActiveSlotIndex_WithPrediction(QuickSlotIndex);

		// 为武器弹匣内添加99999个子弹
		if (bEnableUnlimitedAmmo)
		{
			WeaponItemInst.AddStatTagStack(GameplayTags::Weapon_Stat_MagazineAmmo, 99999);
		}

		return true;
	}
}
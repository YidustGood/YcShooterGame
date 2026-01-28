/**
 * 行为树服务：Bot 射击控制
 *
 * 功能说明：
 * - 每帧检查武器弹药状态
 * - 有弹药时持续射击
 * - 弹匣空时自动换弹
 * - 所有武器都没弹药时切换武器或标记弹药耗尽
 *
 * 使用方式：
 * 1. 在行为树的战斗节点上添加此服务
 * 2. 配置 OutOfAmmo 黑板键（标记弹药耗尽状态）
 * 3. 配置 TargetEnemy 黑板键（存储目标敌人）
 *
 * 弹药管理逻辑：
 * - 当前武器有弹药 → 持续射击
 * - 弹匣空但有备弹 → 触发自动换弹
 * - 当前武器完全没弹药 → 切换到有弹药的武器
 * - 所有武器都没弹药 → 设置 OutOfAmmo 标记
 */
class UBTService_YcBotShoot : UBTService_BlueprintBase
{
	/** 黑板键：弹药耗尽标记 */
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector OutOfAmmo;
	default OutOfAmmo.SelectedKeyName = n"OutOfAmmo";

	/** 黑板键：目标敌人 */
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector TargetEnemy;
	default TargetEnemy.SelectedKeyName = n"TargetEnemy";

	/**
	 * 服务每帧更新时调用
	 * 检查武器状态并触发射击
	 *
	 * @param OwnerController AI 控制器
	 * @param ControlledPawn 被控制的 Pawn
	 * @param DeltaSeconds 距离上一帧的时间间隔
	 */
	UFUNCTION(BlueprintOverride)
	void TickAI(AAIController OwnerController, APawn ControlledPawn, float DeltaSeconds)
	{
		if (!IsValid(ControlledPawn))
			return;

		// 检查当前武器状态，如果可以射击则发送射击事件
		if (GetCurrentWeapon(ControlledPawn, OwnerController))
		{
			AbilitySystem::SendGameplayEventToActor(ControlledPawn, GameplayTags::InputTag_Weapon_Fire, FGameplayEventData());
		}
	}

	/**
	 * 检查当前武器状态并处理弹药管理
	 *
	 * 逻辑流程：
	 * 1. 检查当前武器是否有弹药（弹匣+备弹）
	 * 2. 如果有弹药，返回 true 允许射击
	 * 3. 如果弹匣空但有备弹，返回 false 触发自动换弹
	 * 4. 如果当前武器完全没弹药，尝试切换到有弹药的武器
	 * 5. 如果所有武器都没弹药，设置 OutOfAmmo 标记
	 *
	 * @param Pawn Bot 的 Pawn
	 * @param Controller Bot 的控制器
	 * @return true 表示可以射击，false 表示需要换弹或切换武器
	 */
	bool GetCurrentWeapon(APawn Pawn, AController Controller)
	{
		if (!IsValid(Pawn))
			return false;

		// 获取当前装备的武器实例
		auto QuickBar = Pawn.GetComponentByClass(UYcQuickBarComponent);
		auto WeaponInst = QuickBar.GetActiveEquipmentInstance();
		if (!IsValid(WeaponInst))
			return false;

		// 获取武器的物品实例（包含弹药数据）
		auto CurrentWeaponDef = Cast<UYcInventoryItemInstance>(WeaponInst.Instigator);

		// 检查当前武器的弹药状态
		auto SpareAmmo = CurrentWeaponDef.GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo);
		auto MagazineAmmo = CurrentWeaponDef.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
		bool bHaveAmmo = SpareAmmo + MagazineAmmo > 0;

		// 情况1：当前武器还有弹药（弹匣或备弹），可以继续射击
		if (bHaveAmmo)
			return true;

		// 情况2：弹匣空但有备弹，需要换弹
		bool bNoMagazineAmmo = MagazineAmmo <= 0 && SpareAmmo > 0;
		if (bNoMagazineAmmo)
			return false; // 返回 false 会触发自动换弹逻辑

		// 情况3：当前武器完全没弹药，尝试切换到有弹药的武器
		bool bHasMoreWeapon = false;
		auto InventoryMgr = Controller.GetComponentByClass(UYcInventoryManagerComponent);
		auto AllItemInst = InventoryMgr.GetAllItemInstance();

		// 遍历所有武器，查找有弹药的武器
		for (int i = 0; i < AllItemInst.Num(); i++)
		{
			// 检查该武器是否有弹药
			int32 ItemSpareAmmo = AllItemInst[i].GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo);
			int32 ItemMagazineAmmo = AllItemInst[i].GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
			bool bItemHaveAmmo = ItemSpareAmmo + ItemMagazineAmmo > 0;

			// 跳过当前武器或没弹药的武器
			if (CurrentWeaponDef == AllItemInst[i] || !bItemHaveAmmo)
				continue;

			// 找到有弹药的武器，切换到该武器
			FGameplayEventData Payload;
			Payload.EventMagnitude = i; // 武器在快捷栏中的索引
			AbilitySystem::SendGameplayEventToActor(Pawn, GameplayTags::InputTag_Ability_QuickBar_SelectSlot, Payload);
			bHasMoreWeapon = true;
			break;
		}

		// 如果成功切换到有弹药的武器，返回 true 继续射击
		if (bHasMoreWeapon)
		{
			return true;
		}

		// 情况4：所有武器都没弹药，设置弹药耗尽标记
		BT::SetBlackboardValueAsBool(this, OutOfAmmo, true);
		return false;
	}
}
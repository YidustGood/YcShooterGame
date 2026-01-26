/* 射线武器实例类 **/
class UYcHitScanWeaponInstanceScirpt : UYcHitScanWeaponInstance
{

	UFUNCTION(BlueprintOverride)
	void OnEquipped()
	{
		APawn OwnerPawn = GetPawn();
		if (OwnerPawn != nullptr && OwnerPawn.PlayerState != nullptr)
		{
			// 武器装备的第一时间还不能开火, 等Unholster动画播放完成通过动画通知UAnimNotify_SetWeaponCanFire中处理允许开火
			UAbilitySystemComponent ASC = UAbilitySystemComponent::Get(OwnerPawn.PlayerState);
			if (ASC != nullptr)
			{
				ASC.AddLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction, 1);
			}
		}

		OnEquippedEvent.AddUFunction(this, n"OnEquipAnimEnd");
	}

	// 武器装备动画结束恢复可开火状态
	UFUNCTION()
	void OnEquipAnimEnd(UYcEquipmentInstance EquipmentInst)
	{
		APawn OwnerPawn = GetPawn();
		if (OwnerPawn != nullptr && OwnerPawn.PlayerState != nullptr)
		{
			UAbilitySystemComponent ASC = UAbilitySystemComponent::Get(GetPawn().PlayerState);
			if (ASC != nullptr)
			{
				ASC.RemoveLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction, 999);
			}
		}
	}
}
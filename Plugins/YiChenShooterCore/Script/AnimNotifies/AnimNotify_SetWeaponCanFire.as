
/**
 * 动画通知
 * 用于通过动画通知设置玩家武器是否可以开火
 */
class UAnimNotify_SetWeaponCanFire : UAnimNotify
{
	UPROPERTY(EditAnywhere, EditInstanceOnly)
	bool bCanFire;

	UFUNCTION(BlueprintOverride)
	FString GetNotifyName() const
	{
		return "Set Weapn Can Fire(YcShooterCore)";
	}

	UFUNCTION(BlueprintOverride)
	bool Notify(USkeletalMeshComponent MeshComp, UAnimSequenceBase Animation,
				FAnimNotifyEventReference EventReference) const
	{
		APawn PlayerPawn = Cast<APawn>(MeshComp.GetOwner());
		if (PlayerPawn == nullptr)
			return true;

		UAbilitySystemComponent ASC = UAbilitySystemComponent::Get(PlayerPawn.PlayerState);

		if (bCanFire)
		{
			// 这里传入9999是为了避免多次叠加NoFireAction导致无法正确恢复可开火状态的问题
			ASC.RemoveLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction, 9999);
		}
		else
		{
			ASC.AddLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction, 1);
		}

		return true;
	}
}
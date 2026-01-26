
/**
 * 动画通知
 * 用于通知玩家换弹动作中弹夹插入
 */
class UAnimNotify_MagazineReloadMagIn : UAnimNotify
{
	UFUNCTION(BlueprintOverride)
	FString GetNotifyName() const
	{
		return "Notify Reload Magazine In (YcShooterCore)";
	}

	UFUNCTION(BlueprintOverride)
	bool Notify(USkeletalMeshComponent MeshComp, UAnimSequenceBase Animation,
				FAnimNotifyEventReference EventReference) const
	{
		if (MeshComp == nullptr || MeshComp.GetOwner() == nullptr)
			return true;

		AbilitySystem::SendGameplayEventToActor(MeshComp.GetOwner(), GameplayTags::Yc_Weapon_AnimNotify_ReloadMagIn, FGameplayEventData());
		return true;
	}
}
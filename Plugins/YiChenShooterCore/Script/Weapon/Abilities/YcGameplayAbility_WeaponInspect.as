class UYcGameplayAbility_WeaponInspect : UYcGameplayAbility_WeaponBase
{
	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_Inspect);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_Inspect);

	// 换弹期间不能做检视动作
	default ActivationBlockedTags.AddTag(GameplayTags::InputTag_Weapon_Reload);

	FYcWeaponActionVisual InspectActionVisual;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		Super::OnAbilityAdded();

		WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Inspect, InspectActionVisual);
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		FGameplayTagContainer Container;
		Container.AddTag(GameplayTags::InputTag_Weapon_Reload);
		bool bHas = GetAbilitySystemComponentFromActorInfo().HasMatchingGameplayTag(Container);
		Print(f"{bHas}");
		auto PlayMontageTask =
			AsYcGameCoreGameTask::PlayMontageOnSkeletalMeshAndWait(
				this,
				n"",
				nullptr,
				n"FPCharacter", // 第一人称手臂模型上要添加"FPCharacter"这个Tag才可以找到,
				InspectActionVisual.FPCharacterAnimMontage.Get(),
				FGameplayTagContainer());

		PlayMontageTask.OnCompleted.AddUFunction(this, n"OnInspectDone");
		PlayMontageTask.OnBlendOut.AddUFunction(this, n"OnInspectDone");
		PlayMontageTask.OnInterrupted.AddUFunction(this, n"OnInspectDone");
		PlayMontageTask.OnCancelled.AddUFunction(this, n"OnInspectDone");
		PlayMontageTask.ReadyForActivation();

		//@TODO 具体动画干脆由具体Actor选择, FP和TP是不一样的
		// 通知FP武器Actor执行检视动画
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags::InputTag_Weapon_Inspect;
		Payload.OptionalObject = InspectActionVisual.FPWeaponAnimMontage.Get();
		AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::InputTag_Weapon_Inspect, Payload);
	}

	UFUNCTION()
	void OnInspectDone(FGameplayTag EventTag, FGameplayEventData EventData)
	{
		EndAbility();
	}
}
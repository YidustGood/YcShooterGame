/**
 * HitScan武器射击技能
 * 需要在蓝图继承然后配置AdditionalCosts来实现射击消耗子弹
 */
class UYcGameplayAbility_WeaponHitScanFire : UYcGameplayAbility_HitScanWeapon
{
	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_Fire);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_Fire);

	// 如果有Ability_Weapon_NoFiring标签就不能开火, 例如游戏对局结束后如果不让玩家继续开火就添加这个标签
	default SourceBlockedTags.AddTag(GameplayTags::Ability_Weapon_NoFiring);

	// 这个阻止武器开火是由于武器正在进行某些不允许开火的动作, 例如装备动作期间就不允许开火
	default ActivationBlockedTags.AddTag(GameplayTags::Weapon_State_NoFireAction);

	default ActivationPolicy = EYcAbilityActivationPolicy::WhileInputActive;

	// 添加EventTrigger, 便于AI等系统触发射击能力
	FAbilityTriggerData TriggerData;
	default TriggerData.TriggerTag = GameplayTags::InputTag_Weapon_Fire;
	default TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	default AbilityTriggers.Add(TriggerData);

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageGE;

	UPROPERTY()
	FGameplayTag GameplayCueTag_Fire;
	default GameplayCueTag_Fire = GameplayTags::GameplayCue_Weapon_Fire;

	UPROPERTY()
	FGameplayTag GameplayCueTag_Impact;
	default GameplayCueTag_Impact = GameplayTags::GameplayCue_Weapon_Impact;

	// 缓存WeaponVisualData资产, 方便获取动画之类的资产供技能使用
	UPROPERTY(VisibleInstanceOnly)
	UYcWeaponVisualData WeaponVisualDataCache;

	FYcWeaponActionVisual FireHipActionVisual;
	FGameplayCueParameters GCNParameter;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		WeaponVisualDataCache = YcWeapon::GetWeaponVisualData(GetAssociatedEquipment());
		if (WeaponVisualDataCache != nullptr)
		{
			WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Fire_Hip, FireHipActionVisual);
		}
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbilityFromEvent(FGameplayEventData EventData)
	{
		ActivateFire();
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		ActivateFire();
	}

	// 激活开火函数, 包装起来方便上面两个激活途径调用
	private void ActivateFire()
	{
		auto WaitInputRelease = AngelscriptAbilityTask::WaitInputRelease(this);
		WaitInputRelease.OnRelease.AddUFunction(this, n"OnInputRelease");
		WaitInputRelease.ReadyForActivation();
		if (IsLocallyControlled())
		{
			StartFiring();
		}
	}

	UFUNCTION(BlueprintOverride)
	void OnShotFired(int ShotIndex)
	{
		ApplyLastShotRecoil();

		// 通知武器Actor播放射击动画
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags::InputTag_Weapon_Fire;
		Payload.OptionalObject = FireHipActionVisual.FPWeaponAnimMontage.Get();
		AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::InputTag_Weapon_Fire, Payload);

		// 通知玩家角色播放第一人称模型射击动画
		FGameplayEventData CharacterPayloadFP;
		CharacterPayloadFP.EventTag = GameplayTags::GameplayEvent_Character_PlayMontageFP;
		CharacterPayloadFP.OptionalObject = FireHipActionVisual.FPCharacterAnimMontage.Get();
		AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::GameplayEvent_Character_PlayMontageFP, CharacterPayloadFP);

		// @TODO 后续有事件再来优化一下射击技能的自动结束机制
		// 如果是AIBot那么没法触发OnInputRelease, 所以我们直接在这里检查如果是AI控制器直接自动结束射击技能, 避免AI无法进行下一次射击
		CheckAIEndFire();
	}

	UFUNCTION()
	void OnInputRelease(float32 TimeHeld)
	{
		StopFiring(true);
	}

	// 服务端/主控客户端都会触发
	UFUNCTION(BlueprintOverride)
	void OnRangedWeaponTargetDataReady(FGameplayAbilityTargetDataHandle TargetData)
	{
		// 射击数据准备完成, 现在开始应用伤害等逻辑

		// 从武器实例获取当前的武器基础伤害值设置到DamageGE, 然后应用到命中目标对象上
		float BaseDamage = GetWeaponInstance().GetComputedStats().BaseDamage;
		auto EffectSpecHandle =
			AbilitySystem::AssignTagSetByCallerMagnitude(MakeOutgoingGameplayEffectSpec(DamageGE), GameplayTags::Weapon_Stat_Damage_Base, BaseDamage);
		ApplyGameplayEffectSpecToTarget(EffectSpecHandle, TargetData);

		// 广播命中消息, 准星UI可以监听做命中反馈效果
		auto MessageSubsystem = UGameplayMessageSubsystem::Get();
		FHitCharacterMessage HitMessage;
		HitMessage.Hitter = GetControllerFromActorInfo();
		HitMessage.HitResult = AbilitySystem::GetHitResultFromTargetData(TargetData, 0);
		MessageSubsystem.BroadcastMessage(GameplayTags::GameplayCue_Character_DamageTaken, HitMessage);

		// 处理GameplayCue
		GCNParameter = GameplayCue::MakeGameplayCueParametersFromHitResult(HitMessage.HitResult);
		ExecuteGameplayCueWithParams(GameplayCueTag_Fire, GCNParameter);

		if (HitMessage.HitResult.bBlockingHit)
		{
			ExecuteGameplayCueWithParams(GameplayCueTag_Impact, GCNParameter);
		}
	}

	void CheckAIEndFire()
	{
		if (GetControllerFromActorInfo().IsA(AAIController))
		{
			EndAbility();
		}
	}
}
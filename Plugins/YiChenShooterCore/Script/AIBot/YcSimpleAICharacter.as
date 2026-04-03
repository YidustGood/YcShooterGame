/**
 * 简单的AI角色类
 * 区别于需要PlayerState的PVP AI Bot, 这个是不需要PlayerState的AI角色类
 * 适用场景: PVE AI Bot 以及无需PlayerState的AI角色类
 */
class AYcSimpleAICharacter : AYcCharacter
{
	UPROPERTY(DefaultComponent)
	UYcAbilitySystemComponent AbilitySystemComp;

	UPROPERTY(DefaultComponent)
	UYcAICharacterComponent AICharacterComp;

	/** 必须要有死亡Ability类才能让YcHealthComp正确运行 */
	UPROPERTY()
	TSubclassOf<UYcGameplayAbility_DeathBase> DeathAbilityClass;

	default DeathAbilityClass = UYcGameplayAbility_DefaultDeath;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		if (DeathAbilityClass == nullptr)
		{
			Error(f"{GetName()}: DeathAbilityClass is nullptr!");
			return;
		}

		// 授予死亡Ability类
		if (HasAuthority())
		{
			AbilitySystemComp.GiveAbility(DeathAbilityClass);
		}

		// 绑定血量变化、死亡开始、死亡完成的回调事件
		HealthComponent.OnDeathStarted.AddUFunction(this, n"OnDeath_Callback");
		HealthComponent.OnDeathFinished.AddUFunction(this, n"OnDeathFinished_Callback");
		HealthComponent.OnHealthChanged.AddUFunction(this, n"OnHealthChanged_Callback");
	}

	/// 可在子类重写下列函数, 实现自定义逻辑
	UFUNCTION(BlueprintEvent)
	void OnHealthChanged_Callback(UYcHealthComponent HealthComp, float32 OldValue, float32 NewValue, AActor InInstigator)
	{
	}

	UFUNCTION(BlueprintEvent)
	void OnDeath_Callback(AActor Actor)
	{
	}

	UFUNCTION(BlueprintEvent)
	void OnDeathFinished_Callback(AActor Actor)
	{
	}
}
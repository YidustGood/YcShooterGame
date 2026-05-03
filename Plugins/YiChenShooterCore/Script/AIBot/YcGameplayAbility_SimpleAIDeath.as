/**
 * 简单 AI 的默认死亡能力。
 * 与 GameCore 的默认死亡能力不同，这里会保留一段尸体时间，
 * 方便角色进入 ragdoll，而不是下一帧就走到 DeathFinished 被基类销毁。
 */
class UYcGameplayAbility_SimpleAIDeath : UYcGameplayAbility_DeathBase
{
	UPROPERTY()
	float CorpseRemainTime;

	default CorpseRemainTime = 8.0f;

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		if (CorpseRemainTime <= 0.0f)
		{
			DelayUntilNextTickForAs(n"EndDeath");
			return;
		}

		DelayForAs(n"EndDeath", CorpseRemainTime);
	}

	UFUNCTION()
	void EndDeath()
	{
		EndAbility();
	}
}

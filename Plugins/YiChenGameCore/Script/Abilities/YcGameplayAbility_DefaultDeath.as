/**
 * 默认的死亡能力
 * 我们将死亡的过程定义为一个序列, 可以有死亡开始和死亡结束, 允许开始与结束之间有一定的时间间隔
 * ActivateAbility的时候为死亡开始, 调用了EndAbility后就是死亡结束
 * 所以要使用YcHealthComponent来管理角色的健康状态都需要给它添加一个死亡能力, 无特殊需求可以直接用这个
 * 有特殊需求就自己继承UYcGameplayAbility_DeathBase去实现定制化的逻辑
 */
class UYcGameplayAbility_DefaultDeath : UYcGameplayAbility_DeathBase
{
	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		DelayUntilNextTickForAs(n"EndDeath");
	}

	UFUNCTION()
	void EndDeath()
	{
		EndAbility();
	}
}
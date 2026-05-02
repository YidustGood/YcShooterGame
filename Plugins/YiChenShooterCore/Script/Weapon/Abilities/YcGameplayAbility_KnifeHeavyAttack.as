/**
 * 近战武器重击
 * 默认复用 ADS 输入，便于直接绑定到右键
 */
class UYcGameplayAbility_KnifeHeavyAttack : UYcGameplayAbility_KnifeAttackBase
{
	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_ADS);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_ADS);
	default AttackType = EYcMeleeAttackType::Heavy;

	FAbilityTriggerData TriggerData;
	default TriggerData.TriggerTag = GameplayTags::InputTag_Weapon_ADS;
	default TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	default AbilityTriggers.Add(TriggerData);
}

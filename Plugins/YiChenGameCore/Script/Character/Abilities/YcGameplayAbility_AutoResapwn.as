USTRUCT()
struct FYcPlayerResetMessage
{
	UPROPERTY()
	TObjectPtr<AActor> OwnerPlayerState;
};

//@TOOD 待将蓝图版本的编写为AS版本
// /**
//  * 自动重生技能
//  * 一般作为被动技能赋予玩家, 因为ASC挂载在PlayerState上的所以Pawn死亡也不影响这个技能的生命周期
//  */
// class UYcGameplayAbility_AutoResapwn : UYcGameplayAbility{}
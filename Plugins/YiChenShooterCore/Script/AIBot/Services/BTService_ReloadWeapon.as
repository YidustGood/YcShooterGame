/**
 * 行为树服务：武器换弹
 * 
 * 功能说明：
 * - 在行为树节点激活时触发武器换弹
 * - 通过发送 Gameplay 事件来触发换弹能力
 * 
 * 使用方式：
 * 1. 在行为树中添加此服务节点
 * 2. 将其附加到需要换弹的行为节点上
 * 3. 服务激活时会自动触发换弹
 * 
 * 注意事项：
 * - 换弹能力需要在 Bot 的 ASC 中已授予
 * - 如果弹匣已满或没有备弹，换弹能力会自动失败
 */
class UBTService_ReloadWeapon : UBTService_BlueprintBase
{
	/**
	 * 服务激活时调用
	 * 发送换弹事件到 Bot 的 Pawn
	 * 
	 * @param OwnerController AI 控制器
	 * @param ControlledPawn 被控制的 Pawn
	 */
	UFUNCTION(BlueprintOverride)
	void ActivationAI(AAIController OwnerController, APawn ControlledPawn)
	{
		// 发送换弹输入事件，触发换弹能力
		AbilitySystem::SendGameplayEventToActor(ControlledPawn, GameplayTags::InputTag_Weapon_Reload, FGameplayEventData());
	}
}
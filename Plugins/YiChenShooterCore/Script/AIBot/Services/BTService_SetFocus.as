/**
 * 行为树服务：设置 AI 焦点
 * 
 * 功能说明：
 * - 控制 AI 控制器的焦点目标
 * - 设置焦点后，AI 会自动朝向目标
 * - 可以清除焦点以恢复正常朝向
 * 
 * 使用方式：
 * 1. 在行为树中添加此服务节点
 * 2. 配置 TargetEnemy 黑板键（存储目标敌人）
 * 3. 设置 bSetFocus 为 true 启用焦点，false 清除焦点
 * 
 * 应用场景：
 * - 战斗时锁定敌人目标
 * - 追击时保持朝向敌人
 * - 脱离战斗时清除焦点
 */
class UBTService_SetFocus : UBTService_BlueprintBase
{
	/** 黑板键：目标敌人 */
	UPROPERTY(EditAnywhere)
	FBlackboardKeySelector TargetEnemy;
	default TargetEnemy.SelectedKeyName = n"TargetEnemy";

	/** 是否设置焦点（true=设置焦点，false=清除焦点） */
	UPROPERTY(EditAnywhere)
	bool bSetFocus;

	/**
	 * 服务激活时调用
	 * 根据配置设置或清除 AI 焦点
	 * 
	 * @param OwnerController AI 控制器
	 * @param ControlledPawn 被控制的 Pawn
	 */
	UFUNCTION(BlueprintOverride)
	void ActivationAI(AAIController OwnerController, APawn ControlledPawn)
	{
		if (bSetFocus)
		{
			// 从黑板获取目标敌人
			auto AIController = AIHelper::GetAIController(OwnerController);
			AActor TargetActor = BT::GetBlackboardValueAsActor(this, TargetEnemy);
			
			// 设置焦点到目标敌人（AI 会自动朝向该目标）
			AIController.SetFocus(TargetActor);
		}
		else
		{
			// 清除焦点，恢复正常朝向控制
			OwnerController.ClearFocus();
		}
	}
}
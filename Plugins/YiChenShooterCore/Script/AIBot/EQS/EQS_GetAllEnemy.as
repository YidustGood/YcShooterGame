/**
 * EQS 生成器：获取所有敌人
 *
 * 功能说明：
 * - 用于环境查询系统（EQS）生成敌人目标列表
 * - 自动过滤友军和死亡的角色
 * - 只返回存活的敌对角色
 *
 * 使用方式：
 * 1. 在 EQS 查询中添加此生成器
 * 2. 生成器会自动查找所有敌对角色
 * 3. 配合 EQS 测试节点进行进一步筛选（距离、视线等）
 *
 * 过滤规则：
 * - 跳过友军（相同队伍）
 * - 跳过中立单位
 * - 跳过没有健康组件的角色
 * - 跳过已死亡或正在死亡的角色
 *
 * @TODO 这个是AI系统每帧都会调用到的, 后续可以考虑移至C++实现, 能够使用开销更低的队伍比较接口, 减少性能开销
 */
class UEQS_GetAllEnemy : UEnvQueryGenerator_BlueprintBase
{
	/**
	 * 从场景中的 Actor 生成查询项
	 * 遍历所有角色，筛选出存活的敌对角色
	 *
	 * @param ContextActors 上下文 Actor 列表（通常包含查询者）
	 */
	UFUNCTION(BlueprintOverride)
	void DoItemGenerationFromActors(TArray<AActor> ContextActors) const
	{
		// 获取场景中所有角色
		TArray<AActor> Characters;
		Gameplay::GetAllActorsOfClass(AYcCharacter, Characters);

		// 获取查询者（通常是 AI Bot）
		auto QuerierChar = Cast<AYcCharacter>(GetQuerier());

		// 获取队伍子系统，用于判断敌友关系
		auto TeamSubsystem = UYcTeamSubsystem::Get();

		// 遍历所有角色，筛选敌人
		for (auto Character : Characters)
		{
			// 比较队伍关系
			auto TeamComparisonRes = TeamSubsystem.CompareTeams(Character, QuerierChar);

			// 跳过友军和中立单位，只保留敌对角色
			if (TeamComparisonRes != EYcTeamComparison::DifferentTeams)
				continue;

			// 获取健康组件
			auto HealthComp = Character.GetComponentByClass(UYcHealthComponent);

			// 跳过没有健康组件或已死亡的角色
			if (HealthComp == nullptr || HealthComp.IsDeadOrDying())
				continue;

			// 将有效的敌人添加到生成列表
			AddGeneratedActor(Character);
		}
	}
}
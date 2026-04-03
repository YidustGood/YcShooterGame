// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_TeamRules.generated.h"

/**
 * 团队规则组件
 * 检查团队伤害规则，决定是否允许造成伤害
 * 
 * 执行优先级: 20
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Team Rules"))
class YICHENDAMAGE_API UYcDamageComponent_TeamRules : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_TeamRules();

protected:
	/** 是否启用团队规则检查 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bEnableTeamRules = true;

	/** 是否允许自伤 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bAllowSelfDamage = false;

	/** 是否允许友军伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bAllowFriendlyFire = false;

	/** 友军伤害倍率（仅当 bAllowFriendlyFire 为 true 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bAllowFriendlyFire", ClampMin = "0.0", ClampMax = "10.0"))
	float FriendlyFireDamageMultiplier = 1.0f;

	/** 敌人伤害倍率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float EnemyDamageMultiplier = 1.0f;

	/** 中立对象伤害倍率（无团队或未知团队） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float NeutralDamageMultiplier = 1.0f;

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Components/YcDamageComponent_TeamRules.h"

#include "AbilitySystemComponent.h"
#include "YiChenTeams/Public/YcTeamSubsystem.h"
#include "Library/YcDamageBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_TeamRules)

UYcDamageComponent_TeamRules::UYcDamageComponent_TeamRules()
{
	Priority = 20;
	DebugName = TEXT("TeamRules");
}

void UYcDamageComponent_TeamRules::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!bEnableTeamRules)
	{
		return;
	}

	if (!Params.SourceASC || !Params.TargetASC)
	{
		return;
	}

	AActor* SourceActor = Params.SourceASC->GetAvatarActor_Direct();
	AActor* TargetActor = Params.TargetASC->GetAvatarActor_Direct();

	if (!SourceActor || !TargetActor)
	{
		return;
	}

	// 检查自伤
	if (SourceActor == TargetActor)
	{
		if (!bAllowSelfDamage)
		{
			UYcDamageBlueprintLibrary::CancelExecution(Params);
			LogDebug(TEXT("Blocked: Self damage not allowed"));
			return;
		}
		// 自伤不应用倍率
		return;
	}

	// 获取团队信息
	const int32 SourceTeam = UYcTeamSubsystem::FindTeamFromObject(SourceActor);
	const int32 TargetTeam = UYcTeamSubsystem::FindTeamFromObject(TargetActor);

	// 判断团队关系并应用倍率
	bool bIsFriendly = false;
	bool bIsEnemy = false;
	bool bIsNeutral = false;

	if (SourceTeam != INDEX_NONE && TargetTeam != INDEX_NONE)
	{
		if (SourceTeam == TargetTeam)
		{
			// 同队
			bIsFriendly = true;
		}
		else
		{
			// 敌队
			bIsEnemy = true;
		}
	}
	else
	{
		// 一方或双方无团队
		bIsNeutral = true;
	}

	// 处理友军伤害
	if (bIsFriendly)
	{
		if (!bAllowFriendlyFire)
		{
			UYcDamageBlueprintLibrary::CancelExecution(Params);
			LogDebug(TEXT("Blocked: Friendly fire not allowed"));
			return;
		}
		
		// 应用友军伤害倍率
		if (FriendlyFireDamageMultiplier != 1.0f)
		{
			Params.Coefficient *= FriendlyFireDamageMultiplier;
			LogDebug(FString::Printf(TEXT("Friendly fire multiplier applied: %.2f"), FriendlyFireDamageMultiplier));
		}
	}
	// 应用敌人伤害倍率
	else if (bIsEnemy)
	{
		if (EnemyDamageMultiplier != 1.0f)
		{
			Params.Coefficient *= EnemyDamageMultiplier;
			LogDebug(FString::Printf(TEXT("Enemy damage multiplier applied: %.2f"), EnemyDamageMultiplier));
		}
	}
	// 应用中立对象伤害倍率
	else if (bIsNeutral)
	{
		if (NeutralDamageMultiplier != 1.0f)
		{
			Params.Coefficient *= NeutralDamageMultiplier;
			LogDebug(FString::Printf(TEXT("Neutral damage multiplier applied: %.2f"), NeutralDamageMultiplier));
		}
	}
}

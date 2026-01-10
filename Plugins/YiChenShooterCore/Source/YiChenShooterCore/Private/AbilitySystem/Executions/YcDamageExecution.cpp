// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AbilitySystem/Executions/YcDamageExecution.h"

#include "YcAbilitySourceInterface.h"
#include "YcGameplayEffectContext.h"
#include "YcTeamSubsystem.h"
#include "AbilitySystem/Attributes/YcCombatSet.h"
#include "AbilitySystem/Attributes/YcShooterHealthSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageExecution)

/**
 * 伤害计算的静态数据
 * 用于缓存属性捕获定义，避免重复创建
 */
struct FDamageStatics
{
	/** 基础伤害属性捕获定义，从源对象的CombatSet中捕获BaseDamage */
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;

	FDamageStatics()
	{
		// bSnapshot = false: 每次执行时获取实时的BaseDamage值，而不是GE Spec创建时的快照值
		// 对于周期性伤害GE，需要实时值以反映属性变化（如buff/debuff的添加或移除）
		// 注意：一次性伤害通常使用 bSnapshot = true（快照值），周期性伤害使用 false（实时值）
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UYcCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
	}
};

/**
 * 获取伤害计算静态数据（单例）
 * @return 静态数据引用
 */
static FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UYcDamageExecution::UYcDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().BaseDamageDef);
}

void UYcDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FYcGameplayEffectContext* TypedContext = FYcGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(TypedContext);
	
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;
	
	// 从攻击者的CombatSet属性集中获取BaseDamage, 也就是当前武器的基础伤害值
	// 攻击者的CombatSet属性集由UYcShooterCombatComponent组件提供和维护
	// 注意：BaseDamageDef的bSnapshot参数设置为false，确保每次执行时都获取实时的属性值
	// 这对于周期性GE很重要，因为属性值可能在GE执行期间发生变化（如buff/debuff的添加或移除）
	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDamageDef, EvaluateParameters, BaseDamage);
	
	AActor* EffectCauser = TypedContext->GetEffectCauser();
	const FHitResult* HitActorResult = TypedContext->GetHitResult();
	
	AActor* HitActor = nullptr;
	FVector ImpactLocation = FVector::ZeroVector;
	FVector ImpactNormal = FVector::ZeroVector;
	FVector StartTrace = FVector::ZeroVector;
	FVector EndTrace = FVector::ZeroVector;
	
	// 计算命中Actor、表面、区域和距离都依赖于是否有命中结果
	// 直接添加的效果（没有目标）总是没有命中结果，必须使用一些回退信息
	if (HitActorResult)
	{
		const FHitResult& CurHitResult = *HitActorResult;
		HitActor = CurHitResult.HitObjectHandle.FetchActor();
		if (HitActor)
		{
			ImpactLocation = CurHitResult.ImpactPoint;
			ImpactNormal = CurHitResult.ImpactNormal;
			StartTrace = CurHitResult.TraceStart;
			EndTrace = CurHitResult.TraceEnd;
		}
	}
	
	// 处理没有命中结果或命中结果实际上没有返回Actor的情况
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!HitActor)
	{
		HitActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
		if (HitActor)
		{
			ImpactLocation = HitActor->GetActorLocation();
		}
	}
	
	// 应用团队伤害/自伤等规则
	// 伤害交互允许乘数：如果团队规则不允许造成伤害，则为0，否则为1
	float DamageInteractionAllowedMultiplier = 1.0f;
	if (HitActor)
	{
		const UYcTeamSubsystem* TeamSubsystem = HitActor->GetWorld()->GetSubsystem<UYcTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			DamageInteractionAllowedMultiplier = TeamSubsystem->CanCauseDamage(EffectCauser, HitActor) ? 1.0 : 0.0;
		}
	}
	
	// 确定攻击发起者和被攻击者的距离
	double Distance = WORLD_MAX;
	
	if (TypedContext->HasOrigin())
	{
		Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
	}
	else if (EffectCauser)
	{
		Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
	}
	else
	{
		ensureMsgf(false, TEXT("Damage Calculation cannot deduce a source location for damage coming from %s; Falling back to WORLD_MAX dist!"), *GetPathNameSafe(Spec.Def));
	}
	
	// 应用技能源修饰符（距离衰减和物理材质衰减）
	float PhysicalMaterialAttenuation = 1.0f;	// 基于物理材质的衰减
	float DistanceAttenuation = 1.0f;			// 基于距离的衰减
	if (const IYcAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
	{
		if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
		{
			PhysicalMaterialAttenuation = AbilitySource->GetPhysicalMaterialAttenuation(PhysMat, SourceTags, TargetTags);
		}
	
		DistanceAttenuation = AbilitySource->GetDistanceAttenuation(Distance, SourceTags, TargetTags);
	}
	DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);
	
	// 计算最终伤害值
	const float DamageDone = FMath::Max(BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * DamageInteractionAllowedMultiplier, 0.0f);
	
	if (DamageDone > 0.0f)
	{
		// 应用修改器, 会把计算出的DamageDone应用到被攻击玩家的UYcShooterHealthSet::Damage上, 然后UYcShooterHealthSet::PostGameplayEffectExecute会扣除Damage值大小的生命值, 以实现对玩家造成伤害
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UYcShooterHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, DamageDone));
	}
#endif // #if WITH_SERVER_CODE
}
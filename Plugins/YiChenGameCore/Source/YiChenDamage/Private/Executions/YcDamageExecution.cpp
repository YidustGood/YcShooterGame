// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Executions/YcDamageExecution.h"

#include "AbilitySystemComponent.h"
#include "YiChenAbility/Public/YcAbilitySourceInterface.h"
#include "YiChenAbility/Public/YcGameplayEffectContext.h"
#include "Debug/YcDamageDebugSettings.h"
#include "Subsystem/YcDamageEventSubsystem.h"
#include "DrawDebugHelpers.h"
#include "YcDamageGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageExecution)

UYcDamageExecution::UYcDamageExecution()
{
}

void UYcDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	// 创建伤害参数
	FYcDamageSummaryParams Params;
	InitializeParams(Params, ExecutionParams);
	Params.ExecParams = &ExecutionParams;
	Params.ExecOutput = &OutExecutionOutput;

	// 获取标签容器
	FGameplayTagContainer SourceTags;
	FGameplayTagContainer TargetTags;
	GetTagContainers(ExecutionParams, SourceTags, TargetTags);

	// 排序组件
	SortComponents();

	// 遍历执行组件
	for (const TObjectPtr<UYcAttributeExecutionComponent>& Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (!Component->ShouldExecute(Params, SourceTags, TargetTags))
		{
			continue;
		}

		Component->Execute(Params);

		// 检查是否取消执行
		if (Params.bCancelExecution)
		{
			break;
		}
	}

	// 通过 ASC 获取 World
	UWorld* World = GetWorldFromParams(Params);

	// 广播伤害事件
	BroadcastDamageEvent(Params, SourceTags);

	// 输出调试信息
	const bool bShouldLog = UYcDamageDebugSubsystem::IsDebugLogEnabled() || bEnableDebugLog || (bAutoEnableDebugInPIE && World && World->IsPlayInEditor());
	if (bShouldLog)
	{
		LogDebugInfo(Params);
	}

	// 伤害可视化
#if !UE_BUILD_SHIPPING
	if (UYcDamageDebugSubsystem::IsVisualizationEnabled() && World)
	{
		DrawDamageVisualization(Params, SourceTags, World);
	}
#endif
#endif
}

void UYcDamageExecution::InitializeParams(FYcAttributeSummaryParams& Params, const FGameplayEffectCustomExecutionParameters& ExecutionParams) const
{
	// 调用基类初始化
	Super::InitializeParams(Params, ExecutionParams);

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 转换为伤害参数
	FYcDamageSummaryParams& DamageParams = static_cast<FYcDamageSummaryParams&>(Params);

	// 从 GE Context 获取伤害类型标签，并从 PhysMaterial 派生 HitZone
	if (FYcGameplayEffectContext* TypedContext = FYcGameplayEffectContext::ExtractEffectContext(Spec.GetContext()))
	{
		DamageParams.DamageTypeTag = TypedContext->GetDamageTypeTag();

		// 从 PhysMaterial 派生 HitZone
		if (const IYcAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
		{
			if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
			{
				DamageParams.HitZone = AbilitySource->GetHitZoneFromPhysicalMaterial(PhysMat);
			}
		}
	}
}

void UYcDamageExecution::LogDebugInfo(const FYcAttributeSummaryParams& Params) const
{
	const FYcDamageSummaryParams& DamageParams = static_cast<const FYcDamageSummaryParams&>(Params);

	UE_LOG(LogTemp, Log, TEXT("=== YcDamageExecution Debug ==="));
	UE_LOG(LogTemp, Log, TEXT("BaseDamage: %.2f"), DamageParams.GetBaseDamage());
	UE_LOG(LogTemp, Log, TEXT("FinalDamage: %.2f"), DamageParams.GetFinalDamage());

	// 显示伤害类型
	if (DamageParams.DamageTypeTag.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("DamageType: %s"), *DamageParams.DamageTypeTag.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("DamageType: None"));
	}

	// 显示命中部位
	if (DamageParams.HitZone.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("HitZone: %s"), *DamageParams.HitZone.ToString());
	}

	// 显示乘区数据
	UE_LOG(LogTemp, Log, TEXT("Override: %s (%.2f)"), DamageParams.bOverride ? TEXT("true") : TEXT("false"), DamageParams.OverrideValue);
	UE_LOG(LogTemp, Log, TEXT("PreMultiplyAdditive: %.2f"), Params.PreMultiplyAdditive);
	UE_LOG(LogTemp, Log, TEXT("Coefficient: %.2f"), Params.Coefficient);
	UE_LOG(LogTemp, Log, TEXT("PostMultiplyAdditive: %.2f"), Params.PostMultiplyAdditive);

	// 显示临时标签
	if (!Params.TemporaryTags.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("TemporaryTags: %s"), *Params.TemporaryTags.ToStringSimple());
	}

	// 显示是否取消
	UE_LOG(LogTemp, Log, TEXT("Canceled: %s"), Params.bCancelExecution ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Log, TEXT("================================="));
}

void UYcDamageExecution::BroadcastDamageEvent(const FYcDamageSummaryParams& Params, const FGameplayTagContainer& SourceTags) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UYcDamageEventSubsystem* EventSubsystem = World->GetSubsystem<UYcDamageEventSubsystem>();
	if (!EventSubsystem)
	{
		return;
	}

	// 获取来源和目标 Actor
	AActor* Instigator = nullptr;
	AActor* Target = nullptr;

	if (Params.SourceASC)
	{
		Instigator = Params.SourceASC->GetOwnerActor();
	}
	if (Params.TargetASC)
	{
		Target = Params.TargetASC->GetOwnerActor();
	}

	// 创建事件数据并广播
	FYcDamageEventData EventData = UYcDamageEventSubsystem::CreateEventDataFromParams(Params, Instigator, Target);
	EventSubsystem->BroadcastDamageEvent(EventData, SourceTags);
}

#if !UE_BUILD_SHIPPING
void UYcDamageExecution::DrawDamageVisualization(const FYcDamageSummaryParams& Params, const FGameplayTagContainer& SourceTags, UWorld* World) const
{
	if (!World)
	{
		return;
	}

	// 获取目标位置
	FVector TargetLocation = FVector::ZeroVector;
	if (Params.TargetASC)
	{
		AActor* TargetActor = Params.TargetASC->GetAvatarActor();

		if (!TargetActor)
		{
			TargetActor = Params.TargetASC->GetOwnerActor();
		}

		if (TargetActor)
		{
			TargetLocation = TargetActor->GetActorLocation();
		}
	}

	if (TargetLocation.IsNearlyZero())
	{
		return;
	}

	// 获取调试设置
	const FYcDamageDebugSettings& DebugSettings = UYcDamageDebugSubsystem::GetSettings();
	const float Duration = DebugSettings.VisualizationDuration;
	const float TextSize = DebugSettings.TextSize;

	// 在目标位置绘制伤害值
	const float FinalDamage = Params.GetFinalDamage();
	FString DamageText = FString::Printf(TEXT("%.1f"), FinalDamage);

	// 根据伤害类型选择颜色
	FColor TextColor = FColor::White;
	if (Params.bCancelExecution)
	{
		TextColor = FColor::Yellow;
		if (Params.TemporaryTags.HasTag(YcDamageGameplayTags::Damage_Event_Dodged))
		{
			DamageText = TEXT("DODGED");
		}
		else if (Params.TemporaryTags.HasTag(YcDamageGameplayTags::Damage_Event_Immunity))
		{
			DamageText = TEXT("IMMUNE");
		}
	}
	else if (Params.TemporaryTags.HasTag(YcDamageGameplayTags::Damage_Event_Critical))
	{
		TextColor = FColor::Red;
		DamageText = FString::Printf(TEXT("CRIT %.1f"), FinalDamage);
	}

	// 添加随机偏移避免文字重叠
	const float RandomOffsetX = FMath::RandRange(-50.f, 50.f);
	const float RandomOffsetY = FMath::RandRange(-50.f, 50.f);
	const FVector TextLocation = TargetLocation + FVector(RandomOffsetX, RandomOffsetY, 100);

	// 绘制文字
	DrawDebugString(World, TextLocation, DamageText, nullptr, TextColor, Duration, true, TextSize);

	// 显示伤害类型和计算过程（可选）
	if (DebugSettings.bShowCalculationProcess)
	{
		FString ProcessText;

		if (Params.DamageTypeTag.IsValid())
		{
			ProcessText = FString::Printf(TEXT("Type: %s\nBase: %.1f\nPreAdd: %.1f\nCoef: %.2f\nPostAdd: %.1f"),
				*Params.DamageTypeTag.GetTagName().ToString(),
				Params.BaseValue, Params.PreMultiplyAdditive, Params.Coefficient, Params.PostMultiplyAdditive);
		}
		else
		{
			ProcessText = FString::Printf(TEXT("Type: None\nBase: %.1f\nPreAdd: %.1f\nCoef: %.2f\nPostAdd: %.1f"),
				Params.BaseValue, Params.PreMultiplyAdditive, Params.Coefficient, Params.PostMultiplyAdditive);
		}

		DrawDebugString(World, TextLocation + FVector(0, 0, 50), ProcessText, nullptr, FColor::Cyan, Duration, true, TextSize * 0.7f);
	}

	// 显示组件执行顺序（可选）
	if (DebugSettings.bShowComponentOrder)
	{
		FString ComponentText = TEXT("Components:\n");
		for (const TObjectPtr<UYcAttributeExecutionComponent>& Component : Components)
		{
			if (Component)
			{
				ComponentText += FString::Printf(TEXT("[%d] %s\n"),
					Component->Priority,
					Component->DebugName.IsEmpty() ? *Component->GetName() : *Component->DebugName);
			}
		}
		DrawDebugString(World, TextLocation + FVector(0, 0, 100), ComponentText, nullptr, FColor::Green, Duration, true, TextSize * 0.6f);
	}
}
#endif

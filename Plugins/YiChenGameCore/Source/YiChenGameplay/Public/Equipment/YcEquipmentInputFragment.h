// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fragments/YcEquipmentFragment.h"
#include "GameFeatures/Actions/YcGameFeatureAction_AddInputContextMapping.h"
#include "YcEquipmentInputFragment.generated.h"

class UInputMappingContext;
class UYcInputConfig;

/**
 * 装备输入扩展片段
 *
 * 用于在装备进入 Equipped 状态时为本地玩家动态附加额外的 IMC 和 InputConfig，
 * 在卸下时自动移除，保持输入生命周期与装备生命周期一致。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEPLAY_API FEquipmentFragment_InputBindings : public FYcEquipmentFragment
{
	GENERATED_BODY()

	/** 装备时要附加的输入映射上下文 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TArray<FYcInputMappingContextAndPriority> InputMappings;

	/** 装备时要附加的输入配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TArray<TSoftObjectPtr<const UYcInputConfig>> InputConfigs;

	virtual void OnEquipped(UYcEquipmentInstance* Instance) const override;
	virtual void OnUnequipped(UYcEquipmentInstance* Instance) const override;

	virtual FString GetDebugString() const override
	{
		return FString::Printf(TEXT("FEquipmentFragment_InputBindings(IMC=%d, Config=%d)"), InputMappings.Num(), InputConfigs.Num());
	}
};

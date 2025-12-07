// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "YcPawnData.generated.h"

class UYcAbilitySet;
class UYcInputConfig;
class UYcAbilityTagRelationshipMapping;

/**
 * Pawn配置数据资源
 * 
 * 运行时不可变的数据资产，包含定义Pawn所需的配置属性。
 * 相比于GameMode中直接指定PawnClass，提供了更强大的配置能力，包括：
 *   - Pawn类型定义
 *   - 技能集配置
 *   - 技能标签关系映射
 *   - 输入配置
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "YcPawnData", ShortTooltip = "Data asset used to define a Pawn."))
class YICHENGAMEPLAY_API UYcPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYcPawnData(const FObjectInitializer& ObjectInitializer);
	
#if WITH_EDITORONLY_DATA
	/** 编辑器中的Pawn配置描述，便于开发阶段标注 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Develop")
	FString DevDescription;
#endif
	
	/**
	 * 要实例化的Pawn类型
	 * 通常应该从AYcPawn/AYcCharacter派生, 以更好的和插件框架一起工作
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Pawn")
	TSubclassOf<APawn> PawnClass;
	
	/**
	 * 授予此Pawn的技能集列表
	 * Pawn实例化后将自动获得这些技能集中的所有技能
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Abilities")
	TArray<TObjectPtr<UYcAbilitySet>> AbilitySets;
	
	/**
	 * 技能标签关系映射
	 * 定义此Pawn的技能标签之间的阻止、取消和激活依赖关系
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Abilities")
	TObjectPtr<UYcAbilityTagRelationshipMapping> TagRelationshipMapping;

	/**
	 * 输入配置
	 * 玩家控制的Pawn使用此配置来创建输入映射和绑定输入动作
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Input")
	TObjectPtr<UYcInputConfig> InputConfig;
};

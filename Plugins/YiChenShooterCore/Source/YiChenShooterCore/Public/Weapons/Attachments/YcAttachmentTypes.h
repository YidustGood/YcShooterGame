// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcAttachmentTypes.generated.h"

class UYcAttachmentDefinition;

/**
 * EYcStatModifierOp - 属性修改操作类型
 */
UENUM(BlueprintType)
enum class EYcStatModifierOp : uint8
{
	/** 加法：FinalValue = BaseValue + ModValue */
	Add         UMETA(DisplayName="加法"),
	/** 乘法：FinalValue = BaseValue * (1 + ModValue)，例如0.1表示增加10% */
	Multiply    UMETA(DisplayName="乘法"),
	/** 覆盖：FinalValue = ModValue */
	Override    UMETA(DisplayName="覆盖"),
};

/**
 * FYcStatModifier - 单个属性修改器
 * 
 * 用于配件对武器属性的修改
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcStatModifier
{
	GENERATED_BODY()

	FYcStatModifier()
		: Operation(EYcStatModifierOp::Add)
		, Value(0.0f)
		, Priority(0)
	{
	}

	FYcStatModifier(FGameplayTag InStatTag, EYcStatModifierOp InOp, float InValue, int32 InPriority = 0)
		: StatTag(InStatTag)
		, Operation(InOp)
		, Value(InValue)
		, Priority(InPriority)
	{
	}

	/** 要修改的属性Tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifier")
	FGameplayTag StatTag;

	/** 修改操作类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifier")
	EYcStatModifierOp Operation;

	/** 修改值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifier")
	float Value;

	/** 修改器优先级（同一属性多个修改器时的应用顺序，数值越小越先应用） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Modifier", AdvancedDisplay)
	int32 Priority;
};


/**
 * FYcAttachmentTuningParam - 配件调校参数定义
 * 
 * 定义配件可调校的参数范围（如COD的Gunsmith Tuning）
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentTuningParam
{
	GENERATED_BODY()

	FYcAttachmentTuningParam()
		: MinValue(-1.0f)
		, MaxValue(1.0f)
		, DefaultValue(0.0f)
		, Operation(EYcStatModifierOp::Add)
	{
	}

	/** 调校参数显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tuning")
	FText DisplayName;

	/** 调校影响的属性Tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tuning")
	FGameplayTag StatTag;

	/** 调校范围最小值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tuning")
	float MinValue;

	/** 调校范围最大值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tuning")
	float MaxValue;

	/** 默认调校值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tuning")
	float DefaultValue;

	/** 调校操作类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tuning")
	EYcStatModifierOp Operation;
};


/**
 * FYcAttachmentSlotDef - 配件槽位定义
 * 
 * 定义武器上的一个配件槽位
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentSlotDef
{
	GENERATED_BODY()

	FYcAttachmentSlotDef()
		: SortOrder(0)
		, bIsDynamic(false)
	{
	}

	/** 槽位类型Tag (e.g. Attachment.Slot.Optic) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	FGameplayTag SlotType;

	/** 槽位显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	FText DisplayName;

	/** 该槽位可安装的配件列表 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	TArray<TSoftObjectPtr<UYcAttachmentDefinition>> AvailableAttachments;

	/** 默认配件（可选，武器出厂自带的配件） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	TSoftObjectPtr<UYcAttachmentDefinition> DefaultAttachment;

	/** 槽位在UI中的排序优先级 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	int32 SortOrder;

	/** 
	 * 是否为动态槽位（由配件提供）
	 * 运行时设置，不需要在编辑器中配置
	 */
	UPROPERTY(BlueprintReadOnly, Category="Slot")
	bool bIsDynamic;

	/**
	 * 提供此槽位的配件所在的槽位
	 * 仅当 bIsDynamic=true 时有效
	 */
	UPROPERTY(BlueprintReadOnly, Category="Slot")
	FGameplayTag ProviderSlotType;
};


/**
 * FYcAttachmentInstance - 配件运行时实例
 * 
 * 存储已安装配件的运行时数据
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentInstance
{
	GENERATED_BODY()

	FYcAttachmentInstance()
	{
	}

	/** 配件定义引用 */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	TSoftObjectPtr<UYcAttachmentDefinition> AttachmentDef;

	/** 安装的槽位类型 */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	FGameplayTag SlotType;

	// @TODO 用TMap的话无法参与网络复制，如果确实需要同步给客户端考虑借助相关的一些复制函数处理
	/** 调校参数值（Key=StatTag, Value=调校值） */
	UPROPERTY(BlueprintReadOnly,NotReplicated, Category="Attachment")
	TMap<FGameplayTag, float> TuningValues;

	/** 生成的配件网格体组件（弱引用） */
	UPROPERTY(BlueprintReadOnly, Category="Attachment")
	TWeakObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 检查是否有效 */
	bool IsValid() const { return !AttachmentDef.IsNull(); }

	/** 重置实例 */
	void Reset()
	{
		AttachmentDef.Reset();
		SlotType = FGameplayTag();
		TuningValues.Empty();
		MeshComponent.Reset();
	}
};

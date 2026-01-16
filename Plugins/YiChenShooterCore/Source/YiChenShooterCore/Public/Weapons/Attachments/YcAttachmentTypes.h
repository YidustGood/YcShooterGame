// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DataRegistryId.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcAttachmentTypes.generated.h"

class UTexture2D;
class UStaticMesh;
class UYcWeaponAttachmentComponent;
struct FYcAttachmentDefinition;
struct FYcAttachmentArray;

/**
 * EYcAttachmentTarget - 配件附加目标类型
 * 
 * 从 YcAttachmentDefinition.h 移动到此处，供新的 FYcAttachmentDefinition 结构体使用
 */
UENUM(BlueprintType)
enum class EYcAttachmentTarget : uint8
{
	/** 附加到武器骨骼网格体的Socket */
	WeaponSocket    UMETA(DisplayName="武器Socket"),
	/** 附加到另一个配件槽位的Mesh组件上 */
	AttachmentSlot  UMETA(DisplayName="其他配件"),
};

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
 * 使用 FDataRegistryId 引用配件定义，支持 DataRegistry 聚合查询
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot", meta = (Categories = "Attachment.Slot"))
	FGameplayTag SlotType;

	/** 槽位显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	FText DisplayName;

	/** 该槽位可安装的配件列表 (DataRegistry ID) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	TArray<FDataRegistryId> AvailableAttachments;

	/** 默认配件 (DataRegistry ID，可选，武器出厂自带的配件) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Slot")
	FDataRegistryId DefaultAttachment;

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
 * 使用 FDataRegistryId 引用配件定义，支持网络复制和存档序列化
 * 
 * 继承 FFastArraySerializerItem 支持 Fast Array 增量复制。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentInstance : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcAttachmentInstance()
		: bIsInstalled(false)
		, CachedDef(nullptr)
	{
	}

	// ═══════════════════════════════════════════════════════════════
	// 复制属性
	// ═══════════════════════════════════════════════════════════════

	/** 配件 DataRegistry ID */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category="Attachment")
	FDataRegistryId AttachmentRegistryId;

	/** 安装的槽位类型 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly,  Category="Attachment")
	FGameplayTag SlotType;

	/** 
	 * 是否已安装配件
	 * 用于区分"空槽位"和"已卸载配件的槽位"
	 * 卸载时设为 false 但保留 AttachmentRegistryId，便于客户端处理卸载逻辑
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category="Attachment")
	bool bIsInstalled;

	// ═══════════════════════════════════════════════════════════════
	// 非复制属性 (本地状态)
	// ═══════════════════════════════════════════════════════════════

	/** 调校参数值（Key=StatTag, Value=调校值）- 不参与网络复制 */
	UPROPERTY(BlueprintReadOnly, NotReplicated, VisibleInstanceOnly, Category="Attachment")
	TMap<FGameplayTag, float> TuningValues;

	/** 生成的配件网格体组件（弱引用）- 不参与网络复制 */
	UPROPERTY(BlueprintReadOnly, NotReplicated, VisibleInstanceOnly, Category="Attachment")
	TWeakObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 缓存的配件定义指针 - 不参与网络复制，运行时缓存 */
	mutable const FYcAttachmentDefinition* CachedDef;

	// ═══════════════════════════════════════════════════════════════
	// 辅助方法
	// ═══════════════════════════════════════════════════════════════

	/** 检查是否有效（已安装配件） */
	bool IsValid() const { return bIsInstalled && AttachmentRegistryId.IsValid(); }

	/** 检查是否有配件数据（可能已卸载但数据未清空） */
	bool HasAttachmentData() const { return AttachmentRegistryId.IsValid(); }

	/** 重置实例（完全清空） */
	void Reset();

	/** 标记为已卸载（保留数据供客户端处理） */
	void MarkUninstalled();

	/** 从 DataRegistry 缓存配件定义 */
	bool CacheDefinitionFromRegistry() const;

	/** 获取配件定义 (自动缓存) */
	const FYcAttachmentDefinition* GetDefinition() const;
	
	/** 获取调试字符串 */
	FString GetDebugString() const;
};


/**
 * FYcAttachmentArray - 配件数组序列化器
 * 
 * 继承 FFastArraySerializer 实现增量网络复制。
 * 提供精确的元素变更回调。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentArray : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcAttachmentArray() : OwnerComponent(nullptr) {}

	/** 配件实例数组 */
	UPROPERTY(VisibleInstanceOnly)
	TArray<FYcAttachmentInstance> Items;

	/** 所属组件引用 (用于回调访问) - 不参与网络复制 */
	UPROPERTY(NotReplicated, VisibleInstanceOnly)
	TObjectPtr<UYcWeaponAttachmentComponent> OwnerComponent;

	// ═══════════════════════════════════════════════════════════════
	// FFastArraySerializer 接口
	// ═══════════════════════════════════════════════════════════════

	/** 元素被移除前回调 (客户端) */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/** 元素被添加后回调 (客户端) */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/** 元素被修改后回调 (客户端) */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	/** 网络增量序列化 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcAttachmentInstance, FYcAttachmentArray>(
			Items, DeltaParms, *this);
	}

	// ═══════════════════════════════════════════════════════════════
	// 辅助方法
	// ═══════════════════════════════════════════════════════════════

	/** 标记指定元素已修改 */
	void MarkItemDirtyByIndex(int32 Index)
	{
		if (Items.IsValidIndex(Index))
		{
			MarkItemDirty(Items[Index]);
		}
	}

	/** 根据槽位查找配件索引 */
	int32 FindIndexBySlot(FGameplayTag SlotType) const
	{
		return Items.IndexOfByPredicate([&SlotType](const FYcAttachmentInstance& Item)
		{
			return Item.SlotType == SlotType;
		});
	}

	/** 根据槽位获取配件 */
	FYcAttachmentInstance* FindBySlot(FGameplayTag SlotType)
	{
		int32 Index = FindIndexBySlot(SlotType);
		return Index != INDEX_NONE ? &Items[Index] : nullptr;
	}

	const FYcAttachmentInstance* FindBySlot(FGameplayTag SlotType) const
	{
		int32 Index = FindIndexBySlot(SlotType);
		return Index != INDEX_NONE ? &Items[Index] : nullptr;
	}
};

/** 启用 NetDeltaSerialize */
template<>
struct TStructOpsTypeTraits<FYcAttachmentArray> : public TStructOpsTypeTraitsBase2<FYcAttachmentArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};


/**
 * FYcSlotTuningData - 单个槽位的调校数据
 * 
 * 存储某个槽位配件的调校参数值
 * 用于 FYcWeaponLoadout 中保存每个槽位的调校配置
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcSlotTuningData
{
	GENERATED_BODY()

	FYcSlotTuningData() = default;

	/** 调校参数值 (Key=StatTag, Value=调校值) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tuning")
	TMap<FGameplayTag, float> TuningValues;

	/** 检查是否为空 */
	bool IsEmpty() const { return TuningValues.Num() == 0; }

	/** 清空调校数据 */
	void Clear() { TuningValues.Empty(); }
};


/**
 * FYcWeaponLoadout - 武器配置/预设
 * 
 * 存储一套完整的武器配件配置，支持保存/加载。
 * 可用于：
 * - 武器预设方案（玩家自定义改枪方案）
 * - 武器蓝图（策划预配置的配件组合）
 * - 存档系统（保存/加载玩家的武器配置）
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcWeaponLoadout
{
	GENERATED_BODY()

	FYcWeaponLoadout() = default;

	/** 预设名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loadout")
	FText LoadoutName;

	/** 槽位到配件的映射 (Key=SlotType, Value=AttachmentRegistryId) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loadout")
	TMap<FGameplayTag, FDataRegistryId> SlotAttachments;

	/** 槽位调校值 (Key=SlotType, Value=该槽位的调校数据) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loadout")
	TMap<FGameplayTag, FYcSlotTuningData> SlotTuningValues;

	/** 检查是否为空（没有任何配件配置） */
	bool IsEmpty() const { return SlotAttachments.Num() == 0; }

	/** 清空配置 */
	void Clear()
	{
		LoadoutName = FText::GetEmpty();
		SlotAttachments.Empty();
		SlotTuningValues.Empty();
	}

	/** 获取指定槽位的配件ID */
	FDataRegistryId GetAttachmentForSlot(FGameplayTag SlotType) const
	{
		if (const FDataRegistryId* Found = SlotAttachments.Find(SlotType))
		{
			return *Found;
		}
		return FDataRegistryId();
	}

	/** 获取指定槽位的调校数据 */
	const FYcSlotTuningData* GetTuningDataForSlot(FGameplayTag SlotType) const
	{
		return SlotTuningValues.Find(SlotType);
	}

	/** 设置槽位配件 */
	void SetSlotAttachment(FGameplayTag SlotType, const FDataRegistryId& AttachmentId)
	{
		if (SlotType.IsValid() && AttachmentId.IsValid())
		{
			SlotAttachments.Add(SlotType, AttachmentId);
		}
	}

	/** 设置槽位调校数据 */
	void SetSlotTuningData(FGameplayTag SlotType, const FYcSlotTuningData& TuningData)
	{
		if (SlotType.IsValid() && !TuningData.IsEmpty())
		{
			SlotTuningValues.Add(SlotType, TuningData);
		}
	}

	/** 移除槽位配件 */
	void RemoveSlotAttachment(FGameplayTag SlotType)
	{
		SlotAttachments.Remove(SlotType);
		SlotTuningValues.Remove(SlotType);
	}
};


/**
 * FYcAttachmentDefinition - 配件定义结构体
 * 
 * 继承自 FTableRowBase，用于 DataTable 配置。
 * 通过 DataRegistry 统一管理和访问。
 * 
 * 注意：配件的唯一标识直接使用 DataTable 的 RowName (FName)，
 * 无需额外的 AttachmentId 字段。通过 FDataRegistryId 访问时，
 * ItemName 就是 RowName。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcAttachmentDefinition : public FTableRowBase
{
	GENERATED_BODY()

	FYcAttachmentDefinition()
		: AttachTarget(EYcAttachmentTarget::WeaponSocket)
		, bSupportsTuning(false)
		, UnlockWeaponLevel(1)
	{
	}

	// ═══════════════════════════════════════════════════════════════
	// 1. 基础信息
	// ═══════════════════════════════════════════════════════════════
	
	/** 配件显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础信息")
	FText DisplayName;
	
	/** 配件描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础信息", meta=(MultiLine=true))
	FText Description;
	
	/** 配件图标 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础信息")
	TSoftObjectPtr<UTexture2D> Icon;
	
	/** 配件所属槽位类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="1.基础信息")
	FGameplayTag SlotType;

	// ═══════════════════════════════════════════════════════════════
	// 2. 视觉配置
	// ═══════════════════════════════════════════════════════════════
	
	/** 配件网格体 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.视觉")
	TSoftObjectPtr<UStaticMesh> AttachmentMesh;
	
	/** 附加目标类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.视觉")
	EYcAttachmentTarget AttachTarget;
	
	/** 目标配件槽位 (AttachmentSlot模式) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.视觉",
		meta=(EditCondition="AttachTarget==EYcAttachmentTarget::AttachmentSlot", EditConditionHides))
	FGameplayTag TargetSlotType;
	
	/** 附加Socket名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.视觉")
	FName AttachSocketName;
	
	/** 相对位置偏移 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.视觉")
	FTransform AttachOffset;
	
	/** 安装后需要隐藏的槽位 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="2.视觉")
	TArray<FGameplayTag> HiddenSlots;

	// ═══════════════════════════════════════════════════════════════
	// 3. 属性修改
	// ═══════════════════════════════════════════════════════════════
	
	/** 属性修改器列表 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="3.属性修改")
	TArray<FYcStatModifier> StatModifiers;

	// ═══════════════════════════════════════════════════════════════
	// 4. 调校参数
	// ═══════════════════════════════════════════════════════════════
	
	/** 是否支持调校 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.调校")
	bool bSupportsTuning;
	
	/** 调校参数定义 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="4.调校",
		meta=(EditCondition="bSupportsTuning"))
	TArray<FYcAttachmentTuningParam> TuningParams;

	// ═══════════════════════════════════════════════════════════════
	// 5. 扩展槽位
	// ═══════════════════════════════════════════════════════════════
	
	/** 安装后提供的额外槽位 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="5.扩展槽位")
	TArray<FYcAttachmentSlotDef> ProvidedSlots;

	// ═══════════════════════════════════════════════════════════════
	// 6. 兼容性
	// ═══════════════════════════════════════════════════════════════
	
	/** 互斥配件Tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6.兼容性")
	FGameplayTagContainer IncompatibleAttachments;
	
	/** 前置配件Tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="6.兼容性")
	FGameplayTagContainer RequiredAttachments;

	// ═══════════════════════════════════════════════════════════════
	// 7. 解锁条件
	// ═══════════════════════════════════════════════════════════════
	
	/** 解锁所需武器等级 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="7.解锁")
	int32 UnlockWeaponLevel;
	
	/** 解锁挑战Tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="7.解锁")
	FGameplayTag UnlockChallengeTag;

	// ═══════════════════════════════════════════════════════════════
	// 辅助方法
	// ═══════════════════════════════════════════════════════════════
	
	/** 检查是否提供指定槽位 */
	bool HasProvidedSlot(FGameplayTag InSlotType) const;
	
	/** 获取提供的所有槽位类型 */
	TArray<FGameplayTag> GetProvidedSlotTypes() const;
	
	/** 获取指定属性的修改器 */
	TArray<FYcStatModifier> GetModifiersForStat(FGameplayTag StatTag) const;
	
	/** 检查是否与指定配件互斥 */
	bool IsIncompatibleWith(FGameplayTag OtherAttachmentId) const;
	
	/** 检查是否满足前置配件要求 */
	bool HasRequiredAttachments(const TArray<FGameplayTag>& InstalledAttachmentIds) const;
};

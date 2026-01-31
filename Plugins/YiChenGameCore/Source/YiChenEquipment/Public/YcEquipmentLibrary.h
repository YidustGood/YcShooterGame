// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fragments/YcEquipmentFragment.h"
#include "YcEquipmentLibrary.generated.h"

struct FInventoryFragment_Equippable;
class UYcEquipmentInstance;

UENUM()
enum class EYcEquipmentFragmentResult : uint8
{
	Valid,
	NotValid,
};

/**
 * 装备系统蓝图函数库，提供便利的 Fragment 查询函数
 * 避免蓝图开发者需要调用 GetInstancedStructValue 进行额外的类型转换
 */
UCLASS()
class YICHENEQUIPMENT_API UYcEquipmentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static UYcEquipmentInstance* GetCurrentEquipmentInst(AActor* OwnerActor);
	
	
	/* 从FYcInventoryItemDefinition->Fragments中获取特定类型的Fragment */
	UFUNCTION(BlueprintCallable)
	static TInstancedStruct<FYcEquipmentFragment> FindEquipmentFragment(const FYcEquipmentDefinition& EquipmentDef, const UScriptStruct* FragmentStructType);

	/**
	 * 从装备实例获取指定类型的 Fragment（高效方式 - CustomThunk）
	 * 
	 * 使用 CustomThunk 实现，支持蓝图中直接 Break 为任意 Fragment 类型
	 * 相比 FindEquipmentFragment + GetInstancedStructValue，性能更优（减少 1 次拷贝）
	 * 
	 * @param ExecResult 执行结果（Valid/NotValid）
	 * @param Equipment 装备实例
	 * @param FragmentStructType 目标 Fragment 结构类型（必须是 FYcEquipmentFragment 或其子类）
	 * @param OutFragment 输出的 Fragment 数据（通配符，可 Break 成任意类型）
	 * 
	 * 使用示例（蓝图）：
	 * Equipment → Get Equipment Fragment (Custom Thunk)
	 *   ├─ Fragment Struct Type: FEquipmentFragment_WeaponStats
	 *   └─ 返回 Valid/NotValid
	 *     ├─ Valid → Break OutFragment 为 FEquipmentFragment_WeaponStats
	 *     └─ NotValid → Fragment 不存在
	 * 
	 * 性能优势：
	 * - 只产生 1 次拷贝（直接从 FInstancedStruct 复制到输出参数）
	 * - 无需额外的 GetInstancedStructValue 调用
	 * - 蓝图中可直接 Break 结构体，无需类型转换
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Equipment|Fragment", 
		meta = (CustomStructureParam = "OutFragment", ExpandEnumAsExecs = "ExecResult", BlueprintInternalUseOnly = "true"))
	static void GetEquipmentFragment(EYcEquipmentFragmentResult& ExecResult, UPARAM(Ref) const UYcEquipmentInstance* Equipment, 
		const UScriptStruct* FragmentStructType, int32& OutFragment);

private:
	DECLARE_FUNCTION(execGetEquipmentFragment);
};

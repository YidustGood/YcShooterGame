// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once


#include "YcEquipmentDefinition.h"
#include "Engine/StreamableManager.h"
#include "YcEquipmentFragment.generated.h"

class UYcEquipmentInstance;
class UYcEquipmentVisualData;

/**
 * 快捷装备栏槽位 Fragment
 * 用于指定装备所能放置的QuickBar Slot Index
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FEquipmentFragment_QuickBarSlot : public FYcEquipmentFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 SlotIndex;
};


/**
 * 装备视觉资产 Fragment
 * 
 * 通过 FPrimaryAssetId 引用 UYcEquipmentVisualData 或其派生类，实现：
 * - 数据与资源分离：Fragment 只存 ID，不直接引用资源
 * - 异步加载：通过 AssetManager 统一加载/卸载
 * - 按需加载：使用 AssetBundles 分组加载
 * 
 * 使用流程：
 * 1. 创建 UYcEquipmentVisualData 或派生类的蓝图资产
 * 2. 在此 Fragment 中配置 VisualDataId
 * 3. 运行时调用 LoadVisualDataAsync() 异步加载
 * 4. 加载完成后通过 GetLoadedVisualData() 获取资源
 * 
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FEquipmentFragment_VisualAsset : public FYcEquipmentFragment
{
	GENERATED_BODY()

	/**
	 * 视觉数据的 PrimaryAssetId
	 * 格式: ClassName:AssetName (如 UYcWeaponVisualData:DA_Weapon_AK47)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FPrimaryAssetId VisualDataId;

	/**
	 * 资源束名称，与 UYcEquipmentVisualData 中的 AssetBundles 对应
	 * 默认为 "EquipmentVisual"，所有派生类统一使用此名称
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<FName> AssetBundleNames = { FName(TEXT("EquipmentVisual")) };

public:
	/**
	 * 异步加载视觉数据
	 * @param OnLoadCompleted 加载完成回调
	 * @return 加载句柄，可用于监控进度或取消加载
	 */
	TSharedPtr<struct FStreamableHandle> LoadVisualDataAsync(FStreamableDelegate OnLoadCompleted = FStreamableDelegate()) const;
	
	/**
	 * 卸载视觉数据
	 */
	void UnloadVisualData() const;
	
	/**
	 * 获取已加载的视觉数据（基类指针）
	 * @return 已加载的数据指针，未加载时返回 nullptr
	 */
	UYcEquipmentVisualData* GetLoadedVisualData() const;

	/**
	 * 获取已加载的视觉数据（模板版本，自动转换类型）
	 */
	template<typename T>
	T* GetLoadedVisualDataAs() const
	{
		return Cast<T>(GetLoadedVisualData());
	}
	
	/**
	 * 检查数据是否已加载
	 */
	bool IsVisualDataLoaded() const;

	virtual FString GetDebugString() const override
	{
		return FString::Printf(TEXT("VisualAsset(Id=%s, Loaded=%s)"), 
			*VisualDataId.ToString(), IsVisualDataLoaded() ? TEXT("Yes") : TEXT("No"));
	}
	
	virtual void OnEquipmentInstanceCreated(UYcEquipmentInstance* Instance) const override;
};

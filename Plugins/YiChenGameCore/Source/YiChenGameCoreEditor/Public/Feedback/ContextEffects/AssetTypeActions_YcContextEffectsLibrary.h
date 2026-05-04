// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "AssetTypeCategories.h"

class UClass;

/** YcContextEffectsLibrary 资源类型 Action，注册后可在编辑器内容浏览器中创建该资产。 */
class FAssetTypeActions_YcContextEffectsLibrary : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_YcContextEffectsLibrary", "Yc Context Effects Library"); }
	virtual FColor GetTypeColor() const override { return FColor(65, 200, 98); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Gameplay; }
};

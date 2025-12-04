// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

YICHENEQUIPMENT_API DECLARE_LOG_CATEGORY_EXTERN(LogYcEquipment, Log, All);

class FYiChenEquipmentModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

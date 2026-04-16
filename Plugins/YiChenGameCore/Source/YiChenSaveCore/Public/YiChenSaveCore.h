// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** SaveCore 模块日志。 */
DECLARE_LOG_CATEGORY_EXTERN(LogYcSaveCore, Log, All);

/** SaveCore 模块生命周期入口。 */
class FYiChenSaveCoreModule : public IModuleInterface
{
public:
    /** 模块启动：当前仅保留扩展点。 */
    virtual void StartupModule() override;
    /** 模块关闭：当前仅保留扩展点。 */
    virtual void ShutdownModule() override;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * YiChenAccountCore 模块日志。
 * 主要用于输出账号登录、会话恢复、角色切换等账号链路上的运行信息。
 */
DECLARE_LOG_CATEGORY_EXTERN(LogYcAccountCore, Log, All);

/**
 * YiChenAccountCore 模块入口。
 * 当前模块负责提供账号系统的公共类型、会话子系统以及适配器扩展点，
 * 供项目和其他复用插件统一接入玩家身份与角色档案能力。
 */
class FYiChenAccountCoreModule : public IModuleInterface
{
public:
    /** 模块启动时调用，可在此注册账号相关全局能力。 */
    virtual void StartupModule() override;

    /** 模块卸载时调用，用于释放账号系统注册的全局资源。 */
    virtual void ShutdownModule() override;
};

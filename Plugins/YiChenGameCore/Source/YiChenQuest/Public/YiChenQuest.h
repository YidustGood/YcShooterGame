// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

/**
 * YiChenQuest 模块日志。
 * 主要用于输出任务接取、状态切换、事件推进、资源加载与持久化恢复等链路信息。
 */
YICHENQUEST_API DECLARE_LOG_CATEGORY_EXTERN(LogYcQuest, Log, All);

/**
 * YiChenQuest 模块入口。
 * 当前模块提供任务定义、任务实例、任务目标树、事件路由、共享成员解析与任务存档域注册能力，
 * 是 YiChenGameCore 中负责任务玩法框架的核心模块。
 */
class FYiChenQuestModule : public IModuleInterface
{
public:
    /** 模块启动时注册任务模块依赖的全局服务，例如任务存档域。 */
    virtual void StartupModule() override;

    /** 模块卸载时注销任务模块注册的全局服务。 */
    virtual void ShutdownModule() override;
};

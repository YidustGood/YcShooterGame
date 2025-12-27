// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

YICHENGAMECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogYcGameCore, Log, All);

/**
 * AngelScript支持控制宏
 * 
 * 用于控制插件是否支持AngelScript版本的虚幻引擎
 * 此宏定义在Core模块中，可供所有依赖Core模块的功能模块使用
 * 
 * 使用方式：
 *   1. 手动控制：取消注释下面的宏定义来手动启用/禁用支持
 * 
 * 注意：如果使用AngelScript版本的虚幻引擎，建议启用此宏以确保脚本类能正确工作
 * 
 * 在其他模块中使用：
 *   #include "YiChenGameCore.h"  // 包含Core模块头文件即可使用 YICHEN_GAMECORE_SUPPORT_ANGELSCRIPT 宏
 */
#define YICHEN_GAMECORE_SUPPORT_ANGELSCRIPT 1  // 启用AngelScript支持
// #define YICHEN_GAMECORE_SUPPORT_ANGELSCRIPT 0  // 禁用AngelScript支持

class FYiChenGameCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

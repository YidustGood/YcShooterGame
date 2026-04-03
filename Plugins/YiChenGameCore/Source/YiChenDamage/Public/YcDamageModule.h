// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * YiChenDamage 模块接口
 * 提供通用伤害计算功能
 */
class FYcDamageModule : public IModuleInterface
{
public:
    /** 获取模块名称 */
    static constexpr const char* GetModuleName()
    {
        return "YiChenDamage";
    }

    /** 获取模块实例 */
    static FYcDamageModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FYcDamageModule>(GetModuleName());
    }

    /** 检查模块是否已加载 */
    static bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded(GetModuleName());
    }

public:
    //~ IModuleInterface interface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~ IModuleInterface interface
};

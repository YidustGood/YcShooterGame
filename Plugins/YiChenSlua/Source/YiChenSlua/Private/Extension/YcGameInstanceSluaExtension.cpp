// YcGameInstanceSluaExtension.cpp
// 为 UGameInstance 添加 Slua 扩展方法

#include "LuaObject.h"
#include "LuaCppBinding.h"
#include "Engine/GameInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"

namespace NS_SLUA
{
    namespace GameInstanceExtension
    {
        // GetSubsystem - 获取 GameInstance 子系统
        // 参数: SubsystemClass - 子系统类的 UClass
        // 返回: 子系统实例或 nil
        int GetSubsystem(lua_State* L)
        {
            CheckUD(UGameInstance, L, 1);
            if (!UD)
            {
                luaL_error(L, "arg 1 expect UGameInstance");
                return 0;
            }

            // 获取子系统类
            UClass* SubsystemClass = LuaObject::checkUD<UClass>(L, 2);
            if (!SubsystemClass)
            {
                luaL_error(L, "arg 2 expect UClass");
                return 0;
            }

            // 查找子系统
            UGameInstanceSubsystem* Subsystem = UD->GetSubsystemBase(SubsystemClass);
            if (Subsystem)
            {
                return LuaObject::push(L, Subsystem);
            }
            
            return 0; // 返回 nil
        }

        // GetSubsystemByType - 通过类型名称获取子系统
        // 参数: TypeName - 子系统类型名称字符串
        // 返回: 子系统实例或 nil
        int GetSubsystemByType(lua_State* L)
        {
            CheckUD(UGameInstance, L, 1);
            if (!UD)
            {
                luaL_error(L, "arg 1 expect UGameInstance");
                return 0;
            }

            const char* TypeName = luaL_checkstring(L, 2);
            if (!TypeName)
            {
                return 0;
            }

            // 查找子系统类
            FString ClassName = UTF8_TO_TCHAR(TypeName);
            UClass* SubsystemClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
            
            if (!SubsystemClass)
            {
                // 尝试添加 U 前缀
                FString PrefixedName = FString::Printf(TEXT("U%s"), *ClassName);
                SubsystemClass = FindObject<UClass>(ANY_PACKAGE, *PrefixedName);
            }

            if (SubsystemClass)
            {
                UGameInstanceSubsystem* Subsystem = UD->GetSubsystemBase(SubsystemClass);
                if (Subsystem)
                {
                    return LuaObject::push(L, Subsystem);
                }
            }
            
            return 0; // 返回 nil
        }

        void init()
        {
            // 注册扩展方法到 UGameInstance
            REG_EXTENSION_METHOD_IMP(UGameInstance, "GetSubsystem", {
                return GetSubsystem(L);
            });

            REG_EXTENSION_METHOD_IMP(UGameInstance, "GetSubsystemByType", {
                return GetSubsystemByType(L);
            });
        }
    }
}

// 自动初始化注册器
namespace
{
    struct FGameInstanceExtensionRegistrar
    {
        FGameInstanceExtensionRegistrar()
        {
            NS_SLUA::GameInstanceExtension::init();
        }
    };

    static FGameInstanceExtensionRegistrar GGameInstanceExtensionRegistrar;
}

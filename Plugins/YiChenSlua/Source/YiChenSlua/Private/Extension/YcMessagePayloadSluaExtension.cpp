// YcMessagePayloadSluaExtension.cpp
// 为 Slua 注册 GameplayMessage Payload 获取扩展方法

#include "LuaObject.h"
#include "LuaCppBinding.h"
#include "GameFramework/AsyncAction_ListenForGameplayMessage.h"
#include "YcMessagePayloadLibrary.h"

namespace NS_SLUA
{
    namespace MessagePayloadExtension
    {
        void init()
        {
            // 注册 GetPayloadAsLuaStruct 方法到 UAsyncAction_ListenForGameplayMessage
            REG_EXTENSION_METHOD_IMP(UAsyncAction_ListenForGameplayMessage, "GetPayloadAsLuaStruct", {
                CheckUD(UAsyncAction_ListenForGameplayMessage, L, 1);
                if (!UD)
                {
                    luaL_error(L, "arg 1 expect UAsyncAction_ListenForGameplayMessage");
                    return 0;
                }

                // 获取 Payload 作为 LuaStruct
                LuaStruct* LuaStructInstance = UYcMessagePayloadLibrary::GetPayloadAsLuaStruct(UD);
                if (LuaStructInstance)
                {
                    return LuaObject::push(L, LuaStructInstance);
                }
                
                // 没有数据，返回 nil
                return 0;
            });
            //
            // // 注册 HasPayload 方法
            // REG_EXTENSION_METHOD_IMP(UAsyncAction_ListenForGameplayMessage, "HasPayload", {
            //     CheckUD(UAsyncAction_ListenForGameplayMessage, L, 1);
            //     if (!UD)
            //     {
            //         luaL_error(L, "arg 1 expect UAsyncAction_ListenForGameplayMessage");
            //         return 0;
            //     }
            //     return LuaObject::push(L, UD->HasPayload());
            // });
            //
            // // 注册 GetPayloadStructType 方法
            // REG_EXTENSION_METHOD_IMP(UAsyncAction_ListenForGameplayMessage, "GetPayloadStructType", {
            //     CheckUD(UAsyncAction_ListenForGameplayMessage, L, 1);
            //     if (!UD)
            //     {
            //         luaL_error(L, "arg 1 expect UAsyncAction_ListenForGameplayMessage");
            //         return 0;
            //     }
            //     return LuaObject::push(L, UD->GetPayloadStructType());
            // });
        }
    }
}

// 自动初始化注册器
namespace
{
    struct FMessagePayloadExtensionRegistrar
    {
        FMessagePayloadExtensionRegistrar()
        {
            // 在模块启动时注册扩展方法
            NS_SLUA::MessagePayloadExtension::init();
        }
    };

    static FMessagePayloadExtensionRegistrar GMessagePayloadExtensionRegistrar;
}

// YcGameplayMessageSubsystemSluaExtension.cpp
// 为 UGameplayMessageSubsystem 添加 Slua 扩展方法
// 注意：FGameplayMessageListenerHandle 是 USTRUCT，没有 StaticClass()，使用全局函数方式

#include "LuaObject.h"
#include "LuaCppBinding.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h"
#include "LuaState.h"
#include "LuaVar.h"
#include "Logging/LogMacros.h"

// 定义日志类别
DEFINE_LOG_CATEGORY_STATIC(LogYcGameplayMessageSlua, Log, All);

namespace NS_SLUA
{
    // 注册类型名称
    DefTypeName(FGameplayTag);
    DefTypeName(FGameplayMessageListenerHandle);

    // Lua 回调包装器，用于保存 Lua 函数引用
    // 使用 LuaVar 代替裸 lua_State* + luaL_ref，Slua 会自动管理生命周期
    class FLuaMessageCallback
    {
    public:
        FLuaMessageCallback(lua_State* L, int LuaFuncRef, UScriptStruct* ExpectedStructType)
            : LuaCallback(L, LuaFuncRef, LuaVar::LV_FUNCTION)
            , StructType(ExpectedStructType)
        {
        }

        ~FLuaMessageCallback()
        {
            // LuaCallback(LuaVar) 会自动清理，无需手动 luaL_unref
        }

        void Execute(FGameplayTag Channel, const UScriptStruct* SenderStructType, const void* Payload) const
        {
            // LuaVar::isValid() 会检查 Lua 状态是否有效
            if (!LuaCallback.isValid())
            {
                return;
            }

            lua_State* L = LuaCallback.getState();
            if (!L)
            {
                return;
            }

            // 使用 LuaVar::push 压入函数
            LuaCallback.push(L);

            // 压入 Channel 参数
            LuaObject::push(L, Channel);

            // 压入 Payload 作为 LuaStruct
            if (Payload && SenderStructType)
            {
                // 创建 LuaStruct 并复制数据
                const uint32 Size = SenderStructType->GetStructureSize();
                uint8* Buf = static_cast<uint8*>(FMemory::Malloc(Size));
                SenderStructType->InitializeStruct(Buf);
                SenderStructType->CopyScriptStruct(Buf, Payload);

                LuaStruct* LuaStructObj = new LuaStruct();
                LuaStructObj->Init(Buf, Size, const_cast<UScriptStruct*>(SenderStructType), false);
                LuaObject::push(L, LuaStructObj);
                LuaObject::addLink(L, Buf);
            }
            else
            {
                lua_pushnil(L);
            }

            // 调用 Lua 函数 (2 个参数, 0 个返回值)
            if (lua_pcall(L, 2, 0, 0) != 0)
            {
                const char* Error = lua_tostring(L, -1);
                UE_LOG(LogGameplayMessageSubsystem, Warning, TEXT("Lua message callback error: %s"), UTF8_TO_TCHAR(Error));
                lua_pop(L, 1);
            }
        }

    private:
        LuaVar LuaCallback;  // 使用 LuaVar 安全持有 Lua 函数引用
        UScriptStruct* StructType;
    };

    // 全局映射：Handle ID -> Lua 回调
    static TMap<int32, TSharedPtr<FLuaMessageCallback>> GLuaMessageCallbacks;

    //=====================================================================
    // FGameplayMessageListenerHandle 全局函数（结构体没有 StaticClass）
    //=====================================================================
    namespace GameplayMessageListenerHandleExtension
    {
        // IsValid - 检查句柄是否有效
        int IsValid(lua_State* L)
        {
            const FGameplayMessageListenerHandle* Handle = LuaObject::checkValue<FGameplayMessageListenerHandle*>(L, 1);
            return LuaObject::push(L, Handle ? Handle->IsValid() : false);
        }

        // Unregister - 取消监听
        int Unregister(lua_State* L)
        {
            FGameplayMessageListenerHandle* Handle = LuaObject::checkValue<FGameplayMessageListenerHandle*>(L, 1);
            if (Handle && Handle->IsValid())
            {
                // 清理 Lua 回调
                GLuaMessageCallbacks.Remove(Handle->GetID());
                Handle->Unregister();
            }
            return 0;
        }

        // GetID - 获取句柄ID
        int GetID(lua_State* L)
        {
            const FGameplayMessageListenerHandle* Handle = LuaObject::checkValue<FGameplayMessageListenerHandle*>(L, 1);
            return LuaObject::push(L, Handle ? Handle->GetID() : 0);
        }

        // 方法注册表
        static luaL_Reg Methods[] = {
            {"IsValid", IsValid},
            {"Unregister", Unregister},
            {"GetID", GetID},
            {nullptr, nullptr}
        };

        void init(lua_State* L)
        {
            // 清理旧的 Lua 回调映射（PIE 结束后 Lua 状态被销毁，但全局 Map 仍持有旧回调）
            GLuaMessageCallbacks.Empty();

            // 创建 ListenerHandle 命名空间表
            lua_newtable(L);
            for (const luaL_Reg* Reg = Methods; Reg->name; ++Reg)
            {
                lua_pushstring(L, Reg->name);
                lua_pushcfunction(L, Reg->func);
                lua_rawset(L, -3);
            }
            lua_setglobal(L, "GameplayMessageListenerHandle");
        }
    }

    namespace GameplayMessageSubsystemExtension
    {
        // BroadcastMessage - 广播消息
        // 隐藏参数p=1为UGameplayMessageSubsystem对象
        // 参数1(p=2): Channel - FGameplayTag 频道
        // 参数2(P=3): Message - LuaStruct 消息数据
        // 返回: bool 是否成功，失败返回 nil
        int BroadcastMessage(lua_State* L)
        {
            CheckUD(UGameplayMessageSubsystem, L, 1);
            if (!UD)
            {
                UE_LOG(LogYcGameplayMessageSlua, Error, TEXT("BroadcastMessage: 参数1需要 UGameplayMessageSubsystem"));
                return 0; // 返回 nil
            }

            // 获取 Channel
            FGameplayTag* Channel = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Channel || !Channel->IsValid())
            {
                UE_LOG(LogYcGameplayMessageSlua, Error, TEXT("BroadcastMessage: 参数2需要有效的 FGameplayTag，当前传入的 Tag 无效"));
                return 0; // 返回 nil
            }

            // 获取 LuaStruct
            LuaStruct* MsgStruct = LuaObject::checkUD<LuaStruct>(L, 3);
            if (!MsgStruct || !MsgStruct->buf || MsgStruct->uss == nullptr)
            {
                luaL_error(L, "arg 3 expect valid LuaStruct");
                return 0;
            }

            // 调用内部广播方法
            UD->BroadcastMessageInternal(*Channel, MsgStruct->uss, MsgStruct->buf);
            
            return LuaObject::push(L, true);
        }

        // BroadcastMessageByType - 通过类型名称和表数据广播消息
        // 参数1: Channel - FGameplayTag 频道
        // 参数2: StructTypeName - 结构体类型名称
        // 参数3: MessageTable - 包含消息数据的表
        // 返回: bool 是否成功，失败返回 nil
        int BroadcastMessageByType(lua_State* L)
        {
            CheckUD(UGameplayMessageSubsystem, L, 1);
            if (!UD)
            {
                UE_LOG(LogYcGameplayMessageSlua, Error, TEXT("BroadcastMessageByType: 参数1需要 UGameplayMessageSubsystem"));
                return 0; // 返回 nil
            }

            // 获取 Channel
            FGameplayTag* Channel = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Channel || !Channel->IsValid())
            {
                UE_LOG(LogYcGameplayMessageSlua, Error, TEXT("BroadcastMessageByType: 参数2需要有效的 FGameplayTag，当前传入的 Tag 无效"));
                return 0; // 返回 nil
            }

            // 获取结构体类型名称
            const char* TypeName = luaL_checkstring(L, 3);
            if (!TypeName)
            {
                luaL_error(L, "arg 3 expect struct type name string");
                return 0;
            }

            // 查找 UScriptStruct
            FString StructName = UTF8_TO_TCHAR(TypeName);
            UScriptStruct* ScriptStruct = LoadObject<UScriptStruct>(nullptr, *StructName);
            
            if (!ScriptStruct)
            {
                // 尝试添加 F 前缀
                FString PrefixedName = FString::Printf(TEXT("F%s"), *StructName);
                ScriptStruct = LoadObject<UScriptStruct>(nullptr, *PrefixedName);
            }

            if (!ScriptStruct)
            {
                // 尝试完整路径
                FString FullName = FString::Printf(TEXT("/Script/GameplayMessageRouter.%s"), *StructName);
                ScriptStruct = LoadObject<UScriptStruct>(nullptr, *FullName);
            }

            if (!ScriptStruct)
            {
                luaL_error(L, "Failed to find UScriptStruct: %s", TypeName);
                return 0;
            }

            // 检查第4个参数是否是表
            if (!lua_istable(L, 4))
            {
                luaL_error(L, "arg 4 expect table with message data");
                return 0;
            }

            // 创建结构体实例
            void* StructData = FMemory::Malloc(ScriptStruct->GetStructureSize());
            ScriptStruct->InitializeStruct(StructData);

            // 从表填充结构体数据
            lua_pushvalue(L, 4); // 复制表到栈顶
            lua_pushnil(L); // 第一个 key
            
            while (lua_next(L, -2) != 0)
            {
                // key 在 -2, value 在 -1
                const char* Key = luaL_checkstring(L, -2);
                
                // 查找属性
                FProperty* Prop = ScriptStruct->FindPropertyByName(FName(UTF8_TO_TCHAR(Key)));
                if (Prop)
                {
                    // 根据属性类型设置值
                    if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
                    {
                        const char* Value = luaL_checkstring(L, -1);
                        StrProp->SetValue_InContainer(StructData, FString(UTF8_TO_TCHAR(Value)));
                    }
                    else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
                    {
                        int32 Value = (int32)luaL_checkinteger(L, -1);
                        IntProp->SetValue_InContainer(StructData, Value);
                    }
                    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
                    {
                        float Value = (float)luaL_checknumber(L, -1);
                        FloatProp->SetValue_InContainer(StructData, Value);
                    }
                    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
                    {
                        bool Value = lua_toboolean(L, -1) != 0;
                        BoolProp->SetPropertyValue_InContainer(StructData, Value);
                    }
                    // 可以添加更多类型支持
                }
                
                lua_pop(L, 1); // 弹出 value，保留 key
            }
            lua_pop(L, 1); // 弹出表

            // 广播消息
            UD->BroadcastMessageInternal(*Channel, ScriptStruct, StructData);

            // 清理
            ScriptStruct->DestroyStruct(StructData);
            FMemory::Free(StructData);

            return LuaObject::push(L, true);
        }

        // Get - 静态方法获取子系统实例
        // 参数: WorldContextObject - 世界上下文对象
        // 返回: UGameplayMessageSubsystem 实例
        int Get(lua_State* L)
        {
            UObject* WorldContext = LuaObject::checkUD<UObject>(L, 1);
            if (!WorldContext)
            {
                luaL_error(L, "arg 1 expect UObject as WorldContext");
                return 0;
            }

            if (!UGameplayMessageSubsystem::HasInstance(WorldContext))
            {
                return 0; // 返回 nil
            }

            UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(WorldContext);
            return LuaObject::push(L, &Subsystem);
        }

        // ListenForGameplayMessage - 注册消息监听器
        // 参数1: Channel - FGameplayTag 频道
        // 参数2: Callback - Lua 回调函数 function(Channel, Payload)
        // 参数3: StructType (可选) - 期望的消息结构体类型
        // 参数4: MatchType (可选) - 匹配类型 (ExactMatch=0, PartialMatch=1)
        // 返回: FGameplayMessageListenerHandle 监听器句柄，失败返回 nil
        int ListenForGameplayMessage(lua_State* L)
        {
            CheckUD(UGameplayMessageSubsystem, L, 1);
            if (!UD)
            {
                UE_LOG(LogYcGameplayMessageSlua, Error, TEXT("ListenForGameplayMessage: 参数1需要 UGameplayMessageSubsystem"));
                return 0; // 返回 nil
            }

            // 获取 Channel
            FGameplayTag* Channel = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Channel || !Channel->IsValid())
            {
                UE_LOG(LogYcGameplayMessageSlua, Error, TEXT("ListenForGameplayMessage: 参数2需要有效的 FGameplayTag，当前传入的 Tag 无效"));
                return 0; // 返回 nil
            }

            // 检查第3个参数是否是函数
            if (!lua_isfunction(L, 3))
            {
                luaL_error(L, "arg 3 expect Lua function");
                return 0;
            }

            // 获取期望的结构体类型（可选）
            UScriptStruct* ExpectedStructType = nullptr;
            if (lua_isuserdata(L, 4))
            {
                // import("FMyTestMessageStruct") 返回 UScriptStruct* userdata
                // 使用 checkValue 直接获取 UScriptStruct*
                ExpectedStructType = LuaObject::checkValue<UScriptStruct*>(L, 4);
            }
            else if (lua_isstring(L, 4))
            {
                // 支持字符串类型名,通过反射加载结构体类型
                const char* TypeName = luaL_checkstring(L, 4);
                const FString StructName = UTF8_TO_TCHAR(TypeName);
                ExpectedStructType = LoadObject<UScriptStruct>(nullptr, *StructName);
                // 如果未找到尝试添加F前缀
                if (!ExpectedStructType)
                {
                    const FString PrefixedName = FString::Printf(TEXT("F%s"), *StructName);
                    ExpectedStructType = LoadObject<UScriptStruct>(nullptr, *PrefixedName);
                }
            }

            // 获取匹配类型（可选）
            EGameplayMessageMatch MatchType = EGameplayMessageMatch::ExactMatch;
            if (lua_isinteger(L, 5))
            {
                int32 MatchValue = static_cast<int32>(lua_tointeger(L, 5));
                if (MatchValue >= 0 && MatchValue <= 1)
                {
                    MatchType = static_cast<EGameplayMessageMatch>(MatchValue);
                }
            }

            // 函数在栈位置 3，复制一份到栈顶供 LuaVar 使用
            lua_pushvalue(L, 3);
            int StackIndex = lua_gettop(L);

            // 创建 Lua 回调包装器（LuaVar 会从栈索引位置保存值副本）
            TSharedPtr<FLuaMessageCallback> LuaCallback = MakeShareable(new FLuaMessageCallback(L, StackIndex, ExpectedStructType));

            // 弹出复制的函数（LuaVar 已经保存了副本）
            lua_pop(L, 1);

            // 注册监听器
            FGameplayMessageListenerHandle Handle = UD->RegisterListenerInternal(
                *Channel,
                [LuaCallback](FGameplayTag ActualTag, const UScriptStruct* SenderStructType, const void* Payload)
                {
                    LuaCallback->Execute(ActualTag, SenderStructType, Payload);
                },
                ExpectedStructType,
                MatchType,
                true, // bUnregisterOnWorldDestroyed
                nullptr // UnregisterOnActorDestroyed
            );

            // 保存回调映射
            if (Handle.IsValid())
            {
                GLuaMessageCallbacks.Add(Handle.GetID(), LuaCallback);
            }

            return LuaObject::push(L, Handle);
        }

        // UnregisterListener - 取消消息监听
        // 参数1: Handle - FGameplayMessageListenerHandle 监听器句柄
        int UnregisterListener(lua_State* L)
        {
            CheckUD(UGameplayMessageSubsystem, L, 1);
            if (!UD)
            {
                luaL_error(L, "arg 1 expect UGameplayMessageSubsystem");
                return 0;
            }

            // 获取 Handle
            FGameplayMessageListenerHandle* Handle = LuaObject::checkValue<FGameplayMessageListenerHandle*>(L, 2);
            if (!Handle || !Handle->IsValid())
            {
                luaL_error(L, "arg 2 expect valid FGameplayMessageListenerHandle");
                return 0;
            }

            // 清理 Lua 回调
            GLuaMessageCallbacks.Remove(Handle->GetID());

            // 取消注册
            UD->UnregisterListener(*Handle);

            return 0;
        }

        void init()
        {
            // 注册实例方法
            REG_EXTENSION_METHOD_IMP(UGameplayMessageSubsystem, "BroadcastMessage", {
                return BroadcastMessage(L);
            });

            REG_EXTENSION_METHOD_IMP(UGameplayMessageSubsystem, "BroadcastMessageByType", {
                return BroadcastMessageByType(L);
            });

            REG_EXTENSION_METHOD_IMP(UGameplayMessageSubsystem, "ListenForGameplayMessage", {
                return ListenForGameplayMessage(L);
            });

            REG_EXTENSION_METHOD_IMP(UGameplayMessageSubsystem, "UnregisterListener", {
                return UnregisterListener(L);
            });

            // 注册静态方法
            REG_EXTENSION_METHOD_IMP_STATIC(UGameplayMessageSubsystem, "Get", {
                return Get(L);
            });
        }
    }
}

// 自动初始化注册器 - 使用 Slua 的 onInitEvent 委托
namespace
{
    struct FGameplayMessageSubsystemExtensionRegistrar
    {
        FGameplayMessageSubsystemExtensionRegistrar()
        {
            // UGameplayMessageSubsystem 扩展方法（UObject 有 StaticClass）
            NS_SLUA::GameplayMessageSubsystemExtension::init();
            
            // FGameplayMessageListenerHandle 扩展方法（结构体需要通过 onInitEvent 注册全局函数）
            NS_SLUA::LuaState::onInitEvent.AddStatic(&NS_SLUA::GameplayMessageListenerHandleExtension::init);
        }
    };

    static FGameplayMessageSubsystemExtensionRegistrar GGameplayMessageSubsystemExtensionRegistrar;
};

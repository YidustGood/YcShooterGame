// YcInventorySluaExtension.cpp
// 为 UYcInventoryLibrary 添加 Slua 扩展方法
// 将 TInstancedStruct 转换为 LuaStruct 供 Lua 使用

#include "DataRegistryId.h"
#include "LuaObject.h"
#include "LuaCppBinding.h"
#include "LuaState.h"
#include "YcInventoryLibrary.h"
#include "YcInventoryItemInstance.h"
#include "YcMessagePayloadLibrary.h"
#include "StructUtils/InstancedStruct.h"

#include "Logging/LogMacros.h"

// 定义日志类别
DEFINE_LOG_CATEGORY_STATIC(LogYcInventorySlua, Log, All);

namespace NS_SLUA
{
    DefTypeName(FDataRegistryId)
    DefTypeName(FYcInventoryItemDefinition)

    //=====================================================================
    // LuaStruct 扩展方法
    //=====================================================================
    namespace LuaStructExtension
    {
        // GetTypeName - 获取 LuaStruct 包装的结构体类型名称
        // 参数1: LuaStruct 对象
        // 返回: 结构体类型名称字符串，如 "FItemFragment_GridItem"
        int GetTypeName(lua_State* L)
        {
            LuaStruct* Struct = LuaObject::checkUD<LuaStruct>(L, 1);
            if (!Struct || !Struct->uss)
            {
                return LuaObject::push(L, FString("Unknown"));
            }
            return LuaObject::push(L, Struct->uss->GetName());
        }

        // GetTypeNameWithPrefix - 获取带前缀的结构体类型名称
        // 参数1: LuaStruct 对象
        // 返回: 完整类型名称字符串，如 "FItemFragment_GridItem" 或 "FYcInventoryItemDefinition"
        int GetTypeNameWithPrefix(lua_State* L)
        {
            LuaStruct* Struct = LuaObject::checkUD<LuaStruct>(L, 1);
            if (!Struct || !Struct->uss)
            {
                return LuaObject::push(L, FString("Unknown"));
            }
            // 使用 GetPathName 获取完整路径，然后提取类型名
            FString PathName = Struct->uss->GetPathName();
            return LuaObject::push(L, PathName);
        }

        // IsValid - 检查 LuaStruct 是否有效
        // 参数1: LuaStruct 对象
        // 返回: bool
        int IsValid(lua_State* L)
        {
            LuaStruct* Struct = LuaObject::checkUD<LuaStruct>(L, 1);
            return LuaObject::push(L, Struct && Struct->uss && Struct->buf);
        }

        void init(lua_State* L)
        {
            // 创建 LuaStruct 命名空间表（用于静态方法）
            lua_newtable(L);

            lua_pushstring(L, "GetTypeName");
            lua_pushcfunction(L, GetTypeName);
            lua_rawset(L, -3);

            lua_pushstring(L, "GetTypeNameWithPrefix");
            lua_pushcfunction(L, GetTypeNameWithPrefix);
            lua_rawset(L, -3);

            lua_pushstring(L, "IsValid");
            lua_pushcfunction(L, IsValid);
            lua_rawset(L, -3);

            lua_setglobal(L, "LuaStructHelper");
        }
    }

    namespace InventoryLibraryExtension
    {
        // FindItemFragmentAsLuaStruct - 从物品定义中查找 Fragment 并返回为 LuaStruct
        // 参数1: ItemDef - FYcInventoryItemDefinition 物品定义
        // 参数2: FragmentStructType - UScriptStruct* Fragment 类型
        // 返回: LuaStruct 包装的 Fragment 数据，未找到返回 nil
        int FindItemFragmentAsLuaStruct(lua_State* L)
        {
            // 获取 ItemDef
            FYcInventoryItemDefinition* ItemDef = LuaObject::checkValue<FYcInventoryItemDefinition*>(L, 1);
            if (!ItemDef)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("FindItemFragmentAsLuaStruct: 参数1需要 FYcInventoryItemDefinition"));
                return 0; // 返回 nil
            }

            // 获取 FragmentStructType
            UScriptStruct* FragmentStructType = LuaObject::checkValue<UScriptStruct*>(L, 2);
            if (!FragmentStructType)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("FindItemFragmentAsLuaStruct: 参数2需要 UScriptStruct"));
                return 0; // 返回 nil
            }

            // 调用原始函数
            FInstancedStruct Fragment = UYcInventoryLibrary::FindItemFragment(*ItemDef, FragmentStructType);

            // 检查是否有效
            if (!Fragment.IsValid())
            {
                UE_LOG(LogYcInventorySlua, Warning, TEXT("FindItemFragmentAsLuaStruct: 未找到 Fragment 类型 '%s'"), *FragmentStructType->GetName());
                return 0; // 返回 nil
            }

            // 获取 Fragment 数据
            UScriptStruct* StructType = const_cast<UScriptStruct*>(Fragment.GetScriptStruct());
            const uint8* StructData = reinterpret_cast<const uint8*>(Fragment.GetMemory());

            // 转换为 LuaStruct
            LuaStruct* LuaStructInstance = UYcMessagePayloadLibrary::CreateLuaStructFromData(L, StructType, StructData);
            if (LuaStructInstance)
            {
                return LuaObject::push(L, LuaStructInstance);
            }

            return 0; // 返回 nil
        }

        // FindItemFragmentByIdAsLuaStruct - 通过 ItemDefId 查找 Fragment 并返回为 LuaStruct
        // 参数1: ItemDefId - FDataRegistryId 物品定义ID
        // 参数2: FragmentStructType - UScriptStruct* Fragment 类型
        // 返回: LuaStruct 包装的 Fragment 数据，未找到返回 nil
        int FindItemFragmentByIdAsLuaStruct(lua_State* L)
        {
            // 获取 ItemDefId
            FDataRegistryId* ItemDefId = LuaObject::checkValue<FDataRegistryId*>(L, 1);
            if (!ItemDefId)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("FindItemFragmentByIdAsLuaStruct: 参数1需要 FDataRegistryId"));
                return 0; // 返回 nil
            }

            // 获取 FragmentStructType
            UScriptStruct* FragmentStructType = LuaObject::checkValue<UScriptStruct*>(L, 2);
            if (!FragmentStructType)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("FindItemFragmentByIdAsLuaStruct: 参数2需要 UScriptStruct"));
                return 0; // 返回 nil
            }

            // 调用原始函数
            FInstancedStruct Fragment = UYcInventoryLibrary::FindItemFragmentById(*ItemDefId, FragmentStructType);

            // 检查是否有效
            if (!Fragment.IsValid())
            {
                UE_LOG(LogYcInventorySlua, Warning, TEXT("FindItemFragmentByIdAsLuaStruct: 未找到 Fragment 类型 '%s'"), *FragmentStructType->GetName());
                return 0; // 返回 nil
            }

            // 获取 Fragment 数据
            UScriptStruct* StructType = const_cast<UScriptStruct*>(Fragment.GetScriptStruct());
            const uint8* StructData = reinterpret_cast<const uint8*>(Fragment.GetMemory());

            // 转换为 LuaStruct
            LuaStruct* LuaStructInstance = UYcMessagePayloadLibrary::CreateLuaStructFromData(L, StructType, StructData);
            if (LuaStructInstance)
            {
                return LuaObject::push(L, LuaStructInstance);
            }

            return 0; // 返回 nil
        }

        // FindItemFragmentByInstanceAsLuaStruct - 通过 ItemInstance 查找 Fragment 并返回为 LuaStruct
        // 参数1: ItemInstance - UYcInventoryItemInstance* 物品实例
        // 参数2: FragmentStructType - UScriptStruct* Fragment 类型
        // 返回: LuaStruct 包装的 Fragment 数据，未找到返回 nil
        int FindItemFragmentByInstanceAsLuaStruct(lua_State* L)
        {
            // 获取 ItemInstance
            UYcInventoryItemInstance* ItemInstance = LuaObject::checkValue<UYcInventoryItemInstance*>(L, 1);
            if (!ItemInstance)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("FindItemFragmentByInstanceAsLuaStruct: 参数1需要 UYcInventoryItemInstance"));
                return 0; // 返回 nil
            }

            // 获取 FragmentStructType
            UScriptStruct* FragmentStructType = LuaObject::checkValue<UScriptStruct*>(L, 2);
            if (!FragmentStructType)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("FindItemFragmentByInstanceAsLuaStruct: 参数2需要 UScriptStruct"));
                return 0; // 返回 nil
            }

            // 调用 ItemInstance 的 FindItemFragment 方法
            FInstancedStruct Fragment = ItemInstance->FindItemFragment(FragmentStructType);

            // 检查是否有效
            if (!Fragment.IsValid())
            {
                UE_LOG(LogYcInventorySlua, Warning, TEXT("FindItemFragmentByInstanceAsLuaStruct: 未找到 Fragment 类型 '%s'"), *FragmentStructType->GetName());
                return 0; // 返回 nil
            }

            // 获取 Fragment 数据
            UScriptStruct* StructType = const_cast<UScriptStruct*>(Fragment.GetScriptStruct());
            const uint8* StructData = reinterpret_cast<const uint8*>(Fragment.GetMemory());

            // 转换为 LuaStruct
            LuaStruct* LuaStructInstance = UYcMessagePayloadLibrary::CreateLuaStructFromData(L, StructType, StructData);
            if (LuaStructInstance)
            {
                return LuaObject::push(L, LuaStructInstance);
            }

            return 0; // 返回 nil
        }

        // GetItemDefinitionAsLuaStruct - 获取物品定义并返回为 LuaStruct
        // 参数1: ItemDefId - FDataRegistryId 物品定义ID
        // 返回: LuaStruct 包装的 FYcInventoryItemDefinition 数据，失败返回 nil
        int GetItemDefinitionAsLuaStruct(lua_State* L)
        {
            // 获取 ItemDefId
            FDataRegistryId* ItemDefId = LuaObject::checkValue<FDataRegistryId*>(L, 1);
            if (!ItemDefId)
            {
                UE_LOG(LogYcInventorySlua, Error, TEXT("GetItemDefinitionAsLuaStruct: 参数1需要 FDataRegistryId"));
                return 0; // 返回 nil
            }

            // 获取物品定义
            FYcInventoryItemDefinition ItemDef;
            if (!UYcInventoryLibrary::GetItemDefinition(*ItemDefId, ItemDef))
            {
                UE_LOG(LogYcInventorySlua, Warning, TEXT("GetItemDefinitionAsLuaStruct: 无法获取物品定义 '%s'"), *ItemDefId->ToString());
                return 0; // 返回 nil
            }

            // 获取 FYcInventoryItemDefinition 的 UScriptStruct
            UScriptStruct* StructType = FYcInventoryItemDefinition::StaticStruct();
            const uint8* StructData = reinterpret_cast<const uint8*>(&ItemDef);

            // 转换为 LuaStruct
            LuaStruct* LuaStructInstance = UYcMessagePayloadLibrary::CreateLuaStructFromData(L, StructType, StructData);
            if (LuaStructInstance)
            {
                return LuaObject::push(L, LuaStructInstance);
            }

            return 0; // 返回 nil
        }

        void init(lua_State* L)
        {
            // 创建 YcInventory 全局命名空间表
            lua_newtable(L);

            // 注册 FindItemFragmentAsLuaStruct
            lua_pushstring(L, "FindItemFragment");
            lua_pushcfunction(L, FindItemFragmentAsLuaStruct);
            lua_rawset(L, -3);

            // 注册 FindItemFragmentByIdAsLuaStruct
            lua_pushstring(L, "FindItemFragmentById");
            lua_pushcfunction(L, FindItemFragmentByIdAsLuaStruct);
            lua_rawset(L, -3);

            // 注册 FindItemFragmentByInstanceAsLuaStruct
            lua_pushstring(L, "FindItemFragmentByInstance");
            lua_pushcfunction(L, FindItemFragmentByInstanceAsLuaStruct);
            lua_rawset(L, -3);

            // 注册 GetItemDefinitionAsLuaStruct
            lua_pushstring(L, "GetItemDefinition");
            lua_pushcfunction(L, GetItemDefinitionAsLuaStruct);
            lua_rawset(L, -3);

            lua_setglobal(L, "YcInventory");
        }
    }
}

// 自动初始化注册器 - 使用 Slua 的 onInitEvent 委托
namespace
{
    struct FInventoryExtensionRegistrar
    {
        FInventoryExtensionRegistrar()
        {
            // 在 Lua 状态初始化时注册扩展方法
            NS_SLUA::LuaState::onInitEvent.AddStatic(&NS_SLUA::LuaStructExtension::init);
            NS_SLUA::LuaState::onInitEvent.AddStatic(&NS_SLUA::InventoryLibraryExtension::init);
        }
    };

    static FInventoryExtensionRegistrar GInventoryExtensionRegistrar;
}

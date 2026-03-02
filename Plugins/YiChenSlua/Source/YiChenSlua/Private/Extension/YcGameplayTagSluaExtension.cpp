// YcGameplayTagSluaExtension.cpp
// 为 Slua 注册 FGameplayTag 扩展方法，提供 Lua 友好的接口
// 注意：FGameplayTag 是 USTRUCT，没有 StaticClass()，所以使用全局函数方式

#include "LuaObject.h"
#include "LuaCppBinding.h"
#include "GameplayTagContainer.h"
#include "LuaState.h"
#include "YiChenSlua.h"

namespace NS_SLUA
{
    // 注册 FGameplayTag 和 FGameplayTagContainer 的类型名称
    // 这是 Slua 识别自定义结构体的必要步骤
    DefTypeName(FGameplayTag);
    DefTypeName(FGameplayTagContainer);

    namespace GameplayTagExtension
    {
        //=====================================================================
        // FGameplayTag 全局函数
        //=====================================================================
        
        // GameplayTag_ToString - 将 GameplayTag 转换为字符串
        int GameplayTag_ToString(lua_State* L)
        {
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 1);
            if (!Tag)
            {
                return LuaObject::push(L, FString("None"));
            }
            return LuaObject::push(L, Tag->ToString());
        }
        
        // GameplayTag_IsValid - 检查 Tag 是否有效
        int GameplayTag_IsValid(lua_State* L)
        {
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 1);
            return LuaObject::push(L, Tag ? Tag->IsValid() : false);
        }
        
        // GameplayTag_GetTagName - 获取 Tag 的 FName
        int GameplayTag_GetTagName(lua_State* L)
        {
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 1);
            if (!Tag)
            {
                return LuaObject::push(L, FName());
            }
            return LuaObject::push(L, Tag->GetTagName());
        }
        
        // GameplayTag_MatchesTag - 检查是否匹配指定 Tag
        int GameplayTag_MatchesTag(lua_State* L)
        {
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 1);
            FGameplayTag* OtherTag = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Tag || !OtherTag)
            {
                return LuaObject::push(L, false);
            }
            return LuaObject::push(L, Tag->MatchesTag(*OtherTag));
        }
        
        // GameplayTag_MatchesTagExact - 检查是否精确匹配
        int GameplayTag_MatchesTagExact(lua_State* L)
        {
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 1);
            FGameplayTag* OtherTag = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Tag || !OtherTag)
            {
                return LuaObject::push(L, false);
            }
            return LuaObject::push(L, Tag->MatchesTagExact(*OtherTag));
        }
        
        // GameplayTag_Equal - 检查两个 Tag 是否相等
        int GameplayTag_Equal(lua_State* L)
        {
            FGameplayTag* Tag1 = LuaObject::checkValue<FGameplayTag*>(L, 1);
            FGameplayTag* Tag2 = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Tag1 || !Tag2)
            {
                return LuaObject::push(L, false);
            }
            return LuaObject::push(L, Tag1->MatchesTagExact(*Tag2));
        }
        
        //=====================================================================
        // FGameplayTag 静态函数
        //=====================================================================
        
        // GameplayTag_RequestGameplayTag - 从字符串创建 GameplayTag
        int GameplayTag_RequestGameplayTag(lua_State* L)
        {
            const char* TagNameStr = luaL_checkstring(L, 1);
            if (!TagNameStr)
            {
                UE_LOG(LogYcSlua, Error, TEXT("RequestGameplayTag: 参数为空"));
                return 0; // 返回 nil
            }
            
            FName TagName(UTF8_TO_TCHAR(TagNameStr));
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
            
            if (!Tag.IsValid())
            {
                UE_LOG(LogYcSlua, Error, TEXT("RequestGameplayTag: 无效的 GameplayTag 名称 '%s'"), *TagName.ToString());
                return 0; // Tag 不存在，返回 nil
            }
            
            return LuaObject::push(L, Tag);
        }
        
        // GameplayTag_IsTagValid - 检查指定名称的 Tag 是否存在
        int GameplayTag_IsTagValid(lua_State* L)
        {
            const char* TagNameStr = luaL_checkstring(L, 1);
            if (!TagNameStr)
            {
                return LuaObject::push(L, false);
            }
            
            FName TagName(UTF8_TO_TCHAR(TagNameStr));
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
            return LuaObject::push(L, Tag.IsValid());
        }
        
        // GameplayTag_EmptyTag - 获取空 Tag
        int GameplayTag_EmptyTag(lua_State* L)
        {
            return LuaObject::push(L, FGameplayTag::EmptyTag);
        }
        
        //=====================================================================
        // FGameplayTagContainer 全局函数
        //=====================================================================
        
        // GameplayTagContainer_ToString - 转换为字符串
        int GameplayTagContainer_ToString(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            if (!Container)
            {
                return LuaObject::push(L, FString(""));
            }
            return LuaObject::push(L, Container->ToStringSimple());
        }
        
        // GameplayTagContainer_HasTag - 检查是否包含指定 Tag
        int GameplayTagContainer_HasTag(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (!Container || !Tag)
            {
                return LuaObject::push(L, false);
            }
            return LuaObject::push(L, Container->HasTag(*Tag));
        }
        
        // GameplayTagContainer_HasAll - 检查是否包含所有指定 Tag
        int GameplayTagContainer_HasAll(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            FGameplayTagContainer* Other = LuaObject::checkValue<FGameplayTagContainer*>(L, 2);
            if (!Container || !Other)
            {
                return LuaObject::push(L, false);
            }
            return LuaObject::push(L, Container->HasAll(*Other));
        }
        
        // GameplayTagContainer_HasAny - 检查是否包含任意指定 Tag
        int GameplayTagContainer_HasAny(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            FGameplayTagContainer* Other = LuaObject::checkValue<FGameplayTagContainer*>(L, 2);
            if (!Container || !Other)
            {
                return LuaObject::push(L, false);
            }
            return LuaObject::push(L, Container->HasAny(*Other));
        }
        
        // GameplayTagContainer_AddTag - 添加 Tag
        int GameplayTagContainer_AddTag(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (Container && Tag)
            {
                Container->AddTag(*Tag);
            }
            return 0;
        }
        
        // GameplayTagContainer_RemoveTag - 移除 Tag
        int GameplayTagContainer_RemoveTag(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            FGameplayTag* Tag = LuaObject::checkValue<FGameplayTag*>(L, 2);
            if (Container && Tag)
            {
                Container->RemoveTag(*Tag);
            }
            return 0;
        }
        
        // GameplayTagContainer_Num - 获取 Tag 数量
        int GameplayTagContainer_Num(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            if (!Container)
            {
                return LuaObject::push(L, 0);
            }
            return LuaObject::push(L, Container->Num());
        }
        
        // GameplayTagContainer_IsValid - 检查容器是否有效
        int GameplayTagContainer_IsValid(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            return LuaObject::push(L, Container ? Container->IsValid() : false);
        }
        
        // GameplayTagContainer_Reset - 重置容器
        int GameplayTagContainer_Reset(lua_State* L)
        {
            FGameplayTagContainer* Container = LuaObject::checkValue<FGameplayTagContainer*>(L, 1);
            if (Container)
            {
                Container->Reset();
            }
            return 0;
        }

        //=====================================================================
        // 方法注册表
        //=====================================================================
        static luaL_Reg GameplayTagMethods[] = {
            // FGameplayTag 实例方法
            {"ToString", GameplayTag_ToString},
            {"IsValid", GameplayTag_IsValid},
            {"GetTagName", GameplayTag_GetTagName},
            {"MatchesTag", GameplayTag_MatchesTag},
            {"MatchesTagExact", GameplayTag_MatchesTagExact},
            {"Equal", GameplayTag_Equal},
            // FGameplayTag 静态方法
            {"RequestGameplayTag", GameplayTag_RequestGameplayTag},
            {"IsTagValid", GameplayTag_IsTagValid},
            {"EmptyTag", GameplayTag_EmptyTag},
            {nullptr, nullptr}
        };

        static luaL_Reg GameplayTagContainerMethods[] = {
            {"ToString", GameplayTagContainer_ToString},
            {"HasTag", GameplayTagContainer_HasTag},
            {"HasAll", GameplayTagContainer_HasAll},
            {"HasAny", GameplayTagContainer_HasAny},
            {"AddTag", GameplayTagContainer_AddTag},
            {"RemoveTag", GameplayTagContainer_RemoveTag},
            {"Num", GameplayTagContainer_Num},
            {"IsValid", GameplayTagContainer_IsValid},
            {"Empty", GameplayTagContainer_Reset},
            {nullptr, nullptr}
        };

        //=====================================================================
        // 初始化注册
        //=====================================================================
        void init(lua_State* L)
        {
            // 创建 GameplayTag 命名空间表
            lua_newtable(L);
            for (luaL_Reg* reg = GameplayTagMethods; reg->name; ++reg)
            {
                lua_pushstring(L, reg->name);
                lua_pushcfunction(L, reg->func);
                lua_rawset(L, -3);
            }
            lua_setglobal(L, "GameplayTag");
            
            // 创建 GameplayTagContainer 命名空间表
            lua_newtable(L);
            for (luaL_Reg* reg = GameplayTagContainerMethods; reg->name; ++reg)
            {
                lua_pushstring(L, reg->name);
                lua_pushcfunction(L, reg->func);
                lua_rawset(L, -3);
            }
            lua_setglobal(L, "GameplayTagContainer");
        }
    }
}

// 自动初始化注册器 - 使用 Slua 的 onInitEvent 委托
namespace
{
    struct FGameplayTagExtensionRegistrar
    {
        FGameplayTagExtensionRegistrar()
        {
            // 在 Lua 状态初始化时注册扩展方法
            NS_SLUA::LuaState::onInitEvent.AddStatic(&NS_SLUA::GameplayTagExtension::init);
        }
    };

    static FGameplayTagExtensionRegistrar GGameplayTagExtensionRegistrar;
}

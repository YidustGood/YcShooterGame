// YcGameplayTagSluaExtension.cpp
// 为 Slua 注册 FGameplayTag 扩展方法，提供 Lua 友好的接口
// 注意：FGameplayTag 是 USTRUCT，没有 StaticClass()，所以使用全局函数方式

#include "LuaObject.h"
#include "LuaCppBinding.h"
#include "GameplayTagContainer.h"
#include "LuaState.h"

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
                return 0; // 返回 nil
            }
            
            FName TagName(UTF8_TO_TCHAR(TagNameStr));
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
            
            if (!Tag.IsValid())
            {
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
        // 初始化注册
        //=====================================================================
        void init(lua_State* L)
        {
            // 注册 FGameplayTag 方法为全局函数
            LuaObject::addGlobalMethod(L, "GameplayTag_ToString", GameplayTag_ToString);
            LuaObject::addGlobalMethod(L, "GameplayTag_IsValid", GameplayTag_IsValid);
            LuaObject::addGlobalMethod(L, "GameplayTag_GetTagName", GameplayTag_GetTagName);
            LuaObject::addGlobalMethod(L, "GameplayTag_MatchesTag", GameplayTag_MatchesTag);
            LuaObject::addGlobalMethod(L, "GameplayTag_MatchesTagExact", GameplayTag_MatchesTagExact);
            LuaObject::addGlobalMethod(L, "GameplayTag_Equal", GameplayTag_Equal);
            
            // 注册 FGameplayTag 静态方法
            LuaObject::addGlobalMethod(L, "GameplayTag_RequestGameplayTag", GameplayTag_RequestGameplayTag);
            LuaObject::addGlobalMethod(L, "GameplayTag_IsTagValid", GameplayTag_IsTagValid);
            LuaObject::addGlobalMethod(L, "GameplayTag_EmptyTag", GameplayTag_EmptyTag);
            
            // 注册 FGameplayTagContainer 方法
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_ToString", GameplayTagContainer_ToString);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_HasTag", GameplayTagContainer_HasTag);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_HasAll", GameplayTagContainer_HasAll);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_HasAny", GameplayTagContainer_HasAny);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_AddTag", GameplayTagContainer_AddTag);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_RemoveTag", GameplayTagContainer_RemoveTag);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_Num", GameplayTagContainer_Num);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_IsValid", GameplayTagContainer_IsValid);
            LuaObject::addGlobalMethod(L, "GameplayTagContainer_Empty", GameplayTagContainer_Reset);
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

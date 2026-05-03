// YcShooter 项目 - 消息 Payload 辅助函数库
// 为 Slua 提供友好的 Payload 获取接口

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcMessagePayloadLibrary.generated.h"

class UAsyncAction_ListenForGameplayMessage;
class UScriptStruct;

// 前向声明 Slua 的 LuaStruct
namespace NS_SLUA
{
	struct lua_State;
	struct LuaStruct;
}

/**
 * 消息 Payload 辅助函数库
 * 为 Slua 提供获取 GameplayMessage Payload 的能力
 * 通过CppBinding的形式提供给Lua侧调用
 */
UCLASS()
class YICHENSLUA_API UYcMessagePayloadLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 获取 Payload 数据作为 LuaStruct（供 Slua 内部使用）
	 * 
	 * 此函数返回一个 LuaStruct 包装的 Payload 数据
	 * 在 Lua 中可以直接访问结构体字段
	 *
	 * @param AsyncListener 异步消息监听器
	 * @return LuaStruct 包装的 Payload 数据，如果没有数据则返回 nil
	 */
	static NS_SLUA::LuaStruct* GetPayloadAsLuaStruct(const UAsyncAction_ListenForGameplayMessage* AsyncListener);

	/**
	 * 把UE侧的具备反射的结构体转换为LuaStruct, 以支持Lua访问
	 * @param L Lua 状态机指针，用于注册结构体地址以支持嵌套结构体访问
	 * @param StructType UE侧结构体类型
	 * @param StructData 结构体数据首地址
	 * @return LuaStruct 实例，供 LuaObject::push 使用
	 */
	static NS_SLUA::LuaStruct* CreateLuaStructFromData(slua::lua_State* L, UScriptStruct* StructType, const uint8* StructData);
};

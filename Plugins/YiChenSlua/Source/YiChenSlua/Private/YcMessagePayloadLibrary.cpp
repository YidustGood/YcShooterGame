// YcMessagePayloadLibrary.cpp

#include "YcMessagePayloadLibrary.h"
#include "GameFramework/AsyncAction_ListenForGameplayMessage.h"
#include "LuaObject.h"

NS_SLUA::LuaStruct* UYcMessagePayloadLibrary::GetPayloadAsLuaStruct(const UAsyncAction_ListenForGameplayMessage* AsyncListener)
{
	if (!AsyncListener || !AsyncListener->HasPayload())
	{
		return nullptr;
	}

	UScriptStruct* StructType = AsyncListener->GetPayloadStructType();
	const uint8* PayloadData = AsyncListener->GetPayloadData();
	
	if (!StructType || !PayloadData)
	{
		return nullptr;
	}

	// 创建 Payload 数据的副本
	const uint32 Size = StructType->GetStructureSize();
	uint8* Buffer = static_cast<uint8*>(FMemory::Malloc(Size));
	StructType->InitializeStruct(Buffer);
	StructType->CopyScriptStruct(Buffer, PayloadData);

	// 创建 LuaStruct 包装
	NS_SLUA::LuaStruct* LuaStructInstance = new NS_SLUA::LuaStruct();
	LuaStructInstance->Init(Buffer, Size, StructType, false);

	return LuaStructInstance;
}

// 1. 定义辅助函数
slua::LuaStruct* UYcMessagePayloadLibrary::CreateLuaStructFromData(slua::lua_State* L, UScriptStruct* StructType, const uint8* StructData)
{
	if (!StructType || !StructData)
	{
		return nullptr;
	}

	// 创建 StructData 数据的副本
	const uint32 Size = StructType->GetStructureSize();
	uint8* Buffer = static_cast<uint8*>(FMemory::Malloc(Size));
	StructType->InitializeStruct(Buffer);
	StructType->CopyScriptStruct(Buffer, StructData);

	// 创建 LuaStruct 包装, 供Lua侧使用
	NS_SLUA::LuaStruct* LuaStructInstance = new NS_SLUA::LuaStruct();
	LuaStructInstance->Init(Buffer, Size, StructType, false);

	// 关键：注册 buffer 地址到 propLinks，使嵌套结构体访问能够正常工作
	// 这样当访问嵌套结构体属性时，linkProp 可以找到父结构体的地址
	NS_SLUA::LuaObject::addLink(L, Buffer);

	return LuaStructInstance;
}

// 2. 注册扩展方法
// REG_EXTENSION_METHOD_IMP(UMyClass, "GetMyStruct", {
// 	CheckUD(UMyClass, L, 1);
//
// 	// 假设 MyClass 有 GetStructData() 方法
// 	UScriptStruct* StructType = UD->GetStructType();
// 	const void* Data = UD->GetStructData();
//
// 	LuaStruct* ls = CreateLuaStructFromData(StructType, Data);
// 	if (ls) {
// 		return LuaObject::push(L, ls);
// 	}
// 	return 0;  // 返回 nil
// });
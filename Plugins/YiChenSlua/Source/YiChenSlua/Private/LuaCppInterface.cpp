// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "LuaCppInterface.h"

#include "YcLuaStateManager.h"

// 将CPP的接口绑定到LuaTable,以便Lua可以调用我们这些CPP接口函数, 通过LuaCppInterface这个全局表


// 返回值为TimerIndex, 可以用于清理Timer
int SetTimer(slua::lua_State *L)
{
	/*
	 * 获取3个参数
	 * #1 interval
	 * #2 looping
	 * #3 callback
	 */
	float interval = slua::LuaObject::checkValue<float>(L, 1);
	bool looping = slua::LuaObject::checkValue<bool>(L, 2);
	slua::LuaVar *func = new slua::LuaVar(L, 3, slua::LuaVar::LV_FUNCTION);

	FYcLuaStateManager & mgr = FYcLuaStateManager::GetYiChenLuaStateManager();
	int idx = mgr.SetTimer(interval, looping, func);
	return slua::LuaObject::push(L, idx);
}

int ClearTimer(slua::lua_State *L)
{
	int idx = slua::LuaObject::checkValue<int>(L, 1);
	FYcLuaStateManager & mgr = FYcLuaStateManager::GetYiChenLuaStateManager();
	mgr.ClearTimer(idx);
	return 0;
}

static slua::luaL_Reg CppInterfaceMethods[] = {
	{"SetTimer",						SetTimer},
	{"ClearTimer",						ClearTimer}
};

void create_table(slua::lua_State* L, slua::luaL_Reg* funcs)
{
	lua_newtable(L);									// t

	for (; funcs->name; ++funcs)
	{
		lua_pushstring(L, funcs->name);					// t, func_name
		lua_pushcfunction(L, funcs->func);				// t, func_name, func
		lua_rawset(L, -3);								// t
	}

	lua_setglobal(L, "LuaCppInterface");					// 
}

int LuaCppInterface::OpenLib(slua::lua_State* L)
{
	create_table(L, CppInterfaceMethods);
	return 0;
}
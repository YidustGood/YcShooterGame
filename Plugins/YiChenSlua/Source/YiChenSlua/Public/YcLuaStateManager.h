// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "LuaState.h"
class UGameInstance;

/**
 * Lua状态管理器
 * 单例类，负责管理项目中的LuaState实例，提供Lua环境的初始化、关闭和定时器管理功能
 * 将Lua相关逻辑从GameInstance中分离，提高代码的可维护性和复用性
 */
class YICHENSLUA_API FYcLuaStateManager
{
private:
	/** 私有构造函数，确保单例模式 */
	FYcLuaStateManager();
	
public:
	/** 析构函数，负责清理Lua状态 */
	~FYcLuaStateManager();
	
	/** 获取Lua状态管理器的单例实例 */
	static FYcLuaStateManager& GetYiChenLuaStateManager();
	
	/**
	 * 初始化Lua状态管理器
	 * @param InGameInstance 游戏实例指针，用于访问定时器管理器等功能
	 * @return 0表示成功
	 */
	int Init(UGameInstance* InGameInstance);
	
	/**
	 * 关闭Lua状态管理器，清理所有资源
	 * @return 0表示成功，1表示未初始化
	 */
	int Close();
	
	/** 获取关联的游戏实例 */
	UGameInstance* GetGameInstance() const;
	
	/**
	 * 设置Lua定时器
	 * @param Interval 定时器间隔（秒）
	 * @param bLooping 是否循环执行
	 * @param Func Lua函数指针（LuaVar*）
	 * @return 定时器索引，用于后续清除定时器
	 */
	int SetTimer(float Interval, bool bLooping, void* Func);
	
	/**
	 * 清除指定的定时器
	 * @param Index 定时器索引
	 * @return 0表示成功
	 */
	int ClearTimer(int Index);
	
	/**
	 * 清除所有定时器
	 * @return 0表示成功
	 */
	int ClearAllTimers();
	
protected:
	/**
	 * Lua状态初始化回调函数
	 * 在LuaState创建时被调用，用于注册全局函数（如PrintLog）
	 */
	void LuaStateInitCallback(NS_SLUA::lua_State* L);
	
	/** 创建LuaState实例并配置文件加载委托 */
	void CreateLuaState();
	
	/** 关闭并销毁LuaState实例 */
	void CloseLuaState();
	
private:
	/** LuaState实例指针 */
	NS_SLUA::LuaState* LuaStateInstance;
	
	/** 游戏实例指针，用于访问定时器等功能 */
	UGameInstance* GameInstance;
	
	/** 是否已初始化标志 */
	bool bInitialized;
	
	/** 定时器映射表，键为定时器索引，值为定时器句柄 */
	TMap<int, FTimerHandle> Timers;
	
	/** 定时器索引计数器，用于生成唯一的定时器ID */
	int TimerIndex;
};

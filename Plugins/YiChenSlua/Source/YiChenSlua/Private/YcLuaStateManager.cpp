// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcLuaStateManager.h"

#include "LuaCppInterface.h"

/**
 * 读取文件内容（辅助函数，当前未使用）
 * @param PlatformFile 平台文件系统接口
 * @param path 文件路径
 * @param len 输出参数，文件长度
 * @return 文件内容缓冲区指针
 */
static uint8* ReadFile(IPlatformFile& PlatformFile, FString path, uint32& len) {
	IFileHandle* FileHandle = PlatformFile.OpenRead(*path);
	if (FileHandle) {
		len = (uint32)FileHandle->Size();
		uint8* buf = new uint8[len];

		FileHandle->Read(buf, len);

		// 关闭文件句柄
		delete FileHandle;

		return buf;
	}

	return nullptr;
}

FYcLuaStateManager::FYcLuaStateManager()
	: LuaStateInstance(nullptr), GameInstance(nullptr), bInitialized(false), TimerIndex(0)
{
}

FYcLuaStateManager::~FYcLuaStateManager()
{
	// 析构时关闭Lua状态并重置初始化标志
	CloseLuaState();
	bInitialized = false;
}

FYcLuaStateManager& FYcLuaStateManager::GetYiChenLuaStateManager()
{
	// 静态局部变量实现线程安全的单例模式（C++11保证）
	static FYcLuaStateManager YiChenLuaStateManager;
	return YiChenLuaStateManager;
}

int FYcLuaStateManager::Init(UGameInstance* InGameInstance)
{
	GameInstance = InGameInstance;

	// 避免重复初始化
	if (bInitialized)
		return 0;
	
	// 创建LuaState实例
	CreateLuaState();

	bInitialized = true;
	return 0;
}

int FYcLuaStateManager::Close()
{
	// 如果未初始化则直接返回
	if (!bInitialized)
		return 1;
	
	// 关闭Lua状态
	CloseLuaState();
	bInitialized = false;
	
	// 清理所有定时器
	ClearAllTimers();
	return 0;
}

UGameInstance* FYcLuaStateManager::GetGameInstance() const
{
	return GameInstance;
}

int FYcLuaStateManager::SetTimer(const float Interval, const bool bLooping, void* Func)
{
	// 将void*转换为Lua函数变量
	slua::LuaVar* LuaFunc = static_cast<slua::LuaVar*>(Func);
	
	// 创建定时器委托，绑定Lambda表达式来调用Lua函数
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda([](const slua::LuaVar* InLuaFunc)
	{
		if (InLuaFunc) InLuaFunc->call();
	}, LuaFunc);
	
	// 设置定时器并保存句柄
	FTimerHandle Handler;
	GameInstance->GetTimerManager().SetTimer(Handler, TimerDelegate, Interval, bLooping);
	
	// 递增索引并存储定时器句柄
	++TimerIndex;
	Timers.Add(TimerIndex, Handler);
	
	// 返回定时器索引，供Lua侧保存和后续清除使用
	return TimerIndex;
}

int FYcLuaStateManager::ClearTimer(const int Index)
{
	// 检查定时器是否存在
	if (!Timers.Contains(Index))
		return 0;

	// 获取定时器句柄并清除
	FTimerHandle Handler = Timers[Index];
	Timers.Remove(Index);
	GameInstance->GetTimerManager().ClearTimer(Handler);
	return 0;
}

int FYcLuaStateManager::ClearAllTimers()
{
	// 遍历所有定时器并清除
	for (TPair<int, FTimerHandle> it : Timers)
	{
		GameInstance->GetTimerManager().ClearTimer(it.Value);
	}
	
	// 清空定时器映射表并重置索引
	Timers.Empty();
	TimerIndex = 0;
	return 0;
}

/**
 * Lua全局打印函数
 * 在Lua中可以通过PrintLog()函数输出日志到UE的日志系统
 */
static int32 PrintLog(NS_SLUA::lua_State *L)
{
	FString str;
	size_t len;
	// 将Lua栈上的第一个参数转换为字符串
	const char* s = luaL_tolstring(L, 1, &len);
	if (s) str += UTF8_TO_TCHAR(s);
	// 输出到UE日志系统
	NS_SLUA::Log::Log("PrintLog %s", TCHAR_TO_UTF8(*str));
	return 0;
}

void FYcLuaStateManager::LuaStateInitCallback(slua::lua_State* L)
{
	// 注册PrintLog函数为Lua全局函数
	lua_pushcfunction(L, PrintLog);
	lua_setglobal(L, "PrintLog");
}

void FYcLuaStateManager::CreateLuaState()
{
	// 注册Lua状态初始化回调，用于在LuaState创建时注册全局函数
	NS_SLUA::LuaState::onInitEvent.AddRaw(this, &FYcLuaStateManager::LuaStateInitCallback);

	// 确保先关闭旧的LuaState实例
	CloseLuaState();
	
	// 创建新的LuaState实例
	LuaStateInstance = new NS_SLUA::LuaState("SLuaMainState", GameInstance);
	
	// 设置Lua文件加载委托，定义如何从项目中加载Lua文件
	LuaStateInstance->setLoadFileDelegate([](const char* fn, FString& filepath)->TArray<uint8> {

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		
		// 构建Lua文件路径：Content/Lua/模块名/文件名.lua
		FString path = FPaths::ProjectContentDir();
		FString filename = UTF8_TO_TCHAR(fn);
		path /= "Lua";
		// 将Lua模块名中的点号转换为路径分隔符（例如：module.test -> module/test）
		path /= filename.Replace(TEXT("."), TEXT("/"));

		TArray<uint8> Content;
		// 尝试加载.lua和.luac文件
		TArray<FString> luaExts = { UTF8_TO_TCHAR(".lua"), UTF8_TO_TCHAR(".luac") };
		for (auto& it : luaExts) {
			auto fullPath = path + *it;

			FFileHelper::LoadFileToArray(Content, *fullPath);
			if (Content.Num() > 0) {
				filepath = fullPath;
				return MoveTemp(Content);
			}
		}

		// 文件不存在则返回空数组
		return MoveTemp(Content);
	});
	
	// 初始化LuaState
	LuaStateInstance->init();
}

void FYcLuaStateManager::CloseLuaState()
{
	if (LuaStateInstance)
	{
		// 关闭Lua状态并释放内存
		LuaStateInstance->close();
		delete LuaStateInstance;
		LuaStateInstance = nullptr;
	}
}
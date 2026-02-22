// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "slua.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YiChenSluaSubsystem.generated.h"

/**
 * Lua运行代码，需要一个LuaState维护Lua虚拟机状态。
 * GameInstance的生命周期比较长，相比World(切换Persistent Level地图时会加载/卸载)，更适合维护Lua虚拟机。
 * 但是直接在GameInstance中编写代码侵入性更大, 所以我们这里直接采用创建专门的GameInstanceSubsystem来管理LuaState
 * 职责清晰明了，生命周期跟随GameInstance
 */
UCLASS()
class YICHENSLUA_API UYiChenSluaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface
	
	/** 运行任意Lua文件代码 */
	UFUNCTION(BlueprintCallable)
	void RunLuaFile(FString FileName);
	
	void LuaStateInitCallback(NS_SLUA::lua_State* L);

	void CreateLuaState();
	void CloseLuaState();

	// create global state, freed on app exit
	NS_SLUA::LuaState* state;
};

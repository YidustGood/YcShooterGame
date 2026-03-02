// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcGameInstance.h"

#include "YcShooterGameInstance.generated.h"

/**
 * YcShooter这个项目所使用的GameInstance基类
 * 自动维护了一个主要的LuaState以支持Slua的使用
 */
UCLASS()
class YCSHOOTER_API UYcShooterGameInstance : public UYcGameInstance
{
	GENERATED_BODY()
public:
	UYcShooterGameInstance(const FObjectInitializer& ObjectInitializer);
	
	/** 执行Lua脚本文件 */
	UFUNCTION(BlueprintCallable, Category="Slua")
	void DoLuaFile(const FString& FileName) const;
	
	virtual void Init() override;
	virtual void Shutdown() override;
};

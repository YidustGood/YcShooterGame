// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcRedDotManagerSubsystem.h"
#include "UObject/Interface.h"
#include "YcRedDotDataProvider.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UYcRedDotDataProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class YICHENREDDOTSYSTEM_API IYcRedDotDataProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RedDot")
	FGameplayTag GetTargetRedDotTag() const;
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "RedDot")
	void OnNotifyRedDotCleared();
	
	/**
	 * 初始化红点提供者（在 Widget NativeConstruct 中调用）
	 * 自动注册所有标签的监听和依赖
	 */
	virtual void InitializeRedDotDataProvider();

	/**
	 * 反初始化红点提供者（在 Widget NativeDestruct 中调用）
	 * 自动注销所有监听和依赖
	 */
	virtual void UninitializeRedDotDataProvider();
	
protected:
	// 监听 Handle 存储, 用于注销监听
	FYcRedDotDataProviderHandle ClearedHandle;
};

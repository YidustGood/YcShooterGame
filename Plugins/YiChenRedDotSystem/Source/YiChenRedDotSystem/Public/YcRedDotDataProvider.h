// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcRedDotManagerSubsystem.h"
#include "UObject/Interface.h"
#include "YcRedDotDataProvider.generated.h"

UINTERFACE()
class UYcRedDotDataProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 红点数据提供者接口, 实现了这个接口就可以向GetTargetRedDotTag()所返回的目标红点贡献数据
 * 例如: 
 * 邮件标签UI(就是显示有多少封邮件未读的那个图标), 它实现了UYcRedDotProvider接口, 将UI与RedDot.Mail这个红点标签做绑定,
 * RedDot.Mail的红点数据发生变化时邮件标签UI可以根据新的红点数据做未读数量的显示更新。
 * 那么RedDot.Mail的红点数据变化从何而来呢? 
 * 例如当邮件系统收到一份新的邮件时, 先通过调用通过邮件系统触发UYcRedDotManagerSubsystem::AddRedDotCount("RedDot.Mail", 1)
 * 让邮件数量+1, 邮件系统会存有邮件数据(例如JSON), 邮件列表页面会有N个邮件条目(已读+未读), 就需要通过邮件数据JSON创建N个邮件UI条目,
 * 创建的时候把对应的邮件数据(包含是否已读的状态标记)写入邮件UI中, 就实现了基本邮件列表功能。
 * 当点击阅读一条邮件时如何让邮件红点数量-1呢？ 直接调用UYcRedDotManagerSubsystem::AddRedDotCount("RedDot.Mail", -1)，
 * 然后同步清理这个邮件条目UI的红点显示状态即可。 那么如果需要一键阅读清空所有邮件系统红点的时候怎么办呢？ 简单来说直接遍历邮件条目然后调用它们的清理红点函数即可。
 * 进阶的做法就是让邮件条目UI实现IYcRedDotDataProvider, 并实现GetTargetRedDotTag()返回RedDot.Mail, 表示这个UI是邮件红点数据的提供者/贡献者之一。
 * 会在创建UI的时候自动向红点子系统注册自己, 然后调用UYcRedDotManagerSubsystem::ClearRedDotStateInBranch("RedDot.Mail")清空邮件红点数据的时候
 * 会自动调用所有邮件条目UI的OnNotifyRedDotCleared()事件, 然后只需要在蓝图里实现这个事件做红点状态清除逻辑即可。 
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

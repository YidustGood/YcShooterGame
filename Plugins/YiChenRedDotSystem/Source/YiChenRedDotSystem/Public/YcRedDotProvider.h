// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcRedDotManagerSubsystem.h"
#include "YcRedDotTypes.h"
#include "UObject/Interface.h"
#include "YcRedDotProvider.generated.h"

/**
 * 红点系统提供者接口
 * 任何需要使用红点系统的 Widget 都应该实现此接口, C++和蓝图都可以实现
 * 
 * 使用方式:
 * 1. 在 Widget 类声明中添加: IYcRedDotProvider
 * 2. 实现GetRedDotTag() 返回要绑定的RedDotTag
 * 3. 在 NativeConstruct() 中调用 InitializeRedDotProvider()
 * 4. 在 NativeDestruct() 中调用 UninitializeRedDotProvider()
 * 5. 在UI蓝图中实现OnRedDotStateChanged或者OnNotifyRedDotUpdated 以数据变化更新UI红点显示效果
 * 
 * 优势:
 * - 避免蓝图异步节点开销
 * - 集中式管理红点监听
 * - 自动生命周期管理
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UYcRedDotProvider : public UInterface
{
	GENERATED_BODY()
};

class YICHENREDDOTSYSTEM_API IYcRedDotProvider
{
	GENERATED_BODY()

public:
	/**
	 * 获取该提供者关联的红点标签, 必须被实现
	 * @return 红点标签数组
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RedDot")
	FGameplayTag GetRedDotTag() const;

	/**
	 * 红点状态变化回调
	 * @param RedDotTag 变化的红点标签
	 * @param RedDotInfo 红点信息
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RedDot")
	void OnRedDotStateChanged(FGameplayTag RedDotTag, const FRedDotInfo& RedDotInfo);
	
	/**
	 * 通知UI红点更新事件, 蓝图实现这个函数然后在这里做红点UI部件的可视性、数量更新
	 * 这个函数跟OnRedDotStateChanged实际上是类似的, 知识在其基础上计算简化了传递的参数, 是为了便于使用扩展的一个函数
	 * @param bVisibility 新的可视状态, 直接同步设置给红点UI部件
	 * @param Count 数量, 需要显示带数量的红点就把这个设置给红点UI部件
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "RedDot")
	void OnNotifyRedDotUpdated(const bool bVisibility, const int32 Count);

	/**
	 * 初始化红点提供者（在 Widget NativeConstruct 中调用）
	 * 自动注册所有标签的监听和依赖
	 */
	virtual void InitializeRedDotProvider();

	/**
	 * 反初始化红点提供者（在 Widget NativeDestruct 中调用）
	 * 自动注销所有监听和依赖
	 */
	virtual void UninitializeRedDotProvider();

protected:
	// 监听 Handle 存储, 用于注销监听
	TMap<FGameplayTag, FYcRedDotStateChangedListenerHandle> StateChangedHandles;
};

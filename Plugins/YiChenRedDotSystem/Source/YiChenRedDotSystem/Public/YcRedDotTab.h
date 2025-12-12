// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcRedDotProvider.h"
#include "Blueprint/UserWidget.h"
#include "YcRedDotTab.generated.h"

/**
 * 基于YcRedDotSystem实现的支持红点功能的UI标签案例
 * 在你的项目中使用的时候可以参考本类代码, 通过继承IYcRedDotProvider接口并提供必要实现即可轻松为你的项目接入UI红点系统
 * UYcRedDotManagerSubsystem::Get()可以获取红点管理子系统对象, 提供了一些基于Tag的操作接口
 */
UCLASS(Abstract)
class YICHENREDDOTSYSTEM_API UYcRedDotTab : public UUserWidget , public IYcRedDotProvider
{
	GENERATED_BODY()
public:
	/** 必要: 在NativeConstruct()调用IYcRedDotProvider::InitializeRedDotProvider()注册红点监听 */
	virtual void NativeConstruct() override;
	/** 必要: 在NativeDestruct()调用IYcRedDotProvider::UninitializeRedDotProvider()注销红点监听 */
	virtual void NativeDestruct() override;
	
	// IYcRedDotProvider interface
	/** 必要: 必须重写实现的接口, 用于返回UI标签所要绑定的RedDotTag */
	virtual FGameplayTag GetRedDotTag_Implementation() const override;
	
	/** 可选: 红点数据发生变化的回调, 这里我们重写它缓存了一下RedDotInfo到TabRedDotInfo */
	virtual void OnRedDotStateChanged_Implementation(
		FGameplayTag RedDotTag, const FRedDotInfo& RedDotInfo) override;
	// ~IYcRedDotProvider interface
	
private:
	
	/** 必要: 这个Tab所要绑定的RedDotTag, 一定要在UI蓝图中指定 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RedDot", meta=(AllowPrivateAccess=true))
	FGameplayTag TabRedDotTag;
	
	/** 可选: 缓存红点数据 */
	UPROPERTY(BlueprintReadOnly, Category = "RedDot", meta=(AllowPrivateAccess=true))
	FRedDotInfo TabRedDotInfo;
};

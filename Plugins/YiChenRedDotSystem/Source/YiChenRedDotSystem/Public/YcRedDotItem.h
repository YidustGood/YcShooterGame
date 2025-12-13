// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcRedDotDataProvider.h"
#include "Blueprint/UserWidget.h"
#include "YcRedDotItem.generated.h"

/**
 * 
 */
UCLASS()
class YICHENREDDOTSYSTEM_API UYcRedDotItem : public UUserWidget, public IYcRedDotDataProvider
{
	GENERATED_BODY()
public:
	/** 必要: 在NativeConstruct()IYcRedDotDataProvider::InitializeRedDotProvider()注册目标红点清理事件监听 */
	virtual void NativeConstruct() override;
	/** 必要: 在NativeDestruct()IYcRedDotDataProvider::UninitializeRedDotProvider()注销目标红点清理事件监听 */
	virtual void NativeDestruct() override;
	
	// IYcRedDotDataProvider interface
	/** 必要: 必须重写实现的接口, 用于返回该物品UI的目标RedDotTag */
	virtual FGameplayTag GetTargetRedDotTag_Implementation() const override;
	// ~IYcRedDotDataProvider interface

private:
	/** 
	 * 必要: 这个物品的目标RedDotTag, 也可以理解为物品所属类型的RedDotTag
	 * 会向目标提供数据, 以便红点标签能感知到物品存在，一定要在UI蓝图中指定 
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RedDot", meta=(AllowPrivateAccess=true))
	FGameplayTag TargetTabRedDotTag;
};

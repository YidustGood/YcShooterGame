// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Fragments/YcEquipmentFragment.h"
#include "YcEquipmentFragment_ReticleConfig.generated.h"

class UUserWidget;

/**
 * 武器准星UI类Fragment
 * 用法：创建一个Widget当作准星代理UI(也可以理解为准星UI的容器, 准星位置就是由这个代理UI确定的)
 * 然后在代理UI中监测Yc_QuickBar_Message_ActiveIndexChanged的消息
 * 监测到装备武器发生变化时就从新武器对象上获取到这个Fragment, 拿到ReticleWidgets进行创建显示即可
 */
USTRUCT(BlueprintType, meta=(DisplayName="武器准星UI配置"))
struct YICHENSHOOTERCORE_API FYcEquipmentFragment_ReticleConfig : public FYcEquipmentFragment
{
	GENERATED_BODY()

	FYcEquipmentFragment_ReticleConfig();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reticle")
	TArray<TSubclassOf<UUserWidget>> ReticleWidgets;
};
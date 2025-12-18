// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcInteractionStatics.generated.h"

class IYcInteractableTarget;
template <typename InterfaceType> class TScriptInterface;

/**
 * @class UYcInteractionStatics
 * @brief 交互系统的蓝图函数库，提供了一系列用于处理交互目标的静态工具函数。
 */
UCLASS()
class YICHENGAMEPLAY_API UYcInteractionStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UYcInteractionStatics();
	
	/**
	 * 从一个可交互目标接口获取其所属的Actor。
	 * @param InteractableTarget 实现了IYcInteractableTarget接口的对象。
	 * @return 如果目标是Actor或Actor组件，则返回其所属的Actor；否则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static AActor* GetActorFromInteractableTarget(TScriptInterface<IYcInteractableTarget> InteractableTarget);

	/**
	 * 从给定的Actor上获取所有实现了IYcInteractableTarget接口的对象。
	 * 这包括Actor本身以及其拥有的所有组件。
	 * @param Actor 要从中查找交互目标的Actor。
	 * @param OutInteractableTargets 用于存储找到的交互目标接口的数组。
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void GetInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IYcInteractableTarget>>& OutInteractableTargets);

	/**
	 * 从一组重叠检测结果中提取并追加所有可交互目标。
	 * @param OverlapResults 物理重叠查询的结果数组。
	 * @param OutInteractableTargets 用于追加找到的可交互目标的数组。
	 */
	static void AppendInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IYcInteractableTarget>>& OutInteractableTargets);
	
	/**
	 * 从一个命中检测结果中提取所有可交互目标。
	 * @param HitResult 物理射线或形状检测的命中结果。
	 * @param OutInteractableTargets 用于存储找到的可交互目标的数组。
	 */
	static void GetInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IYcInteractableTarget>>& OutInteractableTargets);
};

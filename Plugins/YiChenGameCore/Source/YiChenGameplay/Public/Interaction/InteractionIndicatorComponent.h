// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "InteractionIndicatorComponent.generated.h"

class UInteractionIndicatorComponent;
class IYcInteractableTarget;

/**
 * 指示器包装结构
 * 用于包装玩家可交互对象数据, 在需要交互的时候取出使用
 */
USTRUCT(BlueprintType)
struct FIndicatorDescriptor
{
	GENERATED_BODY()
	
public:	
	/** 指向的Actor类型可交互对象 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> TargetActor;
	
	/** 交互对象的接口实例 */
	UPROPERTY(BlueprintReadOnly)
	TScriptInterface<IYcInteractableTarget> InteractableTarget;
	
	UInteractionIndicatorComponent* GetIndicatorManagerComponent() const { return ManagerPtr.Get(); }
	void SetIndicatorManagerComponent(UInteractionIndicatorComponent* InManager);
	
private:
	/** 所属的交互指示器组件, 通过这个所属组件可以获得相关的玩家角色等对象 */
	UPROPERTY()
	TWeakObjectPtr<UInteractionIndicatorComponent> ManagerPtr; 
};

/**
 * 交互指示器组件, 存储管理玩家当前可交互的对象数据
 * 是挂载在玩家控制器上的
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UInteractionIndicatorComponent : public UControllerComponent
{
	GENERATED_BODY()
public:
	UInteractionIndicatorComponent(const FObjectInitializer& ObjectInitializer);

	static UInteractionIndicatorComponent* GetComponent(const AController* Controller);

	/** 添加指示器 */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	void AddIndicator(FIndicatorDescriptor IndicatorDescriptor);
	
	/** 获取首个指示器, 适用于获取玩家视野聚焦的交互物体数据 */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	bool GetFirstIndicator(FIndicatorDescriptor& InOutIndicator);
	
	/** 清空所有指示器 */
	UFUNCTION(BlueprintCallable, Category = Indicator)
	void EmptyIndicators();
	
	const TArray<FIndicatorDescriptor>& GetIndicators() const { return Indicators; }

private:
	/** 存储玩家当前可进行交互的对象数据列表 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly , meta=(AllowPrivateAccess = "true"))
	TArray<FIndicatorDescriptor> Indicators;
};
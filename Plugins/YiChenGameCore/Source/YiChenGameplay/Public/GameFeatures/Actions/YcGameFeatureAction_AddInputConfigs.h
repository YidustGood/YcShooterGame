// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcGameFeatureAction_WorldActionBase.h"
#include "YcGameFeatureAction_AddInputConfigs.generated.h"

class AActor;
class UInputMappingContext;
class UPlayer;
class APlayerController;
struct FComponentRequestHandle;
class UYcInputConfig;

/**
 *  Adds InputMappingContext to local players' EnhancedInput system. 
 * Expects that local players are set up to use the EnhancedInput system.
 * 添加InputConfig绑定,目前仅支持AbilityInputActions的绑定，暂不支持NativeInputActions的绑定
 * 依赖HeroComponent实现输入配置的绑定/移除
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Input Configs"))
class UYcGameFeatureAction_AddInputConfigs : public UYcGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
	//~ Begin UGameFeatureAction interface
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeatureAction interface

#if WITH_EDITOR
	/** 编辑器下的数据校验, 用于检查InputConfigs有效性, 避免在开发时意外配置了空的InputConfig */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	
	/** 要添加的InputConfig列表 */
	UPROPERTY(EditAnywhere, Category="Input", meta=(AssetBundles="Client, Server"))
	TArray<TSoftObjectPtr<const UYcInputConfig>> InputConfigs;
private:
	/** 
	 * 记录这个Action的上下文信息结构体, 
	 * 激活时添加记录信息, 取消激活时通过记录的信息做清理
	 * 大多数Action都可用参考这个模板, 包含委托回调句柄+操作了的对象弱指针这两个属性
	 */
	struct FPerContextData
	{
		/** 监听扩展事件的委托句柄列表, 记录下来用于结束时清理 */
		TArray<TSharedPtr<FComponentRequestHandle>> ExtensionRequestHandles;
		/** 
		 * 已经被扩展添加了InputConfig的Pawn对象列表, 记录下来用于结束时移除InputConfig 
		 * 此处用弱指针避免影响对象生命周期, 因为Pawn如果自己已经销毁了, 我们也无需在清理它了
		 */
		TArray<TWeakObjectPtr<APawn>> PawnsAddedTo;
	};
	/** 基于世界上下文存储的Action上下文数据, 以正确区分多个World */
	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	//~ Begin UGameFeatureAction_WorldActionBase interface
	/** Action具体要执行的操作入口 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~ End UGameFeatureAction_WorldActionBase interface

	/** 重置函数, 对已添加的Pawn对象会做移除操作 */
	void Reset(FPerContextData& ActiveData);
	
	/** 响应扩展事件的回调函数, 在这里通过传入的事件名判断是进行添加还是移除输入配置的操作 */
	void HandlePawnExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext);
	
	/** 添加输入配置 */
	void AddInputConfigForPlayer(APawn* Pawn, FPerContextData& ActiveData);
	/** 移除输入配置 */
	void RemoveInputConfig(APawn* Pawn, FPerContextData& ActiveData);
};

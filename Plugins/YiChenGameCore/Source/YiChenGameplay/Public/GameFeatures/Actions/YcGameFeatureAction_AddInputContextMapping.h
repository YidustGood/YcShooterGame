// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcGameFeatureAction_WorldActionBase.h"
#include "YcGameFeatureAction_AddInputContextMapping.generated.h"

class UInputMappingContext;
struct FComponentRequestHandle;

/** 输入映射上下文及其优先级的配置结构体 */
USTRUCT()
struct FYcInputMappingContextAndPriority
{
	GENERATED_BODY()

	/** 输入映射上下文资源 */
	UPROPERTY(EditAnywhere, Category="Input", meta=(AssetBundles="Client, Server"))
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	/** 输入映射的优先级，优先级越高越优先应用 */
	UPROPERTY(EditAnywhere, Category="Input")
	int32 Priority = 0;

	/** 是否在GameFeatureAction注册时将此IMC注册到设置系统，注册后才能在用户按键设置中检索 */
	UPROPERTY(EditAnywhere, Category="Input")
	bool bRegisterWithSettings = true;
};

/**
 * 将输入映射上下文(IMC)添加到本地玩家的增强输入系统中
 * 此功能需要本地玩家已配置为使用增强输入系统。
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Input Context Mapping"))
class UYcGameFeatureAction_AddInputContextMapping : public UYcGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
	//~ Begin UGameFeatureAction interface
	/** 游戏功能注册时调用，注册输入映射上下文到设置系统 */
	virtual void OnGameFeatureRegistering() override;
	
	/** 游戏功能激活时调用，为当前World中的玩家添加输入映射 */
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	
	/** 游戏功能停用时调用，移除为玩家添加的输入映射 */
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	
	/** 游戏功能注销时调用，从设置系统注销输入映射上下文 */
	virtual void OnGameFeatureUnregistering() override;
	//~ End of UGameFeatureAction interface
	
	//~ Begin UObject interface
#if WITH_EDITOR
	/** 编辑器中验证数据有效性，确保所有输入映射上下文都已配置 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~ End of UObject interface
	
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<FYcInputMappingContextAndPriority> InputMappings;
	
private:
	/** 每个World上下文的数据结构，管理该上下文中的扩展请求和本地玩家 */
	struct FPerContextData
	{
		/** 扩展请求处理句柄数组 */
		TArray<TSharedPtr<FComponentRequestHandle>> ExtensionRequestHandles;
		/** 已添加输入映射的本地玩家列表 */
		TArray<TWeakObjectPtr<ULocalPlayer>> LocalPlayersAddedTo;
	};
	/** 按World上下文存储的数据映射表 */
	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	//~ Begin UGameFeatureAction_WorldActionBase interface
	/** 将输入映射添加到指定World中 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~ End of UGameFeatureAction_WorldActionBase interface

	/** 清理指定上下文的所有数据 */
	void Reset(FPerContextData& ActiveData);
	
	/** 处理PlayerController扩展事件，添加或移除输入映射 */
	void HandleControllerExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext);
	
	/** 为指定本地玩家添加输入映射 */
	void AddInputMappingForPlayer(ULocalPlayer* LocalPlayer, FPerContextData& ActiveData);
	
	/** 移除指定本地玩家的输入映射 */
	void RemoveInputMapping(ULocalPlayer* LocalPlayer, FPerContextData& ActiveData);

	// 以下函数用于处理IMC在用户设置中的注册与注销
	// 仅当bRegisterWithSettings=true时才会注册，注册后才能在用户按键设置中检索

	/** 注册所有输入映射上下文到设置系统，并绑定GameInstance启动和LocalPlayer添加/移除事件 */
	void RegisterInputMappingContexts();

	/** 为指定GameInstance注册输入映射上下文，并绑定其LocalPlayer事件 */
	void RegisterInputContextMappingsForGameInstance(UGameInstance* GameInstance);

	/** 为指定LocalPlayer注册输入映射上下文到设置系统 */
	void RegisterInputMappingContextsForLocalPlayer(ULocalPlayer* LocalPlayer);

	/** 从设置系统注销所有输入映射上下文，并解绑GameInstance和LocalPlayer事件 */
	void UnregisterInputMappingContexts();

	/** 为指定GameInstance注销输入映射上下文 */
	void UnregisterInputContextMappingsForGameInstance(UGameInstance* GameInstance);

	/** 为指定LocalPlayer注销输入映射上下文 */
	void UnregisterInputMappingContextsForLocalPlayer(ULocalPlayer* LocalPlayer);
	
	/** GameInstance启动事件的委托句柄 */
	FDelegateHandle RegisterInputContextMappingsForGameInstanceHandle;
};

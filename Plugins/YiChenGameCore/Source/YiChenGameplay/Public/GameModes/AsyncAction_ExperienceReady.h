// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncAction_ExperienceReady.generated.h"

class UYcExperienceDefinition;



/**
 * 异步等待游戏体验就绪并有效，然后调用 OnReady 事件。如果游戏状态已有效，将立即调用 OnReady。
 * 适用于等待到游戏体验加载完成后进行某些操作，例如依赖游戏体验的某些资产那必须等到加载完成后进行操作才是安全的
 */
UCLASS()
class YICHENGAMEPLAY_API UAsyncAction_ExperienceReady : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
public:
	// 等待Experience被确定并加载
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly="true"))
	static UAsyncAction_ExperienceReady* WaitForExperienceReady(UObject* WorldContextObject);

	virtual void Activate() override;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FExperienceReadyAsyncDelegate);
	// 当Experience被确定并已就绪/加载时调用
	UPROPERTY(BlueprintAssignable)
	FExperienceReadyAsyncDelegate OnReady;

private:
	void Step1_HandleGameStateSet(AGameStateBase* GameState);
	void Step2_ListenToExperienceLoading(const AGameStateBase* GameState);
	void Step3_HandleExperienceLoaded(const UYcExperienceDefinition* CurrentExperience);
	void Step4_BroadcastReady();

	TWeakObjectPtr<UWorld> WorldPtr;
};
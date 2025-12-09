// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncAction_PawnExtensionReady.generated.h"

class UYcPawnExtensionComponent;
/**
 * 
 */
UCLASS()
class YICHENGAMEPLAY_API UAsyncAction_PawnExtensionReady : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	// 等待Experience被确定并加载
	UFUNCTION(BlueprintCallable, meta=(WorldContext = "WorldContextObject", BlueprintInternalUseOnly="true"))
	static UAsyncAction_PawnExtensionReady* WaitForPawnExtensionReady(UObject* WorldContextObject);

	virtual void Activate() override;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnExtensionReadyAsyncDelegate);
	// 当Pawn被确定并已就绪/加载时调用
	UPROPERTY(BlueprintAssignable)
	FPawnExtensionReadyAsyncDelegate OnReady;

private:
	UFUNCTION()
	void BroadcastReady();

	TWeakObjectPtr<UYcPawnExtensionComponent> PawnExtCompPtr;
};

// Copyright (c) 2025 YiChen. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YcReplicableObject.generated.h"

/**
 * 支持网络复制的UObject基类，需要注意Outer始终得是可复制的Actor对象.
 * 参考链接：https://jambax.co.uk/replicating-uobjects/
 * 支持作为子对象进行网络复制的UObject基类
 * 在开启网络复制的Actors中的使用案例：案例参考Experiment/UObjectRepTest/ReplicatedSubObjectActor.h
 * 构造函数添加：bReplicateUsingRegisteredSubObjectList = true
 *  UPROPERTY(Replicated)必须在头文件中添加
	USCRepObject* RepObj = NewObject<USCRepObject>(this, UClass* Class);
	// 新使用方法
	AddReplicatedSubObject(RepObj);
	RemoveReplicatedSubObject(RepObj);
	
	// 旧使用方法重写ReplicateSubobjects，5.1被遗弃
	virtual bool ReplicateSubobjects(UActorChannel *Channel, FOutBunch *Bunch, FReplicationFlags *RepFlags) override
	{
		bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
		
		// Single Object
		bWroteSomething |= Channel->ReplicateSubobject(MyObject, *Bunch, *RepFlags);
		
		// Array of Objects
		bWroteSomething |= Channel->ReplicateSubobjectList(ArrayOfMyObject, *Bunch, *RepFlags);
		
		return bWroteSomething;
	}
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew)
class YICHENGAMECORE_API UYcReplicableObject : public UObject
{
	GENERATED_BODY()
public:
	UYcReplicableObject(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UWorld* GetWorld() const override;

	// 子UObject调用RPC必须重写的一个函数
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

	// Call "Remote" (aka, RPC) functions through the actors NetDriver
	// 通过Actor调用RPC
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack) override;

	UFUNCTION(BlueprintPure, Category = "Replicate")
	AActor* GetActorOuter() const;

	/*
	* Optional
	* Since this is a replicated object, typically only the Server should create and destroy these
	* Provide a custom destroy function to ensure these conditions are met.
	* 由于这是一个复制对象，通常只有服务器才能创建和销毁它们
	* 提供自定义destroy函数以确保满足这些条件。根据bReplicatedFlag进行判断
	*/
	UFUNCTION(BlueprintCallable)
	void Destroy();

	UFUNCTION(BlueprintCallable, Category = "Replicate")
	void SetBReplicatedFlag(uint8 InReplicatedFlag);

protected:
	/** 是否支持网络复制的标记 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated, Category = "Replicate", meta=(AllowPrivateAccess = "true"))
	uint8 bReplicatedFlag : 1;

	/** 对象被销毁时的网络多播通知函数 通知到所有关联的网络客户端包括服务端*/
	UFUNCTION(NetMulticast, Reliable)
	void MultiOnDestroyed();

	/** C++的对象销毁回调 */
	virtual void OnDestroyed();

	/** 蓝图可实现事件-对象销毁回调 */
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "On Destroyed", ScriptName = "OnDestroyed", Keywords="destroyed"))
	void K2_OnDestroyed();

public:
	FORCEINLINE uint8 GetReplicatedFlag() const { return bReplicatedFlag; }
};

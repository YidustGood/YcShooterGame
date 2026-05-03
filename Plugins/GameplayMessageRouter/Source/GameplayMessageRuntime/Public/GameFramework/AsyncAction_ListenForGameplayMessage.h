// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "GameplayMessageSubsystem.h"
#include "GameplayMessageTypes2.h"

#include "AsyncAction_ListenForGameplayMessage.generated.h"

class UScriptStruct;
class UWorld;
struct FFrame;

/**
 * Proxy object pin will be hidden in K2Node_GameplayMessageAsyncAction. Is used to get a reference to the object triggering the delegate for the follow up call of 'GetPayload'.
 *
 * @param ActualChannel		The actual message channel that we received Payload from (will always start with Channel, but may be more specific if partial matches were enabled)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncGameplayMessageDelegate, UAsyncAction_ListenForGameplayMessage*, ProxyObject, FGameplayTag, ActualChannel);

UCLASS(BlueprintType, meta=(HasDedicatedAsyncNode))
class GAMEPLAYMESSAGERUNTIME_API UAsyncAction_ListenForGameplayMessage : public UCancellableAsyncAction
{
	GENERATED_BODY()

public:
	/**
	 * Asynchronously waits for a gameplay message to be broadcast on the specified channel.
	 *
	 * @param Channel											The message channel to listen for
	 * @param PayloadType									The kind of message structure to use (this must match the same type that the sender is broadcasting)
	 * @param MatchType										The rule used for matching the channel with broadcasted messages
	 * @param bUnregisterOnWorldDestroyed	Whether or not this listener should be automatically unregistered when the world is destroyed (set to false for GameInstance listeners)
	 * @param UnregisterOnActorDestroyed	An optional actor reference that will automatically unregister this listener when it is destroyed
	 */
	UFUNCTION(BlueprintCallable, Category = Messaging, meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ListenForGameplayMessage* ListenForGameplayMessages(
		UObject* WorldContextObject,
		FGameplayTag Channel,
		UScriptStruct* PayloadType,
		EGameplayMessageMatch MatchType = EGameplayMessageMatch::ExactMatch,
		bool bUnregisterOnWorldDestroyed = true,
		AActor* UnregisterOnActorDestroyed = nullptr
	);

	/**
	 * Attempt to copy the payload received from the broadcasted gameplay message into the specified wildcard.
	 * The wildcard's type must match the type from the received message.
	 *
	 * @param OutPayload	The wildcard reference the payload should be copied into
	 * @return				If the copy was a success
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Messaging", meta = (CustomStructureParam = "OutPayload"))
	bool GetPayload(UPARAM(ref) int32& OutPayload);

	DECLARE_FUNCTION(execGetPayload);
	
	// YiChen: slua support
	// 为Lua提供支持的核心操作实际上就是把结构体数据拷贝构建为一个LuaStruct实例给到Lua侧去使用即可
	/**
	 * 获取消息载荷的结构体类型
	 * @return 当前存储的 Payload 的 UScriptStruct 类型，如果没有则返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Messaging")
	UScriptStruct* GetPayloadStructType() const { return MessageStructType.Get(); }

	/**
	 * 检查是否有缓存的 Payload 数据
	 * @return 是否有有效的 Payload 数据
	 */
	UFUNCTION(BlueprintPure, Category = "Messaging")
	bool HasPayload() const { return ReceivedMessagePayloadPtr != nullptr; }

	/**
	 * 获取 Payload 数据大小（字节）
	 * @return Payload 数据大小，如果没有数据则返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "Messaging")
	int32 GetPayloadSize() const 
	{ 
		return MessageStructType.IsValid() ? MessageStructType->GetStructureSize() : 0; 
	}

	/**
	 * 获取 Payload 缓存数据指针（供 Slua 等脚本语言使用）
	 * @return Payload 数据指针，如果没有数据则返回 nullptr
	 */
	const uint8* GetPayloadData() const 
	{ 
		return static_cast<const uint8*>(ReceivedMessagePayloadPtr); 
	}
	// ~YiChen: slua support

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

public:
	/** Called when a message is broadcast on the specified channel. Use GetPayload() to request the message payload. */
	UPROPERTY(BlueprintAssignable)
	FAsyncGameplayMessageDelegate OnMessageReceived;

private:
	void HandleMessageReceived(FGameplayTag Channel, const UScriptStruct* StructType, const void* Payload);

private:
	const void* ReceivedMessagePayloadPtr = nullptr;

	TWeakObjectPtr<UWorld> WorldPtr;
	FGameplayTag ChannelToRegister;
	TWeakObjectPtr<UScriptStruct> MessageStructType = nullptr;
	EGameplayMessageMatch MessageMatchType = EGameplayMessageMatch::ExactMatch;
	bool bUnregisterOnWorldDestroyed;
	AActor* UnregisterOnActorDestroyed;

	FGameplayMessageListenerHandle ListenerHandle;
};

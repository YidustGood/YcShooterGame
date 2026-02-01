// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YiChenGameCore.h"
#include "GameFramework/GameplayMessageSubsystem.h"

/**
 * RAII 包装类，专门用于成员函数版本的消息监听
 * 自动管理消息监听器的注册和取消注册
 */
template<typename TMessageType, typename TOwner = UObject>
class TScopedGameplayMessageListener
{
public:
    // 类型别名
    using FHandlerFunction = void(TOwner::*)(FGameplayTag, const TMessageType&);
    
    // 默认构造函数
    TScopedGameplayMessageListener() = default;
    
    /**
     * 构造并立即注册监听器（成员函数版本）
     * 
     * @param Channel 消息频道标签
     * @param Object 拥有成员函数的对象实例
     * @param Function 要调用的成员函数
     * @param bUnregisterOnWorldDestroyed 世界销毁时是否自动取消注册
     */
    TScopedGameplayMessageListener(
        FGameplayTag Channel,
        TOwner* Object,
        FHandlerFunction Function,
        bool bUnregisterOnWorldDestroyed = true
    )
    {
        Register(Channel, Object, Function, bUnregisterOnWorldDestroyed);
    }
    
    /**
     * 析构函数 - 自动取消注册
     */
    ~TScopedGameplayMessageListener()
    {
        Unregister();
    }
    
    // 禁止拷贝
    TScopedGameplayMessageListener(const TScopedGameplayMessageListener&) = delete;
    TScopedGameplayMessageListener& operator=(const TScopedGameplayMessageListener&) = delete;
    
    // 允许移动
    TScopedGameplayMessageListener(TScopedGameplayMessageListener&& Other) noexcept
        : ListenerHandle(MoveTemp(Other.ListenerHandle))
    {
        // Other.ListenerHandle.Reset();
    }
    
    TScopedGameplayMessageListener& operator=(TScopedGameplayMessageListener&& Other) noexcept
    {
        if (this != &Other)
        {
            Unregister();
            ListenerHandle = MoveTemp(Other.ListenerHandle);
            // Other.ListenerHandle.Reset();
        }
        return *this;
    }
    
    /**
     * 注册监听器
     * 
     * @param Channel 消息频道标签
     * @param Object 拥有成员函数的对象实例
     * @param Function 要调用的成员函数
     * @param bUnregisterOnWorldDestroyed 世界销毁时是否自动取消注册
     */
    void Register(
        FGameplayTag Channel,
        TOwner* Object,
        FHandlerFunction Function,
        bool bUnregisterOnWorldDestroyed = true
    )
    {
        Unregister(); // 先取消之前的注册
        
        if (Object && Function)
        {
            UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Object);
            
            // 使用你提供的成员函数版本 RegisterListener
            ListenerHandle = Subsystem.RegisterListener<TMessageType>(
                Channel,
                Object,
                Function,
                bUnregisterOnWorldDestroyed
            );
        }
    }
    
    /**
     * 手动取消注册
     */
    void Unregister()
    {
        if (ListenerHandle.IsValid())
        {
            ListenerHandle.Unregister();
            // ListenerHandle.Reset();
            UE_LOG(LogYcGameCore, Log, TEXT("TScopedGameplayMessageListener::Unregistered"));
        }
    }
    
    /**
     * 检查是否正在监听
     */
    bool IsRegistered() const
    {
        return ListenerHandle.IsValid();
    }
    
    /**
     * 获取监听器句柄
     */
    const FGameplayMessageListenerHandle& GetHandle() const
    {
        return ListenerHandle;
    }
    
    /**
     * 重置监听器
     */
    void Reset()
    {
        Unregister();
    }
    
private:
    FGameplayMessageListenerHandle ListenerHandle;
};
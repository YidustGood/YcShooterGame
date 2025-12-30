// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "YcRedDotTypes.generated.h"

class UYcRedDotManagerSubsystem;

/** 红点GameplayTag匹配规则 */
UENUM(BlueprintType)
enum class EYcRedDotTagMatch : uint8
{
    // 精确匹配 - 仅接收通道完全相同的消息
    // (例如：注册监听"A.B"时，能接收到A.B广播的消息，但收不到A.B.C的消息)
    ExactMatch,

    // 部分匹配 - 接收以相同通道为根的所有消息, 红点数据向上穿透就是用的这个规则
    // (例如：注册监听"A.B"时，既能接收到A.B广播的消息，也能接收到A.B.C的消息)
    PartialMatch
};

/** 红点类型枚举 */
UENUM(BlueprintType)
enum class ERedDotType : uint8
{
    Normal      UMETA(DisplayName = "Normal"),        // 普通红点
    Important   UMETA(DisplayName = "Important"),     // 重要红点
    Urgent      UMETA(DisplayName = "Urgent"),        // 紧急红点
    Count       UMETA(DisplayName = "Count"),         // 数字红点
    Text        UMETA(DisplayName = "Text")           // 文本红点
};

/** 红点数据结构体 */
USTRUCT(BlueprintType)
struct YICHENREDDOTSYSTEM_API FRedDotInfo
{
    GENERATED_BODY()

    // 默认构造函数
    FRedDotInfo()
        : Tag()
        , Delta(0)
        , TriggerTime(FDateTime::MinValue())
        , Count(0)
        , Priority(0)
        , Type(ERedDotType::Normal)
        , bIsActive(false)
    {}

    // 带参构造函数
    FRedDotInfo(bool bInActive, int32 InCount = 0, ERedDotType InType = ERedDotType::Normal)
        : Tag()
        , Delta(0)
        , TriggerTime(FDateTime::Now())
        , Count(InCount)
        , Priority(0)
        , Type(InType)
        , bIsActive(bInActive)
    {}
    
    /** 红点路径标签 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    FGameplayTag Tag;
    
    /** 红点更新时与上一次的数量差距 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    int32 Delta;
    
    /** 红点更新时间 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    FDateTime TriggerTime;
    
    /** 红点当前的数量, 如果非叶子节点则数量由子节点透传聚合而来 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    int32 Count;
    
    /** 优先级 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    uint8 Priority;
    
    /** 红点类型 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    ERedDotType Type;
    
    /** 是否激活 */
    UPROPERTY(BlueprintReadOnly, Category = "RedDot")
    bool bIsActive;
    
    
    bool operator==(const FRedDotInfo& Other) const
    {
        return bIsActive == Other.bIsActive &&
               Count == Other.Count &&
               Priority == Other.Priority &&
               Type == Other.Type &&
               Tag == Other.Tag;
    }
    
    bool operator!=(const FRedDotInfo& Other) const
    {
        return !(*this == Other);
    }
};

/** 红点状态数据变化监听者数据, 包含回调函数等 */
USTRUCT()
struct YICHENREDDOTSYSTEM_API FYcRedDotStateChangedListenerData
{
    GENERATED_BODY()
    // 红点状态数据发生变化的回调函数
    TFunction<void(FGameplayTag, const FRedDotInfo*)> ReceivedCallback;
	
    // 记录这个Tag的第几个监听者
    int32 HandleID; 
    EYcRedDotTagMatch MatchType;
    bool bUnregisterOnWorldDestroyed = true;
};

/** 
 * 红点数据提供者绑定数据(这就是为红点标签的红点数据做贡献的目标对象包装, 红点标签数据是由所有提供者提供的数据聚合而来)
 * 用于记录RedDotTag与那些提供者有关, 在RedDotTag发生清理事件时利用这里的回调函数通知提供者清理它们自身的红点数据
 */
USTRUCT()
struct YICHENREDDOTSYSTEM_API FYcRedDotDataProviderData
{
    GENERATED_BODY()

    // 红点标签清空时要调用的回调函数, 用于通知提供者清空红点数据状态
    TFunction<void()> ClearedCallback;
	
    // 提供者要提供的目标RedDotTag
    UPROPERTY(Transient)
    FGameplayTag RedDotTag;
	
    // 记录这个Tag的第几个提供者
    int32 HandleID; 
};

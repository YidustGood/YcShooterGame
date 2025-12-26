// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameplayCommon/YcGameVerbMessage.h"
#include "GameplayCueManager.h"
#include "GameFramework/PlayerState.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameVerbMessage)

//////////////////////////////////////////////////////////////////////
// FYcGameVerbMessage

FString FYcGameVerbMessage::ToString() const
{
	// 使用反射系统将结构体导出为可读的文本格式
	FString HumanReadableMessage;
	FYcGameVerbMessage::StaticStruct()->ExportText(/*out*/ HumanReadableMessage, this, /*Defaults=*/ nullptr, /*OwnerObject=*/ nullptr, PPF_None, /*ExportRootScope=*/ nullptr);
	return HumanReadableMessage;
}

//////////////////////////////////////////////////////////////////////
// UYcGameVerbMessageHelpers

APlayerState* UYcGameVerbMessageHelpers::GetPlayerStateFromObject(UObject* Object)
{
	// 如果对象本身就是PlayerController，直接返回其PlayerState
	if (APlayerController* PC = Cast<APlayerController>(Object))
	{
		return PC->PlayerState;
	}

	// 如果对象本身就是PlayerState，直接返回
	if (APlayerState* TargetPS = Cast<APlayerState>(Object))
	{
		return TargetPS;
	}
	
	// 如果对象是Pawn，尝试获取其PlayerState
	if (APawn* TargetPawn = Cast<APawn>(Object))
	{
		if (APlayerState* TargetPS = TargetPawn->GetPlayerState())
		{
			return TargetPS;
		}
	}
	return nullptr;
}

APlayerController* UYcGameVerbMessageHelpers::GetPlayerControllerFromObject(UObject* Object)
{
	if (Object == nullptr) return nullptr;
	
	// 如果对象本身就是PlayerController，直接返回
	if (APlayerController* PC = Cast<APlayerController>(Object))
	{
		return PC;
	}

	// 如果对象是PlayerState，通过其GetPlayerController方法获取
	if (const APlayerState* TargetPS = Cast<APlayerState>(Object))
	{
		return TargetPS->GetPlayerController();
	}

	// 如果对象是Pawn，获取其Controller并转换为PlayerController
	if (const APawn* TargetPawn = Cast<APawn>(Object))
	{
		return Cast<APlayerController>(TargetPawn->GetController());
	}

	// 如果对象是Actor，尝试从其Owner（如果是Pawn）获取PlayerController
	if (const AActor* TargetActor = Cast<AActor>(Object); APawn* Owner = Cast<APawn>(TargetActor->GetOwner()))
	{
		return Cast<APlayerController>(Owner->GetController());
	}

	return nullptr;
}

FGameplayCueParameters UYcGameVerbMessageHelpers::VerbMessageToCueParameters(const FYcGameVerbMessage& Message)
{
	FGameplayCueParameters Result;

	// 映射消息字段到GameplayCue参数
	Result.OriginalTag = Message.Verb;
	Result.Instigator = Cast<AActor>(Message.Instigator);
	Result.EffectCauser = Cast<AActor>(Message.Target);
	Result.AggregatedSourceTags = Message.InstigatorTags;
	Result.AggregatedTargetTags = Message.TargetTags;
	//@TODO: = Message.ContextTags;  // ContextTags暂未映射到GameplayCueParameters
	Result.RawMagnitude = Message.Magnitude;

	return Result;
}

FYcGameVerbMessage UYcGameVerbMessageHelpers::CueParametersToVerbMessage(const FGameplayCueParameters& Params)
{
	FYcGameVerbMessage Result;
	
	// 映射GameplayCue参数到消息字段
	Result.Verb = Params.OriginalTag;
	Result.Instigator = Params.Instigator.Get();
	Result.Target = Params.EffectCauser.Get();
	Result.InstigatorTags = Params.AggregatedSourceTags;
	Result.TargetTags = Params.AggregatedTargetTags;
	//@TODO: Result.ContextTags = ???;  // ContextTags暂未从GameplayCueParameters映射
	Result.Magnitude = Params.RawMagnitude;

	return Result;
}

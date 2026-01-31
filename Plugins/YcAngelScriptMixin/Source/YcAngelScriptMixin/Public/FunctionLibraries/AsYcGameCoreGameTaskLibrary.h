// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tasks/AbilityTask_PlayMontageForMeshAndWaitForEvent.h"
#include "Abilities/GameplayAbility.h"
#include "Actions/AsyncAction_PushContentToLayerForPlayer.h"
#include "UObject/Object.h"
#include "AsYcGameCoreGameTaskLibrary.generated.h"

/**
 * 
 */
UCLASS()
class YCANGELSCRIPTMIXIN_API UAsYcGameCoreGameTaskLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(ScriptCallable, Category = "Ability|Tasks",BlueprintCosmetic, meta=(WorldContext = "WorldContextObject"))
	static UAsyncAction_PushContentToLayerForPlayer* PushContentToLayerForPlayer(APlayerController* OwningPlayer,TSoftClassPtr<UCommonActivatableWidget> WidgetClass, FGameplayTag LayerName, bool bSuspendInputUntilComplete = true)
	{
		return UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(OwningPlayer, WidgetClass, LayerName, bSuspendInputUntilComplete);
	}

	/** 在avatar的特定tag骨骼网格体播放蒙太奇动画 */
	UFUNCTION(ScriptCallable, Category = "Ability|Tasks",BlueprintCosmetic, meta=(WorldContext = "WorldContextObject"))
	static UAbilityTask_PlayMontageForMeshAndWaitForEvent* PlayMontageOnSkeletalMeshAndWait(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		USkeletalMeshComponent* Mesh,
		FName MeshTag,
		UAnimMontage* MontageToPlay,
		const FGameplayTagContainer& EventTags = FGameplayTagContainer(),
		float Rate = 1.f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.f,
		bool bReplicateMontage = true,
		float OverrideBlendOutTimeForCancelAbility = -1.f,
		float OverrideBlendOutTimeForStopWhenEndAbility = -1.f
	)
	{
		return UAbilityTask_PlayMontageForMeshAndWaitForEvent::PlayMontageForMeshAndWaitForEvent(
			 OwningAbility,
			 TaskInstanceName,
			 Mesh,
			 MeshTag,
			 MontageToPlay,
			 EventTags,
			 Rate,
			 StartSection,
			 bStopWhenAbilityEnds,
			 AnimRootMotionTranslationScale,
			 bReplicateMontage,
			 OverrideBlendOutTimeForCancelAbility,
			 OverrideBlendOutTimeForStopWhenEndAbility
		);
	}

	// // @Dropped
	// /**
	//  * 激活AsyncAction,在AS中有些AsyncAction不会自动激活,可使用这个函数调用激活,但是需要注意处理多次激活的问题
	//  * @param AsyncAction 要激活的AsyncAction
	//  */
	// UFUNCTION(ScriptCallable, Category = "Ability|Tasks")
	// static void ActivateAsyncAction(UBlueprintAsyncActionBase* AsyncAction)
	// {
	// 	if(!IsValid(AsyncAction))
	// 	{
	// 		UE_LOG(LogTemp, Error, TEXT("UAsCommonGameTaskLibrary::ActivateAsyncAction->AsyncAction invalid."));
	// 		return;
	// 	}
	// 	AsyncAction->Activate();
	// }
};

// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/YcInteractionStatics.h"

#include "Engine/OverlapResult.h"
#include "Interaction/YcInteractableTarget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInteractionStatics)

UYcInteractionStatics::UYcInteractionStatics(): Super(FObjectInitializer::Get())
{
}

AActor* UYcInteractionStatics::GetActorFromInteractableTarget(TScriptInterface<IYcInteractableTarget> InteractableTarget)
{
	UObject* Object = InteractableTarget.GetObject();
	if (Object == nullptr) return nullptr;
	
	if (AActor* Actor = Cast<AActor>(Object))
	{
		return Actor;
	}
	else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
	{
		return ActorComponent->GetOwner();
	}
	else
	{
		unimplemented();
	}
	
	return nullptr;
}


void UYcInteractionStatics::GetInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IYcInteractableTarget>>& OutInteractableTargets)
{
	// 首先，检查Actor本身是否直接实现了交互接口。
	TScriptInterface<IYcInteractableTarget> InteractableActor(Actor);
	if (InteractableActor)
	{
		OutInteractableTargets.Add(InteractableActor);
	}
	
	// 其次，查找并添加所有实现了交互接口的组件。
	TArray<UActorComponent*> InteractableComponents = Actor ? Actor->GetComponentsByInterface(UYcInteractableTarget::StaticClass()) : TArray<UActorComponent*>();
	for (UActorComponent* InteractableComponent : InteractableComponents)
	{
		OutInteractableTargets.Add(TScriptInterface<IYcInteractableTarget>(InteractableComponent));
	}
}

void UYcInteractionStatics::AppendInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IYcInteractableTarget>>& OutInteractableTargets)
{
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		if (!Overlap.GetActor()) continue;
		// 检查重叠的Actor是否实现了交互接口。
		TScriptInterface<IYcInteractableTarget> InteractableActor(Overlap.GetActor());
		if (InteractableActor)
		{
			OutInteractableTargets.AddUnique(InteractableActor);
		}
		
		// 检查重叠的Actor中是否拥有实现了交互接口的组件。
		// 这通常用于查找如UYcInteractableComponent这样的组件。
		TScriptInterface<IYcInteractableTarget> InteractableComponent(Overlap.GetActor()->FindComponentByInterface(UYcInteractableTarget::StaticClass()));
		if (InteractableComponent)
		{
			OutInteractableTargets.AddUnique(InteractableComponent);
		}
		
		// 注释掉的代码：直接从OverlapResult中获取组件。当前逻辑不需要，因为它可能不是我们期望的交互组件。
		// TScriptInterface<IYcInteractableTarget> InteractableComponent(Overlap.GetComponent());
		// if (InteractableComponent)
		// {
		// 	OutInteractableTargets.AddUnique(InteractableComponent);
		// }
	}
}

void UYcInteractionStatics::GetInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IYcInteractableTarget>>& OutInteractableTargets)
{
	if (!HitResult.GetActor()) return;
	// 检查命中的Actor是否实现了交互接口。
	TScriptInterface<IYcInteractableTarget> InteractableActor(HitResult.GetActor());
	if (InteractableActor)
	{
		OutInteractableTargets.AddUnique(InteractableActor);
	}
	
	// 检查命中的Actor中是否拥有实现了交互接口的组件。
	// 这通常用于查找如UYcInteractableComponent这样的组件。
	TScriptInterface<IYcInteractableTarget> InteractableComponent(HitResult.GetActor()->FindComponentByInterface(UYcInteractableTarget::StaticClass()));
	if (InteractableComponent)
	{
		OutInteractableTargets.AddUnique(InteractableComponent);
	}
	
	// 注释掉的代码：直接从HitResult中获取组件。当前逻辑不需要，因为它可能不是我们期望的交互组件。
	// const TScriptInterface<IYcInteractableTarget> InteractableComponent(HitResult.GetComponent());
	// if (InteractableComponent)
	// {
	// 	OutInteractableTargets.AddUnique(InteractableComponent);
	// }
}
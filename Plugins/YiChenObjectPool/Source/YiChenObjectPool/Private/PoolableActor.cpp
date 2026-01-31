// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "PoolableActor.h"
#include "ObjectPoolSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolableActor)

APoolableActor::APoolableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	PoolState = EPoolableObjectState::Pooled;
	PoolIdentifier = NAME_None;
}

void APoolableActor::OnAcquiredFromPool()
{
	// 启用Actor
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	// 调用蓝图事件
	OnActivated();
}

void APoolableActor::OnReleasedToPool()
{
	// 调用蓝图事件
	OnDeactivated();

	// 禁用Actor
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void APoolableActor::OnRemovedFromPool()
{
	// 可在子类中重写以进行最终清理
}

void APoolableActor::ResetPooledObject()
{
	// 重置位置
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
	SetActorScale3D(FVector::OneVector);

	// 重置速度
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
		RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	// 调用蓝图事件
	OnReset();
}

void APoolableActor::RequestReturnToPool()
{
	if (PoolState != EPoolableObjectState::Active)
	{
		return;
	}

	PoolState = EPoolableObjectState::PendingRecycle;

	// 通过子系统释放
	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			PoolSubsystem->ReleaseObject(this);
		}
	}
}

void APoolableActor::OnActivated_Implementation()
{
	// 默认实现为空，子类可重写
}

void APoolableActor::OnDeactivated_Implementation()
{
	// 默认实现为空，子类可重写
}

void APoolableActor::OnReset_Implementation()
{
	// 默认实现为空，子类可重写
}

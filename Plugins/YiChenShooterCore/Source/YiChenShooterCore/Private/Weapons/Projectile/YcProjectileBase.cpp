// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Projectile/YcProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcProjectileBase)

AYcProjectileBase::AYcProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 网络复制设置
	bReplicates = true;
	// bNetLoadOnClient = false;
	SetReplicatingMovement(true);

	// 碰撞组件
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionComponent;

	// 投射物移动组件
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 10000.0f;
	ProjectileMovement->MaxSpeed = 15000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bAutoActivate = false;

	// 初始状态
	ProjectileState = EYcProjectileState::Pooled;
	CurrentDamage = 0.0f;
	ActivationTime = 0.0f;
	ConfiguredLifeSpan = 5.0f;
}

void AYcProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// 绑定碰撞事件
	if (CollisionComponent)
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AYcProjectileBase::OnProjectileOverlap);
		CollisionComponent->OnComponentHit.AddDynamic(this, &AYcProjectileBase::OnProjectileHitInternal);
	}

	// 初始状态为休眠
	SetProjectileActive(false);
}

void AYcProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ProjectileState == EYcProjectileState::Active)
	{
		CheckLifeSpan();
	}
}

void AYcProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AYcProjectileBase, ProjectileState, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AYcProjectileBase, CurrentDamage, COND_InitialOnly);
}

void AYcProjectileBase::ActivateFromPool(const FYcProjectileInitParams& Params)
{
	// 设置位置和方向
	SetActorLocation(Params.SpawnLocation);
	SetActorRotation(Params.Direction.Rotation());

	// 配置移动组件
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Params.InitialSpeed;
		ProjectileMovement->MaxSpeed = Params.MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = Params.bEnableGravity ? Params.GravityScale : 0.0f;
		ProjectileMovement->Velocity = Params.Direction * Params.InitialSpeed;
		ProjectileMovement->Activate(true);
	}

	// 设置伤害和发射者
	CurrentDamage = Params.Damage;
	CachedInstigator = Params.Instigator;
	CachedInstigatorController = Params.InstigatorController;
	SetInstigator(Cast<APawn>(Params.Instigator.Get()));

	// 设置存活时间
	ConfiguredLifeSpan = Params.LifeSpan;
	ActivationTime = GetWorld()->GetTimeSeconds();

	// 激活
	ProjectileState = EYcProjectileState::Active;
	SetProjectileActive(true);

	// 强制网络更新
	ForceNetUpdate();
}

void AYcProjectileBase::ReturnToPool()
{
	if (ProjectileState == EYcProjectileState::Pooled)
	{
		return;
	}

	ProjectileState = EYcProjectileState::PendingRecycle;
	
	// 广播回收事件
	OnProjectileRecycled.Broadcast(this);

	// 重置状态
	ResetProjectile();

	ProjectileState = EYcProjectileState::Pooled;
	SetProjectileActive(false);
}

void AYcProjectileBase::OnRep_ProjectileState()
{
	SetProjectileActive(ProjectileState == EYcProjectileState::Active);
}

void AYcProjectileBase::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ProjectileState != EYcProjectileState::Active)
	{
		return;
	}

	// 忽略发射者
	if (OtherActor == CachedInstigator.Get() || OtherActor == this)
	{
		return;
	}

	HandleHit(SweepResult, OtherActor);
}

void AYcProjectileBase::OnProjectileHitInternal(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ProjectileState != EYcProjectileState::Active)
	{
		return;
	}

	// 忽略发射者
	if (OtherActor == CachedInstigator.Get() || OtherActor == this)
	{
		return;
	}

	HandleHit(Hit, OtherActor);
}

void AYcProjectileBase::HandleHit_Implementation(const FHitResult& HitResult, AActor* HitActor)
{
	if (!HasAuthority())
	{
		return;
	}

	FYcProjectileHitResult ProjectileHitResult;
	ProjectileHitResult.HitResult = HitResult;

	// 应用伤害
	if (HitActor)
	{
		ProjectileHitResult.AppliedDamage = ApplyProjectileDamage(HitActor, HitResult);
		
		// 检测是否爆头（简单实现，可在子类中重写）
		if (HitResult.BoneName == TEXT("head"))
		{
			ProjectileHitResult.bWasHeadshot = true;
		}
	}

	// 广播命中事件
	OnProjectileHit.Broadcast(this, ProjectileHitResult);

	// 返回对象池
	ReturnToPool();
}

float AYcProjectileBase::ApplyProjectileDamage_Implementation(AActor* HitActor, const FHitResult& HitResult)
{
	// 基础实现 - 子类应重写以使用GAS或其他伤害系统
	return CurrentDamage;
}

void AYcProjectileBase::ResetProjectile()
{
	// 停止移动
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	// 清除引用
	CachedInstigator.Reset();
	CachedInstigatorController.Reset();
	SetInstigator(nullptr);

	// 重置数值
	CurrentDamage = 0.0f;
	ActivationTime = 0.0f;

	// 重置位置到原点（避免在远处渲染）
	SetActorLocation(FVector::ZeroVector);
	SetActorRotation(FRotator::ZeroRotator);
}

void AYcProjectileBase::SetProjectileActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	SetActorTickEnabled(bActive);

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
}

void AYcProjectileBase::CheckLifeSpan()
{
	if (ProjectileState != EYcProjectileState::Active)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - ActivationTime >= ConfiguredLifeSpan)
	{
		ReturnToPool();
	}
}

// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YcProjectileBase.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

/**
 * 子弹生命周期状态
 */
UENUM(BlueprintType)
enum class EYcProjectileState : uint8
{
	/** 在对象池中休眠 */
	Pooled,
	/** 已激活，正在飞行 */
	Active,
	/** 已命中目标，等待回收 */
	PendingRecycle
};

/**
 * 子弹初始化参数
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcProjectileInitParams
{
	GENERATED_BODY()

	/** 发射位置 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	FVector SpawnLocation = FVector::ZeroVector;

	/** 发射方向 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	FVector Direction = FVector::ForwardVector;

	/** 初始速度 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	float InitialSpeed = 10000.0f;

	/** 最大速度 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	float MaxSpeed = 15000.0f;

	/** 伤害值 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	float Damage = 10.0f;

	/** 子弹存活时间 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	float LifeSpan = 5.0f;

	/** 发射者 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	TWeakObjectPtr<AActor> Instigator;

	/** 发射者的Controller */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	TWeakObjectPtr<AController> InstigatorController;

	/** 是否启用重力 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	bool bEnableGravity = false;

	/** 重力缩放 */
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	float GravityScale = 1.0f;
};

/**
 * 子弹命中结果
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcProjectileHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	float AppliedDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	bool bWasHeadshot = false;
};

/** 子弹命中委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileHit, AYcProjectileBase*, Projectile, const FYcProjectileHitResult&, HitResult);
/** 子弹回收委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectileRecycled, AYcProjectileBase*, Projectile);

/**
 * 子弹Actor基类 - 支持对象池复用
 * 
 * 设计要点:
 * - 支持对象池的激活/休眠生命周期
 * - 网络复制优化，仅在必要时复制
 * - 可配置的物理和伤害参数
 */
UCLASS(Abstract, Blueprintable)
class YICHENSHOOTERCORE_API AYcProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AYcProjectileBase();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor Interface

	/**
	 * 从对象池激活子弹
	 * @param Params 初始化参数
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Pool")
	virtual void ActivateFromPool(const FYcProjectileInitParams& Params);

	/**
	 * 将子弹返回对象池
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Pool")
	virtual void ReturnToPool();

	/** 获取当前状态 */
	UFUNCTION(BlueprintPure, Category = "Projectile")
	EYcProjectileState GetProjectileState() const { return ProjectileState; }

	/** 是否可以从池中获取 */
	UFUNCTION(BlueprintPure, Category = "Projectile|Pool")
	bool IsAvailableInPool() const { return ProjectileState == EYcProjectileState::Pooled; }

	/** 获取池ID，用于多类型池管理 */
	UFUNCTION(BlueprintPure, Category = "Projectile|Pool")
	FName GetPoolID() const { return PoolID; }

	/** 设置池ID */
	void SetPoolID(FName InPoolID) { PoolID = InPoolID; }

public:
	/** 子弹命中事件 */
	UPROPERTY(BlueprintAssignable, Category = "Projectile|Events")
	FOnProjectileHit OnProjectileHit;

	/** 子弹回收事件 */
	UPROPERTY(BlueprintAssignable, Category = "Projectile|Events")
	FOnProjectileRecycled OnProjectileRecycled;

protected:
	/** 碰撞组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** 投射物移动组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** 当前状态 */
	UPROPERTY(ReplicatedUsing = OnRep_ProjectileState, BlueprintReadOnly, Category = "Projectile")
	EYcProjectileState ProjectileState;

	/** 池标识符 */
	UPROPERTY()
	FName PoolID;

	/** 当前伤害值 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Projectile")
	float CurrentDamage;

	/** 激活时间戳 */
	UPROPERTY()
	float ActivationTime;

	/** 配置的存活时间 */
	UPROPERTY()
	float ConfiguredLifeSpan;

	/** 发射者弱引用 */
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedInstigator;

	UPROPERTY()
	TWeakObjectPtr<AController> CachedInstigatorController;

protected:
	/** 状态复制回调 */
	UFUNCTION()
	void OnRep_ProjectileState();

	/** 碰撞回调 */
	UFUNCTION()
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnProjectileHitInternal(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** 处理命中逻辑 - 子类可重写 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile")
	void HandleHit(const FHitResult& HitResult, AActor* HitActor);

	/** 应用伤害 - 子类可重写 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile")
	float ApplyProjectileDamage(AActor* HitActor, const FHitResult& HitResult);

	/** 重置子弹状态 - 返回池时调用 */
	virtual void ResetProjectile();

	/** 设置子弹可见性和碰撞 */
	void SetProjectileActive(bool bActive);

	/** 检查存活时间 */
	void CheckLifeSpan();
};

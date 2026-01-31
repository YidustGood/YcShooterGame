// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameplayMessage/TestActor.h"

#include "GameplayCommon/ExperienceMessageTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TestActor)

// Sets default values
ATestActor::ATestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	ExperienceLoadedListener.Register(YcGameplayTags::Experience_StateEvent_Loaded_HighPriority, this, &ThisClass::OnExperienceLoaded);
	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(this);
            
	// // 使用你提供的成员函数版本 RegisterListener
	// ListenerHandle = Subsystem.RegisterListener(
	// 	YcGameplayTags::Experience_StateEvent_Loaded_HighPriority, this, &ThisClass::OnExperienceLoaded
	// );
}

void ATestActor::OnExperienceLoaded(FGameplayTag Channel, const FExperienceLoadedMessage& Message)
{
	UE_LOG(LogYcGameCore, Warning, TEXT("ATestActor::OnExperienceLoaded"));
	SetLifeSpan(5);
}

// Called every frame
void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


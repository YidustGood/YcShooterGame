// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScopedGameplayMessageListener.h"
#include "GameFramework/Actor.h"
#include "TestActor.generated.h"

struct FExperienceLoadedMessage;

UCLASS()
class YICHENGAMECORE_API ATestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void OnExperienceLoaded(FGameplayTag Channel, const FExperienceLoadedMessage& Message);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
private:
	TScopedGameplayMessageListener<FExperienceLoadedMessage, ThisClass> ExperienceLoadedListener;
	FGameplayMessageListenerHandle ListenerHandle;
};

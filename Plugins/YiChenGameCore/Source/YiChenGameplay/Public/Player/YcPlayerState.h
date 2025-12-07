// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "YcPlayerState.generated.h"

class AYcPlayerController;
class UYcAbilitySystemComponent;
class UYcPawnData;
class UYcExperienceDefinition;

/**
 * 插件Gameplay框架提供的玩家状态基类, 与插件其它模块系统协同工作
 * 如果我们会把ASC组件挂载在PlayerState上, 因为这样就算玩家角色死亡后也得以保留数据, 例如英雄联盟的Buff, 死亡后不会丢失,
 * 具体ASC该挂在哪个类下这个根据项目需求选择, 我们在这里提供了挂载ASC的支持。
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcPlayerState : public AModularPlayerState
{
	GENERATED_BODY()
	
public:
	AYcPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerState")
	AYcPlayerController* GetYcPlayerController() const;
	
	// 设置PawnData,设置成功后会将PawnData的AbilitySets授予这个Player State的ASC组件
	void SetPawnData(const UYcPawnData* InPawnData);
	
	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }
	
protected:
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface
	
private:
	// 当游戏体验加载完成时的回调
	void OnExperienceLoaded(const UYcExperienceDefinition* CurrentExperience);
	
protected:
	UFUNCTION()
	void OnRep_PawnData();
	
	// 玩家Pawn的数据包，当游戏体验加载完成后从体验定义实例中获取
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UYcPawnData> PawnData;
	
private:
	// @TODO 为PlayerState接入ASC
};

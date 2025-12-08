// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularCharacter.h"
#include "YcCharacter.generated.h"

class AYcPlayerController;
class AYcPlayerState;
class UYcAbilitySystemComponent;
class UYcPawnExtensionComponent;

/**
 * 本插件使用的基础角色Pawn类
 * 负责向Pawn组件派发事件, 新的功能行为应尽可能通过Pawn组件进行添加, 可参考Hero组件
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class YICHENGAMEPLAY_API AYcCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AYcCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Character")
	AYcPlayerController* GetYcPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Character")
	AYcPlayerState* GetYcPlayerState() const;
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Character")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcPawnExtensionComponent> PawnExtComponent;
};

class AInteractableActorBase : AActor
{
	UPROPERTY(DefaultComponent)
	UYcInteractableComponent InteractableComp;

	UPROPERTY(DefaultComponent)
	UStaticMeshComponent DisplayMeshComp;

	// 要配置具体的InteractableComp.Option,来影响Actor的交互表现
	// InteractableComp.Option.InteractionAbilityToGrant - 交互时激活的技能, 可以通过GA组织不同的交互效果
	// InteractableComp.Option.InteractionWidgetClass - 视角焦点看向这个交互物时显示的UI类

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		InteractableComp.OnPlayerFocusBeginEvent.AddUFunction(this, n"OnPlayerFocusBegin");
		InteractableComp.OnPlayerFocusEndEvent.AddUFunction(this, n"OnPlayerFocusEnd");
	}

	UFUNCTION(BlueprintEvent)
	void OnPlayerFocusBegin(const FYcInteractionQuery&in InteractQuery)
	{
		// 玩家注视到这个交互物体时要做的事情, 蓝图子类可重写
	}

	UFUNCTION(BlueprintEvent)
	void OnPlayerFocusEnd(const FYcInteractionQuery&in InteractQuery)
	{
		// 玩家取消注视这个交互物体时要做的事情, 蓝图子类可重写
	}
}
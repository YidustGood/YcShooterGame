enum EYcAbilityPressActiveMode
{
	Hold,  // 持续按住激活技能
	Toggle // 切换激活技能
}

/**
 * 基于按键按住或者按键切换驱动的技能基类
 */
class UYcGameplayAbility_HoldToggle : UYcGameplayAbility
{

	//@TODO 这个值可以与输入按键设置系统做集成关联, 以实现用户自由设置按住/切换的输入模式
	EYcAbilityPressActiveMode AbilityPressActiveMode;
	default AbilityPressActiveMode = EYcAbilityPressActiveMode::Toggle;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		OnAbilityAddedEvent();
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		/**
		 * 监听GA的输入释放事件
		 * 还需要注意UI的InputConfig模型需要包含Game,否则UI出现会导致按钮被释放,从而导致UI一显示就马上关闭显示
		 */
		if (AbilityPressActiveMode == EYcAbilityPressActiveMode::Hold)
		{
			auto WaitInputRelease = AngelscriptAbilityTask::WaitInputRelease(this);
			WaitInputRelease.OnRelease.AddUFunction(this, n"OnRelease");
			WaitInputRelease.ReadyForActivation();
		}
		else
		{
			auto WaitInputPress = AngelscriptAbilityTask::WaitInputPress(this);
			WaitInputPress.OnPress.AddUFunction(this, n"OnPress");
			WaitInputPress.ReadyForActivation();
		}
		OnActivateAbilityEvent();
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		OnEndAbilityEvent();
	}

	UFUNCTION()
	private void OnPress(float32 TimeWaited)
	{
		EndAbility();
	}

	// 松开按钮后结束技能
	UFUNCTION()
	private void OnRelease(float32 TimeHeld)
	{
		EndAbility();
	}

	// 因为AS对于原生C++函数父类调用存在限制所以采用以下形式来复制实现在子类处理相应的事件,而非直接在子类重写
	UFUNCTION(BlueprintEvent)
	void OnAbilityAddedEvent()
	{
		// 供蓝图/脚本重写扩展功能
	}

	UFUNCTION(BlueprintEvent)
	void OnAbilityRemovedEvent()
	{
		// 供蓝图/脚本重写扩展功能
	}

	UFUNCTION(BlueprintEvent)
	void OnEndAbilityEvent()
	{
		// 供蓝图/脚本重写扩展功能
	}

	UFUNCTION(BlueprintEvent)
	void OnActivateAbilityEvent()
	{
		// 供蓝图/脚本重写扩展功能
	}
}
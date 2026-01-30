/**
 * 显示 Widget 的游戏技能基类
 *
 * 功能说明：
 * - 基于按键按住或切换模式驱动的技能
 * - 激活时显示指定的 Widget（如计分板、菜单等）
 * - 结束时自动移除 Widget 并恢复输入模式
 * - 支持显示鼠标光标和设置输入模式
 *
 * 继承关系：
 * UYcGameplayAbility_HoldToggle（按住/切换技能基类）
 *   └─ UYcGameplayAbility_ShowWidget（显示 Widget 技能）
 *
 * 典型应用场景：
 * - 按 Tab 键显示计分板（按住模式）
 * - 按 Esc 键显示暂停菜单（切换模式）
 * - 按 M 键显示地图界面（切换模式）
 * - 按 I 键显示背包界面（切换模式）
 *
 * 使用方法：
 * 1. 创建子类继承此类
 * 2. 设置 WidgetClass 为要显示的 Widget 类, 这个类最好继承自UYcActivatableWidget, 可以很好的配置处理输入问题
 * 3. 配置 LayerName（UI 层级）
 * 4. 配置是否显示鼠标光标
 * 5. 绑定到输入系统
 */
class UYcGameplayAbility_ShowWidget : UYcGameplayAbility_HoldToggle
{
	// ========================================
	// 配置属性
	// ========================================

	/**
	 * 要显示的 Widget 类
	 * 使用软引用避免硬引用导致的资源加载问题
	 */
	UPROPERTY()
	TSoftClassPtr<UCommonActivatableWidget> WidgetClass;

	/**
	 * UI 层级名称
	 * 决定 Widget 显示在哪个 UI 层
	 *
	 * 常用层级：
	 * - UI.Layer.GameMenu：游戏菜单层（默认）, 计分板这些UI都是用这个层级
	 * - UI.Layer.Modal：模态对话框层
	 * - UI.Layer.Game：游戏 HUD 层
	 */
	UPROPERTY()
	FGameplayTag LayerName;

	/**
	 * 生成的 Widget 实例
	 * 用于跟踪当前显示的 Widget，以便后续移除
	 * 注意：不可编辑，由系统自动管理
	 */
	UPROPERTY(NotEditable)
	UCommonActivatableWidget SpawnedWidget;

	/**
	 * 是否显示鼠标光标
	 * true：激活技能时显示鼠标（如菜单、背包）
	 * false：不显示鼠标（如计分板）
	 */
	UPROPERTY()
	bool bShowMouseCursor;

	/**
	 * 激活时的鼠标锁定模式
	 * 仅在 bShowMouseCursor 为 true 时生效
	 *
	 * 可选值：
	 * - DoNotLock：不锁定鼠标
	 * - LockOnCapture：捕获时锁定
	 * - LockAlways：始终锁定
	 * - LockInFullscreen：全屏时锁定
	 */
	UPROPERTY()
	EMouseLockMode ActivateMouseLockMode;

	// ========================================
	// 内部变量
	// ========================================

	/**
	 * 异步推送 UI 的操作句柄
	 * 用于跟踪 UI 加载和显示的异步操作
	 * 如果技能被取消，可以通过此句柄取消异步操作
	 */
	private UAsyncAction_PushContentToLayerForPlayer AsyncAction_PushUI;

	// ========================================
	// 默认配置
	// ========================================

	/** 默认不显示鼠标光标 */
	default bShowMouseCursor = false;

	/** 默认使用游戏菜单层 */
	default LayerName = GameplayTags::UI_Layer_GameMenu;

	/** 仅在本地执行（UI 不需要网络同步） */
	default NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	/** 独占阻塞模式（激活时阻止其他技能） */
	default ActivationGroup = EYcAbilityActivationGroup::Exclusive_Blocking;

	// ========================================
	// 生命周期事件
	// ========================================

	/**
	 * 技能激活时调用
	 *
	 * 执行流程：
	 * 1. 调用父类的激活逻辑
	 * 2. 启动技能状态（监听输入结束）
	 * 3. 创建并显示 Widget
	 *
	 * 注意：
	 * - 按住模式：松开按键时会调用 RemoveWidget
	 * - 切换模式：再次按键时会调用 RemoveWidget
	 */
	UFUNCTION(BlueprintOverride)
	void OnActivateAbilityEvent()
	{
		Super::OnActivateAbilityEvent();

		// 启动技能状态，监听输入结束和中断事件
		auto StartAbilityState = AngelscriptAbilityTask::StartAbilityState(this, n"ShowWidget_WhenInputHeld");
		StartAbilityState.OnStateEnded.AddUFunction(this, n"RemoveWidget");
		StartAbilityState.OnStateInterrupted.AddUFunction(this, n"RemoveWidget");
		StartAbilityState.ReadyForActivation();

		// 创建并显示 Widget
		CreateAndDisplayWidget();
	}

	// ========================================
	// Widget 管理
	// ========================================

	/**
	 * 创建并显示 Widget
	 *
	 * 执行流程：
	 * 1. 检查是否已有 Widget 实例（避免重复创建）
	 * 2. 获取玩家控制器
	 * 3. 异步加载并推送 Widget 到指定 UI 层
	 * 4. 等待 Widget 创建完成后调用 AfterPush
	 *
	 * 注意：
	 * - 使用异步加载，不会阻塞游戏
	 * - 如果 WidgetClass 未设置，会失败
	 */
	UFUNCTION(BlueprintEvent)
	void CreateAndDisplayWidget()
	{
		// 如果已有 Widget 实例，直接返回（避免重复创建）
		if (IsValid(SpawnedWidget))
			return;

		// 获取玩家控制器
		auto PC = Cast<APlayerController>(GetControllerFromActorInfo());

		// 异步推送 Widget 到 UI 层
		AsyncAction_PushUI = AsYcGameCoreGameTask::PushContentToLayerForPlayer(PC, WidgetClass, LayerName, true);
		AsyncAction_PushUI.AfterPush.AddUFunction(this, n"AfterPush");
		AsyncAction_PushUI.Activate();
	}

	/**
	 * Widget 创建完成后的回调
	 *
	 * @param UserWidget 创建的 Widget 实例
	 *
	 * 执行流程：
	 * 1. 保存 Widget 实例引用
	 * 2. 清理异步操作句柄
	 * 3. 如果需要显示鼠标，设置鼠标可见性
	 * 4. 如果需要显示鼠标，设置输入模式为"游戏和UI"
	 */
	UFUNCTION()
	void AfterPush(UCommonActivatableWidget UserWidget)
	{
		// 保存 Widget 实例
		SpawnedWidget = UserWidget;

		// 清理异步操作句柄
		AsyncAction_PushUI = nullptr;

		// 获取玩家控制器
		auto PC = Cast<APlayerController>(GetControllerFromActorInfo());

		// 如果需要显示鼠标，设置鼠标可见
		if (bShowMouseCursor)
			PC.bShowMouseCursor = bShowMouseCursor;

		// 如果需要显示鼠标，设置输入模式为"游戏和UI"
		// 这样玩家可以同时接收游戏输入和 UI 输入
		if (bShowMouseCursor)
		{
			Widget::SetInputMode_GameAndUIEx(PC, SpawnedWidget, ActivateMouseLockMode);
		}
	}

	/**
	 * 移除 Widget
	 *
	 * 调用时机：
	 * - 技能结束时（按键松开或再次按下）
	 * - 技能被中断时
	 * - 技能被取消时
	 *
	 * 执行流程：
	 * 1. 取消正在进行的异步加载操作（如果有）
	 * 2. 停用并移除 Widget
	 * 3. 如果显示了鼠标，恢复鼠标隐藏
	 * 4. 恢复输入模式为"仅游戏"
	 */
	UFUNCTION()
	void RemoveWidget()
	{
		// 如果异步加载还在进行，取消它
		if (IsValid(AsyncAction_PushUI))
		{
			AsyncAction_PushUI.Cancel();
		}

		// 如果 Widget 已创建，停用并移除它
		if (IsValid(SpawnedWidget))
		{
			SpawnedWidget.DeactivateWidget();
			SpawnedWidget = nullptr;
		}

		// 如果显示了鼠标，恢复鼠标隐藏和输入模式
		if (bShowMouseCursor)
		{
			auto PC = Cast<APlayerController>(GetControllerFromActorInfo());
			PC.bShowMouseCursor = false;
			Widget::SetInputMode_GameOnly(PC);
		}
	}
}
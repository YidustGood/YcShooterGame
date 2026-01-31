/**
 * 通过GameplayCueNotify创建Widget的基类
 * 通过在蓝图种实现子类然后配置WidgetToSpawn和GameplayCueTag来使用
 * 记得统一放到GameplayCueNotifies目录下, 并且确保在项目设置中配置了这个Cue的扫描路径
 * 如果是放到GameFeaature的GameplayCueNotifies目录下的话可以用GFA_AddGameplayCuePaths来添加路径
 */
class AGameplayCueNotify_WidgetBase : AGameplayCueNotify_Looping
{
	/** 要创建的Widget类型 */
	UPROPERTY()
	TSubclassOf<UUserWidget> WidgetToSpawn;

	UPROPERTY(VisibleInstanceOnly)
	private APlayerController OptionalLocalController;

	/** 创建出来的Widget实例对象, 用于移除 */
	UPROPERTY(VisibleInstanceOnly)
	UUserWidget SpawnedWidget;

	UFUNCTION(BlueprintOverride)
	void OnLoopingStart(AActor Target, FGameplayCueParameters Parameters,
						FGameplayCueNotify_SpawnResult SpawnResults)
	{
		if (IsValid(Parameters.Instigator))
		{
			APlayerState PS = Cast<APlayerState>(Parameters.Instigator);
			if (IsValid(PS) && !PS.IsABot() && IsValid(PS.GetPlayerController()))
			{
				if (PS.GetPlayerController().IsLocalController())
				{
					OptionalLocalController = PS.GetPlayerController();
					CueCreateWdiget(Parameters);
					return;
				}
			}
			UnableToDetermineLocalPlayer(Parameters.Instigator);
			CueCreateWdiget(Parameters);
		}
		else
		{
			OptionalLocalController = nullptr;
			CueCreateWdiget(Parameters);
		}
	}

	UFUNCTION(BlueprintOverride)
	void OnRemoval(AActor Target, FGameplayCueParameters Parameters,
				   FGameplayCueNotify_SpawnResult SpawnResults)
	{
		if (SpawnedWidget == nullptr)
			return;
		SpawnedWidget.RemoveFromParent();
	}

	void UnableToDetermineLocalPlayer(AActor InInstigator)
	{
		Log(f"AGameplayCueNotify_WidgetBase: Unable to determine local player for widget from Instigator:  {InInstigator.GetName()}");
	}

	// @TOOD 目前必须在蓝图重写然后调用父函数然后添加上ConstructFromCueParams的接口调用传入SpawnedWidget和Parameters, 为了传递数据到UI, 因为AS不支持接口, 后续可以做一个C++工具函数来中转处理
	UFUNCTION(BlueprintEvent)
	void CueCreateWdiget(FGameplayCueParameters Parameters)
	{
		SpawnedWidget = WidgetBlueprint::CreateWidget(WidgetToSpawn, OptionalLocalController);
		SpawnedWidget.AddToPlayerScreen();
	}
}
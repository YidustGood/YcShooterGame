/** 击杀消息条目过期委托 */
event void FToastExpiredAction(UObject ItemObject);
/**
 * UEliminationBroadcastEntryWidget
 * 击杀反馈消息条目UI
 * 一定要在蓝图控件类设置中实现UserObjectListEntry接口, 然后实现OnListItemObjectSet函数调用OnEliminationFeedMessageSet完成数据正确传递
 */
class UEliminationBroadcastEntryWidget : UUserWidget
{
	UPROPERTY(BindWidgetAnim)
	UWidgetAnimation IntroWithTimedOutro;

	UPROPERTY(BindWidget)
	UCommonTextBlock AttackerName;

	UPROPERTY(BindWidget)
	UImage AttackerWeaponIcon;

	UPROPERTY(BindWidget)
	UCommonTextBlock AttackeeName;

	UPROPERTY()
	float ShowTime = 3;

	// 运行时内部数据
	int32 AttackerTeamId;
	int32 AttackeeTeamId;
	UElimFeedEntry ElimFeedEntry;

	FToastExpiredAction OnToastExpiredEvent;

	UFUNCTION()
	protected void OnEliminationFeedMessageSet(UObject ElimFeedEntryObj)
	{
		ElimFeedEntry = Cast<UElimFeedEntry>(ElimFeedEntryObj);
		if (ElimFeedEntry == nullptr)
		{
			Warning("UEliminationBroadcastEntryWidget::OnEliminationFeedMessageSet: 击杀反馈消息对象无效");
			return;
		}
		AttackerTeamId = ElimFeedEntry.EliminationFeedMessage.AttackerTeamId;
		AttackeeTeamId = ElimFeedEntry.EliminationFeedMessage.AttackeeTeamId;
		SetTeamColor();
		SetWeaponIconAndNameText();

		// 播放消息框显示动画
		PlayAnimation(IntroWithTimedOutro);

		// 设置自动过期小时
		System::SetTimer(this, n"OnToastExpired", ShowTime, false);

		if (IsOwningPlayerElim())
		{
			// @TODO 如果是当前玩家击杀的消息那么可以再这里做特殊效果处理
		}
	}

	UFUNCTION()
	void SetTeamColor()
	{
		// 这里就把与自己相关的击杀通报中自己的名称设置为蓝色, 敌人的设置为红色
		if (GetOwningPlayer().PlayerState.GetPlayerName() == ElimFeedEntry.EliminationFeedMessage.Attacker.ToString())
		{
			AttackerName.ColorAndOpacity = FSlateColor(FLinearColor::Blue);
			AttackeeName.ColorAndOpacity = FSlateColor(FLinearColor::Red);
		}
		else if (GetOwningPlayer().PlayerState.GetPlayerName() == ElimFeedEntry.EliminationFeedMessage.Attackee.ToString())
		{
			AttackerName.ColorAndOpacity = FSlateColor(FLinearColor::Red);
			AttackeeName.ColorAndOpacity = FSlateColor(FLinearColor::Blue);
		}
	}

	void SetWeaponIconAndNameText()
	{
		AttackerName.SetText(ElimFeedEntry.EliminationFeedMessage.Attacker);
		AttackeeName.SetText(ElimFeedEntry.EliminationFeedMessage.Attackee);

		// @TODO 处理击杀武器图片, InstigatorTags目前设计下代指的是造成击杀的对象tag, 例如某把武器, 这个后续其实可以再想一下重新设计
		// ElimFeedEntry.EliminationFeedMessage.InstigatorTags
	}

	/** 检查是否为当前玩家的击杀消息, 可以通过这个做特殊显示 */
	bool IsOwningPlayerElim()
	{
		return GetOwningPlayer().PlayerState.GetPlayerName() == ElimFeedEntry.EliminationFeedMessage.Attacker.ToString();
	}

	UFUNCTION(NotBlueprintCallable)
	protected void OnToastExpired()
	{
		OnToastExpiredEvent.Broadcast(ElimFeedEntry);
	}
}
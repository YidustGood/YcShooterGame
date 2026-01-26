/**
 *  UEliminationFeedRelayComponent
 *  击杀反馈消息网络转发组件, 挂载在AYcGameState上, 以便组件自动将击杀消息转发到客户端
 *  客户端通过监听Yc.Elimination.Message消息可以处理击杀反馈、通知之类的操作
 *  客户端要想收到消灭/击杀敌人消息这个组件必须挂载在AYcGameState上
 */
class UEliminationFeedRelayComponent : UActorComponent
{
	// AS脚本RegisterListener返回的handler必须在EndPlay或者合适的时机被清理掉,否则会引起崩溃!!!
	FGameplayMessageListenerHandle EliminationListenerHandle;
	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// Log(f"Add UEliminationFeedRelayComponent: {this.GetName()}->Owner->{GetOwner().GetName()}");

		// 原始消息在UYcShooterHealthComponent::HandleOutOfHealth()中广播的, 这只会在服务端广播
		// 服务端首先受到消息后在OnReceivedEliminationMessage中通过GameState转发消息到所有客户端
		// 然后客户端就可以正确的受到消息进行处理了
		EliminationListenerHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Yc.Elimination.Message"),
			this,
			n"OnReceivedEliminationMessage",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason EndPlayReason)
	{
		// 必须要在结束的时候进行清理否则会出现一些问题
		EliminationListenerHandle.Unregister();
	}

	UFUNCTION()
	private void OnReceivedEliminationMessage(FGameplayTag ActualTag, FYcGameVerbMessage Data)
	{
		// Log(f"{GetWorld().GetNetMode():n}OnReceivedEliminationMessage:{ActualTag.ToString()}");

		auto Instigator = Cast<APlayerState>(Data.Instigator);
		auto InstigatorName = FText::FromString(Instigator.GetPlayerName());
		auto Target = Cast<APlayerState>(Data.Target);
		auto TargetName = FText::FromString(Target.GetPlayerName());
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		int AttackerTeamId = -1;
		int AttackeeTeamId = -1;

		if (GetOwner().HasAuthority())
		{
			AYcGameState GameState = Cast<AYcGameState>(Gameplay::GetGameState());
			if (GameState != nullptr)
			{
				GameState.MulticastReliableMessageToClients(Data);
				// 在非dedicated server上执行
				if (!System::IsDedicatedServer())
				{
					bool bIsPartOfTeam;
					TeamSubsystem.FindTeamFromActor(Instigator.GetPawn(), bIsPartOfTeam, AttackerTeamId);
				}
			}
		}

		bool bIsPartOfTeam_Attacker;
		TeamSubsystem.FindTeamFromActor(Instigator.GetPawn(), bIsPartOfTeam_Attacker, AttackerTeamId);

		bool bIsPartOfTeam_Attackee;
		TeamSubsystem.FindTeamFromActor(Target.GetPawn(), bIsPartOfTeam_Attackee, AttackeeTeamId);
		FEliminationFeedMessage EliminationFeedMessage;
		EliminationFeedMessage.Attacker = InstigatorName;
		EliminationFeedMessage.Attackee = TargetName;
		EliminationFeedMessage.TargetChannel = GameplayTags::HUD_Slot_EliminationFeed;
		EliminationFeedMessage.AttackerTeamId = AttackerTeamId;
		EliminationFeedMessage.AttackeeTeamId = AttackeeTeamId;
		EliminationFeedMessage.InstigatorTags = Data.InstigatorTags;

		// 广播添加击杀反馈消息
		UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::Yc_AddNotification_KillFeed, EliminationFeedMessage);
	}
}

// 消灭反馈消息
struct FEliminationFeedMessage
{
	UPROPERTY()
	FText Attacker; // 攻击者名称
	UPROPERTY()
	FText Attackee; // 被攻击者名称
	UPROPERTY()
	FGameplayTag TargetChannel;
	UPROPERTY()
	APlayerState TargetPlayer;
	UPROPERTY()
	int AttackerTeamId;
	UPROPERTY()
	FGameplayTagContainer InstigatorTags;
	UPROPERTY()
	int AttackeeTeamId;
}

// 用于ListView子项的包装Object, 包装了击杀信息FEliminationFeedMessage
class UElimFeedEntry : UObject
{
	UPROPERTY()
	FEliminationFeedMessage EliminationFeedMessage;
}
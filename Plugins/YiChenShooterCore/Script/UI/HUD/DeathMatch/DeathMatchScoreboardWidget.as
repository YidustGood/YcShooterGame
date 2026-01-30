/**
 * 个人死亡竞赛计分板 Widget
 *
 * 功能说明：
 * - 显示所有玩家的数据列表
 * - 数据包括：玩家名称、淘汰数、被淘汰数、延迟(Ping)
 * - 按击杀数从高到低排序
 * - 支持高亮本地玩家
 *
 * 使用方法：
 * 1. 在 UMG 中创建 Widget 蓝图，继承此类
 * 2. 创建以下 UI 元素并绑定：
 *    - PlayerListBox (UScrollBox) - 玩家列表容器
 *    - PlayerEntryClass (TSubclassOf<UUserWidget>) - 玩家条目 Widget 类
 * 3. 创建玩家条目 Widget（见 DeathMatchScoreboardEntry）
 * 4. 在游戏中通过按键显示/隐藏此 Widget
 */
class UDeathMatchScoreboardWidget : UYcActivatableWidget
{
	// ========================================
	// UI 元素绑定
	// ========================================

	/** 玩家列表容器 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UScrollBox PlayerListBox;

	// ========================================
	// 配置属性
	// ========================================

	/** 玩家条目 Widget 类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	TSubclassOf<UUserWidget> PlayerEntryClass;

	/** 更新间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	float UpdateInterval = 0.5f;

	/** 本地玩家高亮颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	FLinearColor LocalPlayerHighlightColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.3f);

	// ========================================
	// 内部变量
	// ========================================

	private float AccumulatedTime = 0.0f;
	private AYcGameState CachedGameState;
	private AYcPlayerState CachedLocalPlayerState;

	/** 玩家条目缓存池（用于复用，避免频繁创建销毁） */
	private TArray<UUserWidget> EntryPool;

	/** 当前使用的条目数量 */
	private int ActiveEntryCount = 0;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		CachedGameState = Cast<AYcGameState>(Gameplay::GetGameState());
		CachedLocalPlayerState = GetLocalPlayerState();

		// 初始化显示
		UpdateScoreboard();
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		AccumulatedTime += InDeltaTime;

		if (AccumulatedTime >= UpdateInterval)
		{
			AccumulatedTime = 0.0f;
			UpdateScoreboard();
		}
	}

	// ========================================
	// 更新逻辑
	// ========================================

	/**
	 * 更新计分板
	 */
	private void UpdateScoreboard()
	{
		if (PlayerListBox == nullptr)
		{
			return;
		}

		// 确保缓存有效
		if (CachedGameState == nullptr)
		{
			CachedGameState = Cast<AYcGameState>(Gameplay::GetGameState());
		}

		if (CachedLocalPlayerState == nullptr)
		{
			CachedLocalPlayerState = GetLocalPlayerState();
		}

		if (CachedGameState == nullptr)
		{
			return;
		}

		// 获取排序后的玩家列表
		TArray<AYcPlayerState> SortedPlayers = GetSortedPlayers();

		// 更新显示
		UpdatePlayerEntries(SortedPlayers);
	}

	/**
	 * 获取按击杀数排序的玩家列表
	 */
	private TArray<AYcPlayerState> GetSortedPlayers() const
	{
		if (CachedGameState == nullptr)
		{
			TArray<AYcPlayerState> EmptyArray;
			return EmptyArray;
		}

		// 使用 C++ 层的高效排序方法
		// 比手动冒泡排序快得多（O(n log n) vs O(n²)）
		return CachedGameState.GetPlayersSortedByTag(
			GameplayTags::ShooterGame_Score_Eliminations,
			true // 降序排列（从高到低）
		);
	}

	/**
	 * 更新玩家条目显示
	 */
	private void UpdatePlayerEntries(const TArray<AYcPlayerState>& SortedPlayers)
	{
		if (PlayerEntryClass == nullptr)
		{
			Warning("[DeathMatchScoreboardWidget] PlayerEntryClass 未设置");
			return;
		}

		int PlayerCount = SortedPlayers.Num();
		ActiveEntryCount = 0;

		// 为每个玩家创建或更新条目
		for (int i = 0; i < PlayerCount; i++)
		{
			auto PlayerState = SortedPlayers[i];
			if (PlayerState == nullptr)
			{
				continue;
			}

			// 获取或创建条目 Widget
			UUserWidget EntryWidget = GetOrCreateEntry(i);
			if (EntryWidget == nullptr)
			{
				continue;
			}

			// 更新条目数据
			UpdateEntryData(EntryWidget, PlayerState, i + 1);

			ActiveEntryCount++;
		}

		// 隐藏多余的条目
		HideUnusedEntries();
	}

	/**
	 * 获取或创建条目 Widget
	 */
	private UUserWidget GetOrCreateEntry(int Index)
	{
		// 如果池中已有，直接复用
		if (Index < EntryPool.Num())
		{
			UUserWidget Entry = EntryPool[Index];
			if (Entry != nullptr)
			{
				Entry.SetVisibility(ESlateVisibility::Visible);
				return Entry;
			}
		}

		// 创建新条目
		auto PlayerController = Gameplay::GetPlayerController(0);
		if (PlayerController == nullptr)
		{
			return nullptr;
		}

		UUserWidget NewEntry = WidgetBlueprint::CreateWidget(PlayerEntryClass, PlayerController);
		if (NewEntry != nullptr)
		{
			PlayerListBox.AddChild(NewEntry);
			EntryPool.Add(NewEntry);
		}

		return NewEntry;
	}

	/**
	 * 更新条目数据
	 */
	private void UpdateEntryData(UUserWidget EntryWidget, AYcPlayerState PlayerState, int Rank)
	{
		// 获取玩家数据
		FString PlayerName = PlayerState.GetPlayerName();
		int Kills = PlayerState.GetStatTagStackCount(GameplayTags::ShooterGame_Score_Eliminations);
		int Deaths = PlayerState.GetStatTagStackCount(GameplayTags::ShooterGame_Score_Deaths);
		float Ping = PlayerState.GetPingInMilliseconds();

		// 判断是否是本地玩家
		bool bIsLocalPlayer = (PlayerState == CachedLocalPlayerState);

		// 调用条目的更新方法（需要在条目 Widget 中实现）
		// 使用蓝图可实现事件
		auto EntryBP = Cast<UDeathMatchScoreboardEntry>(EntryWidget);
		if (EntryBP != nullptr)
		{
			EntryBP.UpdateData(Rank, PlayerName, Kills, Deaths, int(Ping), bIsLocalPlayer);
		}
	}

	/**
	 * 隐藏未使用的条目
	 */
	private void HideUnusedEntries()
	{
		for (int i = ActiveEntryCount; i < EntryPool.Num(); i++)
		{
			if (EntryPool[i] != nullptr)
			{
				EntryPool[i].SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	/**
	 * 获取本地玩家的 PlayerState
	 */
	private AYcPlayerState GetLocalPlayerState() const
	{
		auto PlayerController = Gameplay::GetPlayerController(0);
		if (PlayerController == nullptr)
		{
			return nullptr;
		}

		return Cast<AYcPlayerState>(PlayerController.PlayerState);
	}

	// ========================================
	// 公共接口
	// ========================================

	/**
	 * 手动刷新计分板
	 */
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void RefreshScoreboard()
	{
		UpdateScoreboard();
	}

	/**
	 * 清空计分板
	 */
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void ClearScoreboard()
	{
		if (PlayerListBox != nullptr)
		{
			PlayerListBox.ClearChildren();
		}
		EntryPool.Empty();
		ActiveEntryCount = 0;
	}

	/**
	 * 设置更新间隔
	 */
	UFUNCTION(BlueprintCallable, Category = "Scoreboard")
	void SetUpdateInterval(float NewInterval)
	{
		UpdateInterval = Math::Max(0.1f, NewInterval);
	}
}

/**
 * 计分板玩家条目 Widget
 *
 * 这是一个基类，需要在 UMG 中创建蓝图继承此类
 * 并实现 UpdateData 方法来更新 UI 显示
 */
class UDeathMatchScoreboardEntry : UUserWidget
{
	// ========================================
	// UI 元素绑定（在子类蓝图中绑定）
	// ========================================

	/** 排名文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock RankText;

	/** 玩家名称文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock PlayerNameText;

	/** 击杀数文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock KillsText;

	/** 死亡数文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock DeathsText;

	/** 延迟文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock PingText;

	/** 背景图片（用于高亮本地玩家） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage BackgroundImage;

	// ========================================
	// 配置属性
	// ========================================

	/** 本地玩家高亮颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
	FLinearColor LocalPlayerColor = FLinearColor(1.0f, 1.0f, 0.0f, 0.3f);

	/** 普通玩家背景颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
	FLinearColor NormalPlayerColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);

	// ========================================
	// 更新方法
	// ========================================

	/**
	 * 更新条目数据
	 *
	 * @param Rank 排名
	 * @param PlayerName 玩家名称
	 * @param Kills 击杀数
	 * @param Deaths 死亡数
	 * @param Ping 延迟（毫秒）
	 * @param bIsLocalPlayer 是否是本地玩家
	 */
	UFUNCTION(BlueprintCallable, Category = "Entry")
	void UpdateData(int Rank, FString PlayerName, int Kills, int Deaths, int Ping, bool bIsLocalPlayer)
	{
		// 更新排名
		if (RankText != nullptr)
		{
			RankText.SetText(FText::FromString(f"{Rank}"));
		}

		// 更新玩家名称
		if (PlayerNameText != nullptr)
		{
			PlayerNameText.SetText(FText::FromString(PlayerName));
		}

		// 更新击杀数
		if (KillsText != nullptr)
		{
			KillsText.SetText(FText::FromString(f"{Kills}"));
		}

		// 更新死亡数
		if (DeathsText != nullptr)
		{
			DeathsText.SetText(FText::FromString(f"{Deaths}"));
		}

		// 更新延迟
		if (PingText != nullptr)
		{
			PingText.SetText(FText::FromString(f"{Ping}ms"));
		}

		// 高亮本地玩家
		if (BackgroundImage != nullptr)
		{
			if (bIsLocalPlayer)
			{
				BackgroundImage.SetColorAndOpacity(LocalPlayerColor);
			}
			else
			{
				BackgroundImage.SetColorAndOpacity(NormalPlayerColor);
			}
		}
	}
}

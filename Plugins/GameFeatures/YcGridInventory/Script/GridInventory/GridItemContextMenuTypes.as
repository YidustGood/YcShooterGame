USTRUCT()
struct FGridItemContextMenuOpenMessage
{
	UPROPERTY()
	UYcInventoryItemInstance ItemInst;

	UPROPERTY()
	FVector2D ScreenPosition;
}

USTRUCT()
struct FGridItemContextMenuClickMessage
{
	UPROPERTY()
	UYcInventoryItemInstance ItemInst;

	UPROPERTY()
	FGameplayTag ActionTag;

	UPROPERTY()
	bool bCloseMenuOnExecute = true;
}

USTRUCT()
struct FGridItemContextMenuCloseMessage
{
	UPROPERTY()
	bool bForceClose = true;
}

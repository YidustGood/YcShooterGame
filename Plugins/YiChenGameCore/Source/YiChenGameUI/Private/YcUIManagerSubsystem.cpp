// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcUIManagerSubsystem.h"

#include "CommonLocalPlayer.h"
#include "GameUIPolicy.h"
#include "PrimaryGameLayout.h"
#include "GameFramework/HUD.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcUIManagerSubsystem)

UYcUIManagerSubsystem::UYcUIManagerSubsystem()
{
	
}

void UYcUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 添加使用FTSTicker实现Subsystem的Tick
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UYcUIManagerSubsystem::Tick), 0.0f);
}

void UYcUIManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
}

bool UYcUIManagerSubsystem::Tick(float DeltaTime)
{
	SyncRootLayoutVisibilityToShowHUD();
	
	return true;
}

void UYcUIManagerSubsystem::SyncRootLayoutVisibilityToShowHUD()
{
	const UGameUIPolicy* Policy = GetCurrentUIPolicy();
	if (Policy == nullptr) return;
	
	for (const ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		bool bShouldShowUI = true;
			
		if (const APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
		{
			const AHUD* HUD = PC->GetHUD();

			if (HUD && !HUD->bShowHUD)
			{
				bShouldShowUI = false;
			}
		}

		if (UPrimaryGameLayout* RootLayout = Policy->GetRootLayout(CastChecked<UCommonLocalPlayer>(LocalPlayer)))
		{
			const ESlateVisibility DesiredVisibility = bShouldShowUI ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
			if (DesiredVisibility != RootLayout->GetVisibility())
			{
				RootLayout->SetVisibility(DesiredVisibility);	
			}
		}
	}
}
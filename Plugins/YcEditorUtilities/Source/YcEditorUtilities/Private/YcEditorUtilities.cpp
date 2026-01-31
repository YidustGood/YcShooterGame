// Copyright Epic Games, Inc. All Rights Reserved.

#include "YcEditorUtilities.h"
#include "YcEditorUtilitiesStyle.h"
#include "YcEditorUtilitiesCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"


DEFINE_LOG_CATEGORY(LogYcEditorUtilities);

static const FName YcEditorUtilitiesTabName("YcEditorUtilities");

#define LOCTEXT_NAMESPACE "FYcEditorUtilitiesModule"

void FYcEditorUtilitiesModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FYcEditorUtilitiesStyle::Initialize();
	FYcEditorUtilitiesStyle::ReloadTextures();

	FYcEditorUtilitiesCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FYcEditorUtilitiesCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FYcEditorUtilitiesModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FYcEditorUtilitiesModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(YcEditorUtilitiesTabName, FOnSpawnTab::CreateRaw(this, &FYcEditorUtilitiesModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FYcEditorUtilitiesTabTitle", "YcEditorUtilities"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FYcEditorUtilitiesModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FYcEditorUtilitiesStyle::Shutdown();

	FYcEditorUtilitiesCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(YcEditorUtilitiesTabName);
}

TSharedRef<SDockTab> FYcEditorUtilitiesModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FYcEditorUtilitiesModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("YcEditorUtilities.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FYcEditorUtilitiesModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(YcEditorUtilitiesTabName);
}

void FYcEditorUtilitiesModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FYcEditorUtilitiesCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FYcEditorUtilitiesCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYcEditorUtilitiesModule, YcEditorUtilities)
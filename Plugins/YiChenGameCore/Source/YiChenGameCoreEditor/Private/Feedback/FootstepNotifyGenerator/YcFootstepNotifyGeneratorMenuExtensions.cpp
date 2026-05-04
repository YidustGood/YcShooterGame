// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Feedback/FootstepNotifyGenerator/YcFootstepNotifyGeneratorMenuExtensions.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include "AnimationToolMenuContext.h"
#include "AssetViewUtils.h"
#include "ContentBrowserMenuContexts.h"
#include "Editor.h"
#include "Feedback/FootstepNotifyGenerator/YcFootstepNotifyGenerationService.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IAnimationEditor.h"
#include "IDetailsView.h"
#include "IPersonaPreviewScene.h"
#include "IPersonaToolkit.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "YcFootstepNotifyGeneratorMenuExtensions"

namespace YcFootstepNotifyGeneratorMenuExtensions
{
	static constexpr const TCHAR* MenuOwnerName = TEXT("YiChenGameCoreEditor.FootstepNotifyGenerator");

	static void ShowNotification(const FText& Message, SNotificationItem::ECompletionState CompletionState)
	{
		FNotificationInfo NotificationInfo(Message);
		NotificationInfo.ExpireDuration = 5.0f;

		if (TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(NotificationInfo))
		{
			Notification->SetCompletionState(CompletionState);
		}
	}

	static bool ShowSettingsDialog(UYcFootstepNotifyGeneratorSettings* Settings, const FText& Title)
	{
		if (Settings == nullptr)
		{
			return false;
		}

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bUpdatesFromSelection = false;

		TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		DetailsView->SetObject(Settings);

		bool bAccepted = false;
		TSharedPtr<SWindow> Window;

		SAssignNew(Window, SWindow)
			.Title(Title)
			.ClientSize(FVector2D(560.0f, 720.0f))
			.SizingRule(ESizingRule::UserSized)
			.SupportsMinimize(false)
			.SupportsMaximize(false)
			[
				SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						DetailsView
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SUniformGridPanel)
						.SlotPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
						+ SUniformGridPanel::Slot(0, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("ConfirmGenerate", "生成"))
							.OnClicked_Lambda([&bAccepted, &Window]()
							{
								bAccepted = true;
								if (Window.IsValid())
								{
									Window->RequestDestroyWindow();
								}
								return FReply::Handled();
							})
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("CancelGenerate", "取消"))
							.OnClicked_Lambda([&bAccepted, &Window]()
							{
								bAccepted = false;
								if (Window.IsValid())
								{
									Window->RequestDestroyWindow();
								}
								return FReply::Handled();
							})
						]
					]
				]
			];

		GEditor->EditorAddModalWindow(Window.ToSharedRef());
		return bAccepted;
	}

	static void RunGeneratorForSequences(const TArray<UAnimSequence*>& AnimSequences, const FYcFootstepGenerationContext* Context = nullptr)
	{
		if (AnimSequences.IsEmpty())
		{
			ShowNotification(LOCTEXT("NoAnimSequenceSelected", "没有找到可生成脚步通知的动画序列。"), SNotificationItem::CS_Fail);
			return;
		}

		UYcFootstepNotifyGeneratorSettings* Settings = NewObject<UYcFootstepNotifyGeneratorSettings>(GetTransientPackage());
		if (!ShowSettingsDialog(Settings, LOCTEXT("GenerateSettingsDialog", "生成脚步上下文通知")))
		{
			return;
		}

		FText Summary;
		const bool bSucceeded = FYcFootstepNotifyGenerationService::GenerateForSequences(AnimSequences, Settings, Summary, Context);
		ShowNotification(Summary, bSucceeded ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}

	static void ExtendAnimSequenceContextMenu(FToolMenuSection& InSection)
	{
		const UContentBrowserAssetContextMenuContext* Context = InSection.FindContext<UContentBrowserAssetContextMenuContext>();
		if (Context == nullptr)
		{
			return;
		}

		TArray<FAssetData> AnimSequenceAssets;
		for (const FAssetData& AssetData : Context->SelectedAssets)
		{
			if (AssetData.AssetClassPath == UAnimSequence::StaticClass()->GetClassPathName())
			{
				AnimSequenceAssets.Add(AssetData);
			}
		}

		if (AnimSequenceAssets.IsEmpty())
		{
			return;
		}

		InSection.AddMenuEntry(
			"YcGenerateFootstepNotify",
			LOCTEXT("GenerateFootstepNotify", "生成脚步上下文通知"),
			LOCTEXT("GenerateFootstepNotifyTooltip", "分析选中的动画序列，并自动添加左右脚落地的上下文效果通知。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.AnimNotify"),
			FUIAction(FExecuteAction::CreateLambda([AnimSequenceAssets]()
			{
				TArray<UObject*> LoadedObjects;
				AssetViewUtils::LoadAssetsIfNeeded(AnimSequenceAssets, LoadedObjects, AssetViewUtils::FLoadAssetsSettings{});

				TArray<UAnimSequence*> AnimSequences;
				for (UObject* LoadedObject : LoadedObjects)
				{
					if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(LoadedObject))
					{
						AnimSequences.Add(AnimSequence);
					}
				}

				RunGeneratorForSequences(AnimSequences);
			})));
	}

	static void ExtendAnimationEditorToolbar(UToolMenu* Menu)
	{
		if (Menu == nullptr)
		{
			return;
		}

		UAnimationToolMenuContext* AnimationContext = Menu->FindContext<UAnimationToolMenuContext>();
		if (AnimationContext == nullptr || !AnimationContext->AnimationEditor.IsValid())
		{
			return;
		}

		TSharedPtr<IAnimationEditor> AnimationEditor = AnimationContext->AnimationEditor.Pin();
		if (!AnimationEditor.IsValid())
		{
			return;
		}

		UAnimationAsset* AnimationAsset = AnimationEditor->GetPersonaToolkit()->GetAnimationAsset();
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);
		if (AnimSequence == nullptr)
		{
			return;
		}

		FToolMenuSection& Section = Menu->FindOrAddSection("YcFootstepNotify");
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"YcGenerateFootstepNotifyFromEditor",
			FUIAction(FExecuteAction::CreateLambda([WeakAnimSequence = TWeakObjectPtr<UAnimSequence>(AnimSequence), WeakAnimationEditor = TWeakPtr<IAnimationEditor>(AnimationEditor)]()
			{
				if (UAnimSequence* PinnedSequence = WeakAnimSequence.Get())
				{
					FYcFootstepGenerationContext Context;
					if (TSharedPtr<IAnimationEditor> PinnedAnimationEditor = WeakAnimationEditor.Pin())
					{
						const TSharedRef<IPersonaToolkit> PersonaToolkit = PinnedAnimationEditor->GetPersonaToolkit();
						Context.PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
						Context.PreviewWorld = PersonaToolkit->GetPreviewScene()->GetWorld();
					}

					RunGeneratorForSequences({ PinnedSequence }, Context.IsValid() ? &Context : nullptr);
				}
			})),
			LOCTEXT("GenerateFootstepNotifyToolbar", "脚步通知"),
			LOCTEXT("GenerateFootstepNotifyToolbarTooltip", "为当前动画自动生成脚步上下文通知。"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.AnimNotify")));
	}
}

void FYcFootstepNotifyGeneratorMenuExtensions::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(YcFootstepNotifyGeneratorMenuExtensions::MenuOwnerName);

	if (UToolMenu* ContentBrowserMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.AnimSequence"))
	{
		FToolMenuSection& Section = ContentBrowserMenu->FindOrAddSection("GetAssetActions");
		Section.AddDynamicEntry(
			"YcFootstepNotifyGeneratorEntry",
			FNewToolMenuSectionDelegate::CreateStatic(&YcFootstepNotifyGeneratorMenuExtensions::ExtendAnimSequenceContextMenu));
	}

	if (UToolMenu* AnimationEditorToolbar = UToolMenus::Get()->ExtendMenu("AssetEditor.AnimationEditor.ToolBar"))
	{
		AnimationEditorToolbar->AddDynamicSection(
			"YcFootstepNotifyGeneratorToolbarSection",
			FNewToolMenuDelegate::CreateStatic(&YcFootstepNotifyGeneratorMenuExtensions::ExtendAnimationEditorToolbar));
	}
}

void FYcFootstepNotifyGeneratorMenuExtensions::UnregisterMenus()
{
	UToolMenus::UnregisterOwner(YcFootstepNotifyGeneratorMenuExtensions::MenuOwnerName);
}

#undef LOCTEXT_NAMESPACE

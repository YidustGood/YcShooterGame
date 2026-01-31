// Copyright Epic Games, Inc. All Rights Reserved.

#include "YcEditorUtilitiesStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FYcEditorUtilitiesStyle::StyleInstance = nullptr;

void FYcEditorUtilitiesStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FYcEditorUtilitiesStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FYcEditorUtilitiesStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("YcEditorUtilitiesStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FYcEditorUtilitiesStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("YcEditorUtilitiesStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("YcEditorUtilities")->GetBaseDir() / TEXT("Resources"));

	Style->Set("YcEditorUtilities.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	return Style;
}

void FYcEditorUtilitiesStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FYcEditorUtilitiesStyle::Get()
{
	return *StyleInstance;
}

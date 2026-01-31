// Copyright Epic Games, Inc. All Rights Reserved.

#include "YcEditorUtilitiesCommands.h"

#define LOCTEXT_NAMESPACE "FYcEditorUtilitiesModule"

void FYcEditorUtilitiesCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "YcEditorUtilities", "Bring up YcEditorUtilities window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE

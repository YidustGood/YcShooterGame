// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "YcEditorUtilitiesStyle.h"

class FYcEditorUtilitiesCommands : public TCommands<FYcEditorUtilitiesCommands>
{
public:

	FYcEditorUtilitiesCommands()
		: TCommands<FYcEditorUtilitiesCommands>(TEXT("YcEditorUtilities"), NSLOCTEXT("Contexts", "YcEditorUtilities", "YcEditorUtilities Plugin"), NAME_None, FYcEditorUtilitiesStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
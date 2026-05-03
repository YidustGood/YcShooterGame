// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Messaging/CommonGameDialog.h"
#include "AsCommonGameDialogLibrary.generated.h"

UCLASS()
class YCANGELSCRIPTMIXIN_API UAsCommonGameDialogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(ScriptCallable, Category = "CommonGame|Dialog")
	static UCommonGameDialogDescriptor* CreateConfirmationOk(const FText& Header, const FText& Body)
	{
		return UCommonGameDialogDescriptor::CreateConfirmationOk(Header, Body);
	}

	UFUNCTION(ScriptCallable, Category = "CommonGame|Dialog")
	static UCommonGameDialogDescriptor* CreateConfirmationOkCancel(const FText& Header, const FText& Body)
	{
		return UCommonGameDialogDescriptor::CreateConfirmationOkCancel(Header, Body);
	}

	UFUNCTION(ScriptCallable, Category = "CommonGame|Dialog")
	static UCommonGameDialogDescriptor* CreateConfirmationYesNo(const FText& Header, const FText& Body)
	{
		return UCommonGameDialogDescriptor::CreateConfirmationYesNo(Header, Body);
	}

	UFUNCTION(ScriptCallable, Category = "CommonGame|Dialog")
	static UCommonGameDialogDescriptor* CreateConfirmationYesNoCancel(const FText& Header, const FText& Body)
	{
		return UCommonGameDialogDescriptor::CreateConfirmationYesNoCancel(Header, Body);
	}
};

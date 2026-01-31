// Fill out your copyright notice in the Description page of Project Settings.


#include "YcEditorUtilitiesStatics.h"

#include "SGameplayTagPicker.h"
#include "YcEditorUtilities.h"

void UYcEditorUtilitiesStatics::OpenGameplayTagsManager()
{
	FGameplayTagManagerWindowArgs Args;
	Args.bRestrictedTags = false;
	UE::GameplayTags::Editor::OpenGameplayTagManager(Args);
	UE_LOG(LogYcEditorUtilities, Log, TEXT("UYcEditorUtilitiesStatics::OpenGameplayTagsManager()->Open GameplayTagManager Window."));
}

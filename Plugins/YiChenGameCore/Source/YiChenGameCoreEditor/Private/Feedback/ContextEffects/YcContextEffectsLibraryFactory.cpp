// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Feedback/ContextEffects/YcContextEffectsLibraryFactory.h"

#include "Feedback/ContextEffects/YcContextEffectsLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcContextEffectsLibraryFactory)


UYcContextEffectsLibraryFactory::UYcContextEffectsLibraryFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UYcContextEffectsLibrary::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UYcContextEffectsLibraryFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UYcContextEffectsLibrary* ContextEffectsLibrary = NewObject<UYcContextEffectsLibrary>(InParent, Name, Flags);

	return ContextEffectsLibrary;
}

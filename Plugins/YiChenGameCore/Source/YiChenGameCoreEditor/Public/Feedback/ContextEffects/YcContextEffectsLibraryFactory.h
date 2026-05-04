// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YcContextEffectsLibraryFactory.generated.h"

/** YcContextEffectsLibrary 资产工厂，用于在编辑器中新建反馈资源库资产。 */
UCLASS(hidecategories = Object, MinimalAPI)
class UYcContextEffectsLibraryFactory : public UFactory
{
	GENERATED_UCLASS_BODY()
	
	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override
	{
		return true;
	}
	//~ End UFactory Interface	
};

// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentLibrary.h"
#include "YcEquipmentInstance.h"
#include "YcQuickBarComponent.h"
#include "Blueprint/BlueprintExceptionInfo.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "UObject/EnumProperty.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentLibrary)

#define LOCTEXT_NAMESPACE "UYcEquipmentLibrary"

UE_DISABLE_OPTIMIZATION

UYcEquipmentInstance* UYcEquipmentLibrary::GetCurrentEquipmentInst(AActor* OwnerActor)
{
	const UYcQuickBarComponent* QuickBar = UYcQuickBarComponent::FindQuickBarComponent(OwnerActor);
	if (!QuickBar) return nullptr;
	
	return QuickBar->GetActiveEquipmentInstance();
}

TInstancedStruct<FYcEquipmentFragment> UYcEquipmentLibrary::FindEquipmentFragment(
	const FYcEquipmentDefinition& EquipmentDef, const UScriptStruct* FragmentStructType)
{
	TInstancedStruct<FYcEquipmentFragment> NullStruct;
	for (auto& Frag : EquipmentDef.Fragments)
	{	
		if (FragmentStructType == Frag.GetScriptStruct())
		{
			return Frag;
		}
	}
	return NullStruct;
}

void UYcEquipmentLibrary::GetEquipmentFragment(EYcEquipmentFragmentResult& ExecResult, const UYcEquipmentInstance* Equipment, const UScriptStruct* FragmentStructType, int32& OutFragment)
{
	// We should never hit this! stubs to avoid NoExport on the class.
	checkNoEntry();
}

DEFINE_FUNCTION(UYcEquipmentLibrary::execGetEquipmentFragment)
{
	P_GET_ENUM_REF(EYcEquipmentFragmentResult, ExecResult);
	P_GET_OBJECT(UYcEquipmentInstance, Equipment);
	P_GET_OBJECT(UScriptStruct, FragmentStructType);

	// Read wildcard OutFragment input.
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentPropertyContainer = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	
	const FStructProperty* OutFragmentProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* OutFragmentPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	ExecResult = EYcEquipmentFragmentResult::NotValid;

	if (!OutFragmentProp || !OutFragmentPtr)
	{
		FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AbortExecution,
			LOCTEXT("EquipmentFragment_GetInvalidValueWarning", "Failed to resolve the OutFragment for Get Equipment Fragment")
		);

		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
	}
	else if (!Equipment || !FragmentStructType)
	{
		ExecResult = EYcEquipmentFragmentResult::NotValid;
	}
	else
	{
		P_NATIVE_BEGIN;
		TInstancedStruct<FYcEquipmentFragment> TempFragment = Equipment->FindEquipmentFragment(FragmentStructType);
		if (TempFragment.IsValid() && 
			TempFragment.GetScriptStruct()->IsChildOf(OutFragmentProp->Struct))
		{
			// 直接将 Fragment 的内存复制到输出参数
			OutFragmentProp->Struct->CopyScriptStruct(OutFragmentPtr, TempFragment.GetMemory());
			ExecResult = EYcEquipmentFragmentResult::Valid;
		}
		else
		{
			ExecResult = EYcEquipmentFragmentResult::NotValid;
		}
		P_NATIVE_END;
	}
}

#undef LOCTEXT_NAMESPACE

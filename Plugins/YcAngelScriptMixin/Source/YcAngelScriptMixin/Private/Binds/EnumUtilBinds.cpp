#include "AngelscriptBinds.h"
#include "StaticJIT/StaticJITHelperFunctions.h"
#include "AngelscriptManager.h"
#include "Engine/UserDefinedEnum.h"

//AS_FORCE_LINK
const FAngelscriptBinds::FBind Bind_EnumUtils((int32)FAngelscriptBinds::EOrder::Late+10, []
{
	FAngelscriptBinds::FNamespace Namespace("Enum");

	FAngelscriptBinds::BindGlobalFunction(
		"TMap<FString, int>& GetValueMap(const ?& EnumType, bool bIgnoreLast = false)",
		[](void* ValuePtr, const int TypeId, const bool bIgnoreLast = false)
		{
			const auto ToReturn = new TMap<FString, int>();
			const asITypeInfo* TypeInfo = FAngelscriptManager::Get().Engine->GetTypeInfoById(TypeId);
			
			if (TypeInfo == nullptr || (TypeInfo->GetFlags() & asOBJ_ENUM) == 0)
			{
				FAngelscriptManager::Throw("Non-enum value passed into Enum::GetValueMap");
				return ToReturn;
			}

			if (const UUserDefinedEnum* UnrealEnum = static_cast<UUserDefinedEnum*>(TypeInfo->GetUserData()); UnrealEnum != nullptr)
			{
				int NumValues = UnrealEnum->NumEnums();

				if (NumValues == 0)
				{
					return ToReturn;
				}

				if (UnrealEnum->GetNameStringByIndex(NumValues - 1).EndsWith(TEXT("_MAX")))
				{
					NumValues--;
				}
				
				if (bIgnoreLast)
				{
					NumValues--;
				}
				
				for (int i = 0; i < NumValues; i++)
				{
					FString UnrealValueName = UnrealEnum->GetDisplayNameTextByIndex(i).ToString();
					const uint8 UnrealValue = static_cast<uint8>(UnrealEnum->GetValueByIndex(i));
					ToReturn->Add(UnrealValueName, UnrealValue);
				}
				
				return ToReturn;
			}
			
			const asUINT EnumCount = TypeInfo->GetEnumValueCount();
			for(asUINT i = 0; bIgnoreLast ? i < EnumCount - 1 : i < EnumCount; ++i)
			{
				int UnrealValue;
				FString UnrealValueName = ANSI_TO_TCHAR(TypeInfo->GetEnumValueByIndex(i, &UnrealValue));
				ToReturn->Add(UnrealValueName, UnrealValue);
			}

			return ToReturn;
		}
	);

	FAngelscriptBinds::BindGlobalFunction(
		"FString GetDisplayName(const ?& EnumType)",
		[](void* ValuePtr, const int TypeId) -> FString
		{
			const asITypeInfo* TypeInfo = FAngelscriptManager::Get().Engine->GetTypeInfoById(TypeId);
			
			if (TypeInfo == nullptr || (TypeInfo->GetFlags() & asOBJ_ENUM) == 0)
			{
				FAngelscriptManager::Throw("Non-enum value passed into Enum::GetDisplayName");
				return FString("");
			}

			if (const UUserDefinedEnum* UnrealEnum = static_cast<UUserDefinedEnum*>(TypeInfo->GetUserData()); UnrealEnum != nullptr)
			{
				return UnrealEnum->GetDisplayNameTextByValue(*static_cast<uint8*>(ValuePtr)).ToString();
			}

			const asUINT EnumCount = TypeInfo->GetEnumValueCount();
			for(asUINT i = 0; i < EnumCount; ++i)
			{
				int EnumValue;
				const char* ValueName = TypeInfo->GetEnumValueByIndex(i, &EnumValue);
				if (EnumValue == *static_cast<uint8*>(ValuePtr) && ValueName != nullptr)
				{
					return ANSI_TO_TCHAR(ValueName);
				}
			}

			return FString("");
		}
	);
});

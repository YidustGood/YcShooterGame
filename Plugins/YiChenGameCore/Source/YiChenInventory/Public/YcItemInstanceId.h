// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcItemInstanceId.generated.h"

/**
 * 物品实例唯一标识。
 * 以FGuid作为底层值，避免跨Inventory/跨场景的ID冲突。
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcItemInstanceId
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	FGuid Value;

	FYcItemInstanceId() = default;
	explicit FYcItemInstanceId(const FGuid& InValue) : Value(InValue) {}

	static FYcItemInstanceId NewId()
	{
		return FYcItemInstanceId(FGuid::NewGuid());
	}

	bool IsValid() const
	{
		return Value.IsValid();
	}

	FString ToString(EGuidFormats Format = EGuidFormats::DigitsWithHyphens) const
	{
		return Value.ToString(Format);
	}

	bool operator==(const FYcItemInstanceId& Other) const
	{
		return Value == Other.Value;
	}

	bool operator!=(const FYcItemInstanceId& Other) const
	{
		return !(*this == Other);
	}
};

FORCEINLINE uint32 GetTypeHash(const FYcItemInstanceId& InId)
{
	return GetTypeHash(InId.Value);
}

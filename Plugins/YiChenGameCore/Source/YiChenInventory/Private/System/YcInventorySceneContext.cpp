// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcInventorySceneContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventorySceneContext)

bool UYcInventorySceneContext::IsValidForOutOfMatchPersistence() const
{
	if (!IsOutOfMatchContext())
	{
		return false;
	}

	if (!ProfileIdentity.IsValid())
	{
		return false;
	}

	return Runtime.SupportsOutOfMatchPersistence();
}

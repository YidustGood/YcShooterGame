// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcInventorySceneContext.h"

#include "YcInventoryManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventorySceneContext)

bool UYcInventorySceneContext::IsValidForOutOfMatchPersistence() const
{
	if (!bRequirePersistenceCommit || !IsOutOfMatchContext())
	{
		return false;
	}

	if (AccountId.IsEmpty())
	{
		return false;
	}

	return IsValid(PlayerInventoryRef) && IsValid(ContainerInventoryRef);
}

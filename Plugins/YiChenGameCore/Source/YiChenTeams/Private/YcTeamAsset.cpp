// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamAsset.h"

#include "YcTeamSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamAsset)

#if WITH_EDITOR
void UYcTeamAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for (UYcTeamSubsystem* TeamSubsystem : TObjectRange<UYcTeamSubsystem>())
	{
		TeamSubsystem->NotifyTeamAssetModified(this);
	}
}
#endif
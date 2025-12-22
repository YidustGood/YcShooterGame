// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamAgentInterface.h"

#include "YiChenTeams.h"
#include "YiChenGameCore/Public/Utils/CommonSimpleUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamAgentInterface)

UYcTeamAgentInterface::UYcTeamAgentInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void IYcTeamAgentInterface::ConditionalBroadcastTeamChanged(const TScriptInterface<IYcTeamAgentInterface>& This, const FGenericTeamId OldTeamID, const FGenericTeamId NewTeamID)
{
	// 仅在团队ID实际发生变化时进行广播
	if (OldTeamID != NewTeamID)
	{
		// 将通用团队ID转换为整数索引
		const int32 OldTeamIndex = GenericTeamIdToInteger(OldTeamID); 
		const int32 NewTeamIndex = GenericTeamIdToInteger(NewTeamID);

		UObject* ThisObj = This.GetObject();
		UE_LOG(LogYcTeamSystem, Verbose, TEXT("[%s] %s assigned team %d"), *GetClientServerContextString(ThisObj), *GetPathNameSafe(ThisObj), NewTeamIndex);

		// 广播团队变更事件，通知所有订阅者
		This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeamIndex, NewTeamIndex);
	}
}
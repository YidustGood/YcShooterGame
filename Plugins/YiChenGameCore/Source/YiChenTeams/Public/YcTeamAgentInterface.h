// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GenericTeamAgentInterface.h"
#include "YcTeamAgentInterface.generated.h"

template <typename InterfaceType> class TScriptInterface;

/** 团队索引变更委托，当对象的团队ID发生变化时触发 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnYcTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamID, int32, NewTeamID);

/** 将通用团队ID转换为整数类型 */
inline int32 GenericTeamIdToInteger(const FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam) ? INDEX_NONE : static_cast<int32>(ID);
}

/** 将整数类型转换为通用团队ID */
inline FGenericTeamId IntegerToGenericTeamId(const int32 ID)
{
	return (ID == INDEX_NONE) ? FGenericTeamId::NoTeam : FGenericTeamId(static_cast<uint8>(ID));
}

/** 用于与团队相关联的Actor的接口, 通过实现该接口赋予Actor团队相关的功能 */
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UYcTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};

/** 用于与团队相关联的Actor的接口, 通过实现该接口赋予Actor团队相关的功能 */
class YICHENTEAMS_API IYcTeamAgentInterface: public IGenericTeamAgentInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * 获取当前对象的团队ID
	 * @return 团队ID的整数值，如果未分配团队则返回INDEX_NONE
	 */
	virtual int32 GetTeamId() const { return GenericTeamIdToInteger(FGenericTeamId::NoTeam); };
	
	/**
	 * 获取团队索引变更委托的指针
	 * @return 团队索引变更委托的指针，如果未实现则返回nullptr
	 */
	virtual FOnYcTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	/**
	 * 条件性地广播团队变更事件
	 * 当旧团队ID与新团队ID不同时，触发团队变更委托
	 * @param This 实现了IYcTeamAgentInterface接口的对象
	 * @param OldTeamID 旧的团队ID
	 * @param NewTeamID 新的团队ID
	 */
	static void ConditionalBroadcastTeamChanged(const TScriptInterface<IYcTeamAgentInterface>& This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID);
	
	/** 获取团队变更委托的引用（已检查非空） */
	FOnYcTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnYcTeamIndexChangedDelegate* Result = GetOnTeamIndexChangedDelegate();
		check(Result);
		return *Result;
	}
};
// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcTeamInfoBase.h"
#include "YcTeamPublicInfo.generated.h"

class UYcTeamAsset;

/**
 * 队伍的公开信息，所有人可以访问
 */
UCLASS()
class YICHENTEAMS_API AYcTeamPublicInfo : public AYcTeamInfoBase
{
	GENERATED_BODY()

	friend UYcTeamCreationComponent;

public:
	AYcTeamPublicInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 获取团队资源资产
	 * @return 团队资源资产的指针，如果未设置则返回nullptr
	 */
	UYcTeamAsset* GetTeamAsset() const { return TeamAsset; }

private:
	/** 团队资源资产复制属性变更时的回调函数 */
	UFUNCTION()
	void OnRep_TeamDisplayAsset();

	/**
	 * 设置团队资源资产
	 * 注意：此函数只能在服务器端调用，且TeamAsset必须为nullptr
	 * @param NewTeamAsset 新的团队资源资产
	 */
	void SetTeamAsset(TObjectPtr<UYcTeamAsset> NewTeamAsset);
	
	/** 团队资源资产，仅在初始时进行网络复制 */
	UPROPERTY(ReplicatedUsing=OnRep_TeamDisplayAsset)
	TObjectPtr<UYcTeamAsset> TeamAsset;
};
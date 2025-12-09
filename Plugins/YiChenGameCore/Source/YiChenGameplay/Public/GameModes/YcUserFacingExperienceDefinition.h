// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "YcUserFacingExperienceDefinition.generated.h"

class UCommonSession_HostSessionRequest;

/**
 * 面向用户的游戏体验定义资产。
 * 
 * 该资产用于在 UI 中展示游戏体验，并包含启动新会话所需的所有配置信息，
 * 包括地图、游戏体验、加载屏幕等设置。
 */
UCLASS(BlueprintType)
class YICHENGAMEPLAY_API UYcUserFacingExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** 本体验的本地化描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Description")
	FText Description = FText::GetEmpty();
	
	/** 要加载的特定地图 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="Map"))
	FPrimaryAssetId MapID;

	/** 要加载的游戏体验 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="YcExperienceDefinition"))
	FPrimaryAssetId ExperienceID;

	/** 作为 URL 选项传递给游戏的额外参数 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TMap<FString, FString> ExtraArgs;

	/** UI 中显示的主标题 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileTitle;

	/** UI 中显示的副标题 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileSubTitle;

	/** UI 中显示的完整描述 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileDescription;

	/** UI 中显示的图标 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TObjectPtr<UTexture2D> TileIcon;

	/** 在加载或退出此体验时显示的加载屏幕 UI 组件 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=LoadingScreen)
	TSoftClassPtr<UUserWidget> LoadingScreenWidget;

	/** 如果为 true，此体验将被用作快速游玩的默认体验，并在 UI 中获得优先级 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bIsDefaultExperience = false;

	/** 如果为 true，此体验将在前端的体验列表中显示 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bShowInFrontEnd = true;

	/** 此会话的最大玩家数 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	int32 MaxPlayerCount = 16;

public:
	/**
	 * 创建用于启动会话的请求对象。
	 * 
	 * 根据此资产中的配置信息创建一个会话请求对象，该对象可用于实际启动游戏会话。
	 * 
	 * @return 创建的会话请求对象指针
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	UCommonSession_HostSessionRequest* CreateHostingRequest() const;
};

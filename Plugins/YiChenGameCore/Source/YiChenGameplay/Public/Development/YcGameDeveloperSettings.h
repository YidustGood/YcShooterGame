// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "YcGameDeveloperSettings.generated.h"

/**
 * Developer settings / editor cheat
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class UYcGameDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
public:
	UYcGameDeveloperSettings();
	
	//~UDeveloperSettings interface
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface
	
	// The experience override to use for Play in Editor (if not set, the default for the world settings of the open map will be used)
	// 在编辑器中使用的体验覆盖(如果没有设置，将使用打开地图的默认世界设置)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category="YcGameCore", meta=(AllowedTypes="YcExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;
	
public:
#if WITH_EDITORONLY_DATA
	/** A list of common maps that will be accessible via the editor detoolbar */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
#endif
	
#if WITH_EDITOR

	// Called by the editor engine to let us pop reminder notifications when cheats are active
	YICHENGAMEPLAY_API void OnPlayInEditorStarted() const;

private:
	void ApplySettings();
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
	
	
};

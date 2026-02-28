// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class YcShooterServerTarget : TargetRules
{
	public YcShooterServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		bWithPushModel = true;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("YcShooter");
	}
}

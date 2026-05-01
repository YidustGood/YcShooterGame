// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YiChenQuest : ModuleRules
{
    public YiChenQuest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "NetCore",
                "StructUtils",
                "YiChenGameCore",
                "YiChenAccountCore",
                "YiChenSaveCore",
                "YiChenGameplay"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "GameplayMessageRuntime"
            }
        );
    }
}

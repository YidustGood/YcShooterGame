// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YiChenSaveCore : ModuleRules
{
    public YiChenSaveCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "YiChenAccountCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Engine",
                "YiChenGameCore"
            }
        );
    }
}

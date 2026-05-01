// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YiChenAccountCore : ModuleRules
{
    public YiChenAccountCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Engine"
            }
        );
    }
}

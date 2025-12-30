// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YiChenInventory : ModuleRules
{
    public YiChenInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "CoreUObject", 
                "NetCore", 
                "StructUtils", 
                "YiChenGameCore",
                "GameplayTags",
                "DataRegistry"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "GameplayMessageRuntime"
            }
        );
    }
}
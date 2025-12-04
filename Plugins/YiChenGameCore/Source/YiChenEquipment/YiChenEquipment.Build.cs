// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YiChenEquipment : ModuleRules
{
    public YiChenEquipment(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "YiChenGameCore",
                "YiChenInventory"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore"
            }
        );
    }
}
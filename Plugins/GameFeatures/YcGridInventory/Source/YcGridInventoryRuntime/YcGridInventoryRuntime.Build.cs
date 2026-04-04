// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YcGridInventoryRuntime : ModuleRules
{
	public YcGridInventoryRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"YiChenInventory", 
				"GameplayAbilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}

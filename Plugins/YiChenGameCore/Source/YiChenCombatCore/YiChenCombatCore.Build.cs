// Copyright (c) 2025 YiChen. All Rights Reserved.

using UnrealBuildTool;

public class YiChenCombatCore : ModuleRules
{
	public YiChenCombatCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GameplayTags",
				"GameplayAbilities",
				"YiChenAbility",
				"YiChenDamage",
				"YiChenInventory",
				"YiChenEquipment",
				"YiChenGameCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"NetCore",
				"GameplayMessageRuntime"
			}
		);
	}
}

using UnrealBuildTool;

public class YiChenGameplay : ModuleRules
{
    public YiChenGameplay(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CommonGame",
                "EnhancedInput",
                "GameplayAbilities",
                "CommonLoadingScreen",
                "ModularGameplayActors",
                "DataRegistry",
                "DeveloperSettings",
                "YiChenGameCore",
                "YiChenAbility",
                "GameFeatures",
                "YiChenTeams",
                "YiChenDamage",
                "YiChenInventory",
                "YiChenEquipment",
                "YiChenAccountCore",
                "ModularGameplay"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "NetCore",
                "Slate",
                "SlateCore",
                "EnhancedInput",
                "ModularGameplay",
                "GameplayTags",
                "GameFeatures",
                "DeveloperSettings",
                "CommonUser",
                "EngineSettings",
                "UMG",
                "GameplayTasks",
                "GameplayMessageRuntime",
                "UIExtension",
                "PhysicsCore",
                "Niagara",
                "AIModule",
                "OnlineSubsystemUtils",
                "YiChenCombatCore"
            }
        );
    }
}

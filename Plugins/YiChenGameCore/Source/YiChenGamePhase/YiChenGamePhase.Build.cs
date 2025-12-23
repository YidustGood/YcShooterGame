using UnrealBuildTool;

public class YiChenGamePhase : ModuleRules
{
    public YiChenGamePhase(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "YiChenAbility",
                "GameplayAbilities",
                "GameplayTags",
                "ModularGameplay", 
                "GameplayMessageRuntime", 
                "YiChenGameCore"
            }
        );
    }
}
using UnrealBuildTool;

public class YiChenTeams : ModuleRules
{
    public YiChenTeams(ReadOnlyTargetRules Target) : base(Target)
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
                "YiChenGameCore",
                "GameplayTags",
                "GameplayAbilities",
                "ModularGameplay", 
                "GameplayMessageRuntime",
                "AIModule"
            }
        );
    }
}
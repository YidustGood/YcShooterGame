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
                "GameplayAbilities", 
                "CommonLoadingScreen",
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
                "YiChenAbility",
                "YiChenGameCore",
                "EnhancedInput",
                "ModularGameplayActors",
                "ModularGameplay",
                "GameplayTags", 
                "GameFeatures",
                "DeveloperSettings",
                "CommonUser",
                "EngineSettings",
                "UMG",
                "GameplayTasks"
            }
        );
    }
}
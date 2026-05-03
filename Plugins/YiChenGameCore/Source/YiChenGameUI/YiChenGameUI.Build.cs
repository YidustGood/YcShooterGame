using UnrealBuildTool;

public class YiChenGameUI : ModuleRules
{
    public YiChenGameUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "GameplayMessageRuntime",
                "StructUtils",
                "UIExtension",
                "CommonUI",
                "UMG",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "ModularGameplay", 
                "CommonGame",
                "UMG",
                "GameFeatures",
                "CommonInput",
                "InputCore",
                "YiChenGameplay",
                "EnhancedInput",
                "DeveloperSettings",
                "NetCore",
            }
        );
    }
}

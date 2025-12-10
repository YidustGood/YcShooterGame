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
                "CommonUI", 
                "CommonGame",
                "GameplayTags",
                "UMG",
            }
        );
    }
}
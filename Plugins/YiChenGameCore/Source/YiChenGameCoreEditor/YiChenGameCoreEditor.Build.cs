using UnrealBuildTool;

public class YiChenGameCoreEditor : ModuleRules
{
    public YiChenGameCoreEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "AssetTools",
                "AnimationBlueprintLibrary",
                "AnimationEditor",
                "ContentBrowser",
                "EditorFramework",
                "Persona",
                "PropertyEditor",
                "UnrealEd",
                "PhysicsCore",
                "GameplayTags",
                "GameplayTagsEditor",
                "GameplayTasksEditor",
                "GameplayAbilities",
                "GameplayAbilitiesEditor",
                "StudioTelemetry",
                "YiChenGameCore",
                "YiChenGameplay"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"InputCore",
				"Slate",
				"SlateCore",
				"AppFramework",
				"ToolMenus",
				"EditorStyle",
				"DataValidation",
				"MessageLog",
				"Projects",
				"DeveloperToolSettings",
				"CollectionManager",
				"SourceControl",
				"Chaos"
            }
        );
    }
}

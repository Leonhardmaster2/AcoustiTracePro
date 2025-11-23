// Copyright AcoustiTrace Pro. All Rights Reserved.

using UnrealBuildTool;

public class AcousticEngineEditor : ModuleRules
{
    public AcousticEngineEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "AcousticEngine"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
            "UnrealEd",
            "PropertyEditor",
            "EditorStyle",
            "InputCore",
            "LevelEditor",
            "ComponentVisualizers",
            "PlacementMode"
        });
    }
}

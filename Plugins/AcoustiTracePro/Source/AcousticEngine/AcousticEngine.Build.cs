// Copyright AcoustiTrace Pro. All Rights Reserved.

using UnrealBuildTool;

public class AcousticEngine : ModuleRules
{
    public AcousticEngine(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Enable faster iteration
        bUseUnity = false;

        PublicIncludePaths.AddRange(new string[] {
            ModuleDirectory + "/Public"
        });

        PrivateIncludePaths.AddRange(new string[] {
            ModuleDirectory + "/Private"
        });

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "AudioMixer",
            "AudioExtensions",
            "SignalProcessing",
            "MetasoundEngine",
            "MetasoundFrontend",
            "MetasoundGraphCore",
            "MetasoundStandardNodes"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Projects",
            "InputCore",
            "Slate",
            "SlateCore",
            "DeveloperSettings",
            "PhysicsCore",
            "Chaos",
            "GeometryCore"
        });

        // Platform-specific dependencies
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.Add("XAUDIO2");
        }

        // Editor-only dependencies
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "UnrealEd",
                "PropertyEditor"
            });
        }
    }
}

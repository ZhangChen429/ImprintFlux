using UnrealBuildTool;

public class CameraTrackToolEditor : ModuleRules
{
    public CameraTrackToolEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "LevelSequence",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "AssetRegistry",
                "AssetTools",
                "UnrealEd",
                "MovieScene",
                "MovieSceneTracks",
                "CinematicCamera",
                "Json"
            }
        );
    }
}
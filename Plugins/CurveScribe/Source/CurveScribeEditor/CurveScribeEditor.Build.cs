using UnrealBuildTool;

public class CurveScribeEditor : ModuleRules
{
    public CurveScribeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core","Json",          
                "JsonUtilities"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",              
                "LevelEditor",           
                "ToolMenus",           
                "InputCore",            
                "EditorStyle",
                "PropertyEditor",
                "Projects",              
                "WorkspaceMenuStructure", "CurveScribe", "MeshConversion",
                "SlateCore",
                "MeshDescription",
                "StaticMeshDescription",
                "GeometryCore",
                "MeshUtilities",
                "UnrealEd",
                "Kismet"
            }
        );
    }
}
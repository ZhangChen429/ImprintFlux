using UnrealBuildTool;

public class PythonHotEditor : ModuleRules
{
	public PythonHotEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",	"SlateCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
			
				"UnrealEd",
				"LevelEditor",
				"ToolMenus",
				"InputCore",
				"EditorStyle",
				"Projects",
				"WorkspaceMenuStructure",
				"PythonHot", "Blutility"
			}
		);
	}
}
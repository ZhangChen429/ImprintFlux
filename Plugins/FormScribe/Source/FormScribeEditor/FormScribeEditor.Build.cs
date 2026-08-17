using UnrealBuildTool;

public class FormScribeEditor : ModuleRules
{
	public FormScribeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"FormScribe",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"UnrealEd",
				"AssetDefinition",
				"AdvancedPreviewScene",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"InputCore",
			}
		);
	}
}

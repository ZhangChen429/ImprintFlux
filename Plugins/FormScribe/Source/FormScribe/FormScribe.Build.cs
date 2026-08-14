// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FormScribe : ModuleRules
{
	public FormScribe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
			);
	}
}

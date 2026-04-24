// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CurveScribe : ModuleRules
{
	public CurveScribe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"DeveloperSettings",
				"ProceduralMeshComponent",
				"InteractiveToolsFramework",
			}
			);

		// 引入 AlterMesh.dll
		string DllDirectory = Path.Combine(PluginDirectory, "Binaries", "Win64");
		string DllPath = Path.Combine(DllDirectory, "AlterMesh.dll");

		// 延迟加载（运行时手动加载，不在启动时自动链接）
		PublicDelayLoadDLLs.Add("AlterMesh.dll");

		// 打包时把 DLL 一起复制到输出目录
		RuntimeDependencies.Add(DllPath);
	}
}

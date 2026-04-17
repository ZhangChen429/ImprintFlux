
#include "Style/BlenderPluginStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
TSharedPtr<FSlateStyleSet> FBlenderPluginStyle::StyleSet;
FName FBlenderPluginStyle::StyleSetName = TEXT("BlenderPlugin");
void FBlenderPluginStyle::Initialize()
{
	if (!StyleSet.IsValid())
	{
		StyleSet = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
	}
}

void FBlenderPluginStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

FName FBlenderPluginStyle::GetStyleSetName()
{
	return StyleSetName;
}

TSharedRef<FSlateStyleSet> FBlenderPluginStyle::Create()
{
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	// 设置资源根目录为插件的 Resources 文件夹
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("CurveScribe")->GetBaseDir() / TEXT("Resources"));

	// 注册 Blender 按钮图标
	Style->Set(
		"BlenderPlugin.OpenBlenderWindow",
		new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("blender.svg")), Icon20x20)
	);

	// 也注册一个 40x40 的版本用于可能的大图标
	Style->Set(
		"BlenderPlugin.OpenBlenderWindow.Large",
		new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("blender.svg")), Icon40x40)
	);

	// 注册贝塞尔曲线按钮图标（使用引擎内置的曲线图标）
	Style->Set(
		"BlenderPlugin.OpenBezierCurveWindow",
		new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("blender.svg")), Icon20x20)
	);
	Style->Set(
		"BlenderPlugin.OpenBezierCurveWindow.Large",
		new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("blender.svg")), Icon40x40)
	);

	return Style;
}
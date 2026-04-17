#include "Style/PythonHotEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FPythonHotEditorStyle::StyleSet;
FName FPythonHotEditorStyle::StyleSetName = TEXT("PythonHotEditor");

void FPythonHotEditorStyle::Initialize()
{
	if (!StyleSet.IsValid())
	{
		StyleSet = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
	}
}

void FPythonHotEditorStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

FName FPythonHotEditorStyle::GetStyleSetName()
{
	return StyleSetName;
}

TSharedRef<FSlateStyleSet> FPythonHotEditorStyle::Create()
{
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	// 设置资源根目录为插件的 Resources 文件夹
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("PythonHot")->GetBaseDir() / TEXT("Resources"));

	// 注册 Python Debug 按钮图标
	Style->Set(
		"PythonHotEditor.OpenPythonDebugWindow",
		new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("icon-icons.svg")), Icon20x20)
	);

	// 注册大图标版本
	Style->Set(
		"PythonHotEditor.OpenPythonDebugWindow.Large",
		new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("icon-icons.svg")), Icon40x40)
	);

	return Style;
}
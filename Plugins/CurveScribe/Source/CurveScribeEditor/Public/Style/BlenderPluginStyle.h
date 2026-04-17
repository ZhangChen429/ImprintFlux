#pragma once


#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * UEBlender 编辑器样式
 * 管理编辑器按钮图标和其他 UI 样式
 */
class FBlenderPluginStyle
{
public:
	// 初始化样式
	static void Initialize();

	// 关闭样式
	static void Shutdown();

	// 获取样式集名称
	static FName GetStyleSetName();

	// 获取样式集
	static const ISlateStyle& Get() { return *StyleSet; }

private:
	// 创建样式集
	static TSharedRef<FSlateStyleSet> Create();

	// 样式集名称
	static FName StyleSetName;

	// 样式集实例
	static TSharedPtr<FSlateStyleSet> StyleSet;
};



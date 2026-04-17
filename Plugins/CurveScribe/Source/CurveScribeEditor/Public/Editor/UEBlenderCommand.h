#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Style/BlenderPluginStyle.h"

/**
 * UEBlender 编辑器命令
 * 定义编辑器工具栏和菜单中的按钮命令
 */
class FUEBlenderCommand : public TCommands<FUEBlenderCommand>
{
public:
	FUEBlenderCommand()
		: TCommands<FUEBlenderCommand>(
			FBlenderPluginStyle::GetStyleSetName(),                 // 上下文名称（必须与样式名称一致才能找到图标）
			NSLOCTEXT("BlenderPlugin", "BlenderPlugin", "Blender Plugin Commands"),  // 友好名称
			NAME_None,                                              // 父级上下文
			FBlenderPluginStyle::GetStyleSetName()                  // 样式集名称
		)
	{
	}

	// 注册命令
	virtual void RegisterCommands() override;

	// Blender 窗口打开命令
	TSharedPtr<FUICommandInfo> OpenBlenderWindow;
};

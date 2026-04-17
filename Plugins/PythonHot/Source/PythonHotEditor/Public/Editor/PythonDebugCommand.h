#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Style/PythonHotEditorStyle.h"

/**
 * PythonDebug 编辑器命令
 * 定义编辑器工具栏和菜单中的按钮命令
 */
class FPythonDebugCommand : public TCommands<FPythonDebugCommand>
{
public:
	FPythonDebugCommand()
		: TCommands<FPythonDebugCommand>(
			"PythonHotEditor",                      // 上下文名称
			NSLOCTEXT("PythonHotEditor", "PythonHotEditor", "Python Debug Commands"),  // 友好名称
			NAME_None,                               // 父级上下文
			"PythonHotEditor"                       // 样式集名称
		)
	{
	}

	// 注册命令
	virtual void RegisterCommands() override;

	// Python Debug 窗口打开命令
	TSharedPtr<FUICommandInfo> OpenPythonDebugWindow;
};
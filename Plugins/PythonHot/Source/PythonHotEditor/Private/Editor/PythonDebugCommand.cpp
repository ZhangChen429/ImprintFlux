#include "Editor/PythonDebugCommand.h"
#include "Style/PythonHotEditorStyle.h"

#define LOCTEXT_NAMESPACE "PythonDebugCommands"

void FPythonDebugCommand::RegisterCommands()
{
	UI_COMMAND(
		OpenPythonDebugWindow,            // 成员变量名
		"Python Debug",                    // 按钮显示文本
		"Open Python Debug Window",       // 工具提示
		EUserInterfaceActionType::Button, // 类型
		FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::P)  // 快捷键 Ctrl+Shift+P
	);
}

#undef LOCTEXT_NAMESPACE
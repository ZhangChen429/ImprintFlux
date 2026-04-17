#include "Editor/UEBlenderCommand.h"
#include "Style/BlenderPluginStyle.h"

#define LOCTEXT_NAMESPACE "BlenderPluginCommands"

void FUEBlenderCommand::RegisterCommands()
{
	UI_COMMAND(
		OpenBlenderWindow,               // 成员变量名
		"Blender",                       // 按钮显示文本
		"Open Blender Plugin Window",    // 工具提示
		EUserInterfaceActionType::Button,// 类型
		FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::B)  // 快捷键 Ctrl+Shift+B
	);
}

#undef LOCTEXT_NAMESPACE
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPythonHotEditor, Log, All);

/**
 * PythonHot 编辑器模块
 * 提供 Python 调试按钮和 UI 功能
 */
class FPythonHotEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // 注册编辑器菜单和工具栏
    void RegisterMenus();

    // 注册 DockTab
    void RegisterDockTab();

    // PythonDebug 按钮点击事件
     void OnPythonDebugButtonClicked();

    // 生成 Python Debug 窗口 Tab
    TSharedRef<SDockTab> OnSpawnPythonDebugTab(const class FSpawnTabArgs& Args);

    // 命令列表
    TSharedPtr<FUICommandList> PythonDebugCommands;

    // Tab 名称
    static const FName PythonDebugTabName;
};
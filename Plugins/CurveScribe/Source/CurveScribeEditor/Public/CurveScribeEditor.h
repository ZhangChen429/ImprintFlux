#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlenderLiveLink;
class FMonitoredProcess;
class FAlterMeshBridge;
class FBezierCurveVisualizer;
DECLARE_LOG_CATEGORY_EXTERN(LogCurveScribeTool, Log, All);

class FCurveScribeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    
    // ===== Bezier Curve Tool =====
    // 注册贝塞尔曲线工具 DockTab
    void RegisterBezierCurveTab();

    // 贝塞尔曲线按钮点击事件
    void OnBezierCurveButtonClicked();

    // 生成贝塞尔曲线工具窗口 Tab
    TSharedRef<SDockTab> OnSpawnBezierCurveTab(const FSpawnTabArgs& Args);

    // 贝塞尔曲线命令列表
    TSharedPtr<FUICommandList> BezierCurveCommands;

    // 贝塞尔曲线 Tab 名称
    static const FName BezierCurveTabName;

    // 贝塞尔曲线可视化工具
    TSharedPtr<FBezierCurveVisualizer> BezierCurveVisualizer;
};

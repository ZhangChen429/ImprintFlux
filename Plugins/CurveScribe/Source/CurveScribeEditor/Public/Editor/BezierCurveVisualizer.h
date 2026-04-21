#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

class UCurveScribeScene;

/**
 * 控制点 Hit Proxy，用于检测鼠标点击
 */
struct HBezierControlPointHitProxy : public HComponentVisProxy
{
    DECLARE_HIT_PROXY();

    HBezierControlPointHitProxy(const UActorComponent* InComponent, int32 InControlPointIndex)
        : HComponentVisProxy(InComponent, HPP_Wireframe)
        , ControlPointIndex(InControlPointIndex)
    {}

    int32 ControlPointIndex;
};

/**
 * 贝塞尔曲线组件可视化工具
 * 负责：绘制控制点线框、处理点击选中、处理拖拽移动
 * 直接操作 UCurveScribeScene，不再通过 ACurveScribeActor 中转
 */
class FBezierCurveVisualizer : public FComponentVisualizer
{
public:
    virtual ~FBezierCurveVisualizer() override = default;

    // 绘制可视化（控制点线框）
    virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;

    // 处理点击事件
    virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;

    // 获取 Widget 位置（用于显示 W/E/R 变换工具）
    virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;

    // 处理输入变换（拖拽移动）
    virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;

    // 获取当前选中的控制点索引
    int32 GetSelectedControlPointIndex() const { return SelectedPointIndex; }

private:
    // 当前选中的 Scene 和控制点
    TWeakObjectPtr<UCurveScribeScene> SelectedScene;
    int32 SelectedPointIndex = INDEX_NONE;

    // 控制点半径
    static constexpr float ControlPointRadius = 50.0f;

    // 绘制单个控制点
    void DrawControlPoint(FPrimitiveDrawInterface* PDI, const UActorComponent* Component, const FVector& Location, int32 Index, bool bSelected, const FSceneView* View);
};

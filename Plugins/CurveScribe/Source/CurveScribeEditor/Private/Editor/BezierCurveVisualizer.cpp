#include "Editor/BezierCurveVisualizer.h"
#include "CurveScribeScene.h"
#include "Components/SplineComponent.h"
#include "EditorViewportClient.h"
#include "SceneManagement.h"
#include "Editor.h"

// 定义 Hit Proxy
IMPLEMENT_HIT_PROXY(HBezierControlPointHitProxy, HComponentVisProxy);

namespace
{
    // 从 visualizer 收到的 Component 解析出 Scene，并校验有效性
    const UCurveScribeScene* ResolveScene(const UActorComponent* Component)
    {
        if (!Component || !IsValid(Component)) return nullptr;
        return Cast<UCurveScribeScene>(Component);
    }
}

void FBezierCurveVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    const UCurveScribeScene* Scene = ResolveScene(Component);
    if (!Scene) return;

    // 控制点视为 Scene 本地空间，绘制时通过 Scene 的世界变换映射到世界
    const FTransform SceneXform = Scene->GetComponentTransform();

    // 控制点之间的连线（虚线）
    if (Scene->ControlPoints.Num() > 1)
    {
        for (int32 i = 0; i < Scene->ControlPoints.Num() - 1; ++i)
        {
            const FVector Start = SceneXform.TransformPosition(Scene->ControlPoints[i]);
            const FVector End   = SceneXform.TransformPosition(Scene->ControlPoints[i + 1]);
            PDI->DrawLine(Start, End, FLinearColor(0.3f, 0.3f, 0.3f, 0.5f), SDPG_Foreground, 1.0f, 0.5f);
        }
    }

    // 每个控制点
    for (int32 i = 0; i < Scene->ControlPoints.Num(); ++i)
    {
        const FVector Location = SceneXform.TransformPosition(Scene->ControlPoints[i]);
        const bool bSelected = (Scene->SelectedControlPointIndex == i);
        DrawControlPoint(PDI, Component, Location, i, bSelected, View);
    }
}

void FBezierCurveVisualizer::DrawControlPoint(FPrimitiveDrawInterface* PDI, const UActorComponent* Component, const FVector& Location, int32 Index, bool bSelected, const FSceneView* View)
{
    PDI->SetHitProxy(new HBezierControlPointHitProxy(Component, Index));

    const FLinearColor Color = bSelected ? FLinearColor::Yellow : FLinearColor(0.0f, 0.5f, 1.0f);
    DrawWireSphereAutoSides(PDI, Location, Color, ControlPointRadius, SDPG_Foreground, 2.0f);
    PDI->DrawPoint(Location, Color, 8.0f, SDPG_Foreground);

    PDI->SetHitProxy(nullptr);
}

bool FBezierCurveVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
    if (HBezierControlPointHitProxy* Proxy = static_cast<HBezierControlPointHitProxy*>(VisProxy))
    {
        if (VisProxy->Component.IsValid())
        {
            UCurveScribeScene* Scene = const_cast<UCurveScribeScene*>(ResolveScene(VisProxy->Component.Get()));
            if (Scene
                && Proxy->ControlPointIndex >= 0
                && Proxy->ControlPointIndex < Scene->ControlPoints.Num())
            {
                Scene->SelectedControlPointIndex = Proxy->ControlPointIndex;
                SelectedScene = Scene;
                SelectedPointIndex = Proxy->ControlPointIndex;

                GEditor->RedrawAllViewports();
                return true;
            }
        }
    }

    // 点击空白处取消选择
    if (SelectedScene.IsValid())
    {
        SelectedScene->SelectedControlPointIndex = INDEX_NONE;
        SelectedScene.Reset();
        SelectedPointIndex = INDEX_NONE;
        GEditor->RedrawAllViewports();
    }

    return false;
}

bool FBezierCurveVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
    if (!SelectedScene.IsValid() || SelectedPointIndex == INDEX_NONE) return false;
    if (SelectedPointIndex >= SelectedScene->ControlPoints.Num()) return false;
    if (!SelectedScene->bShow3DWidget) return false;

    OutLocation = SelectedScene->GetComponentTransform().TransformPosition(SelectedScene->ControlPoints[SelectedPointIndex]);
    return true;
}

bool FBezierCurveVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
    if (DeltaTranslate.IsNearlyZero()) return false;
    if (!SelectedScene.IsValid() || SelectedPointIndex == INDEX_NONE) return false;
    if (SelectedPointIndex >= SelectedScene->ControlPoints.Num()) return false;

    SelectedScene->Modify();

    // World-space delta → Scene 本地空间增量（处理 Scene 旋转/缩放）
    const FVector LocalDelta = SelectedScene->GetComponentTransform().InverseTransformVector(DeltaTranslate);
    SelectedScene->ControlPoints[SelectedPointIndex] += LocalDelta;

    SelectedScene->RebuildCurve();
    SelectedScene->NotifyControlPointsChanged();
    GEditor->RedrawAllViewports();
    return true;
}

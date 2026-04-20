#include "Editor/BezierCurveVisualizer.h"
#include "CurveScribeActor.h"
#include "CurveScribeScene.h"
#include "Components/SplineComponent.h"
#include "EditorViewportClient.h"
#include "SceneManagement.h"
#include "Editor.h"

// 定义 Hit Proxy
IMPLEMENT_HIT_PROXY(HBezierControlPointHitProxy, HComponentVisProxy);

namespace
{
    // 从 visualizer 收到的 Component（Scene）解析出 actor，并校验有效性
    bool ResolveSceneAndActor(const UActorComponent* Component, const UCurveScribeScene*& OutScene, ACurveScribeActor*& OutActor)
    {
        OutScene = nullptr;
        OutActor = nullptr;
        if (!Component || !IsValid(Component)) return false;

        const UCurveScribeScene* Scene = Cast<UCurveScribeScene>(Component);
        if (!Scene) return false;

        ACurveScribeActor* Actor = Cast<ACurveScribeActor>(Scene->GetOwner());
        if (!Actor || !IsValid(Actor)) return false;

        OutScene = Scene;
        OutActor = Actor;
        return true;
    }
}

void FBezierCurveVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    const UCurveScribeScene* Scene = nullptr;
    ACurveScribeActor* Actor = nullptr;
    if (!ResolveSceneAndActor(Component, Scene, Actor)) return;

    // 控制点视为 Scene 本地空间，绘制时通过 Scene 的世界变换映射到世界
    const FTransform SceneXform = Scene->GetComponentTransform();

    // 控制点之间的连线（虚线）
    if (Actor->ControlPoints.Num() > 1)
    {
        for (int32 i = 0; i < Actor->ControlPoints.Num() - 1; ++i)
        {
            const FVector Start = SceneXform.TransformPosition(Actor->ControlPoints[i]);
            const FVector End   = SceneXform.TransformPosition(Actor->ControlPoints[i + 1]);
            PDI->DrawLine(Start, End, FLinearColor(0.3f, 0.3f, 0.3f, 0.5f), SDPG_Foreground, 1.0f, 0.5f);
        }
    }

    // 每个控制点
    for (int32 i = 0; i < Actor->ControlPoints.Num(); ++i)
    {
        const FVector Location = SceneXform.TransformPosition(Actor->ControlPoints[i]);
        const bool bSelected = (Actor->SelectedControlPointIndex == i);
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
            const UCurveScribeScene* Scene = nullptr;
            ACurveScribeActor* Actor = nullptr;
            if (ResolveSceneAndActor(VisProxy->Component.Get(), Scene, Actor)
                && Proxy->ControlPointIndex >= 0
                && Proxy->ControlPointIndex < Actor->ControlPoints.Num())
            {
                Actor->SelectedControlPointIndex = Proxy->ControlPointIndex;
                SelectedActor = Actor;
                SelectedPointIndex = Proxy->ControlPointIndex;

                GEditor->RedrawAllViewports();
                return true;
            }
        }
    }

    // 点击空白处取消选择
    if (SelectedActor.IsValid())
    {
        SelectedActor->SelectedControlPointIndex = INDEX_NONE;
        SelectedActor.Reset();
        SelectedPointIndex = INDEX_NONE;
        GEditor->RedrawAllViewports();
    }

    return false;
}

bool FBezierCurveVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
    if (!SelectedActor.IsValid() || SelectedPointIndex == INDEX_NONE) return false;
    if (SelectedPointIndex >= SelectedActor->ControlPoints.Num()) return false;
    if (!SelectedActor->bShow3DWidget) return false;

    const UCurveScribeScene* Scene = SelectedActor->CurveTargetScene;
    if (!Scene) return false;

    OutLocation = Scene->GetComponentTransform().TransformPosition(SelectedActor->ControlPoints[SelectedPointIndex]);
    return true;
}

bool FBezierCurveVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
    if (DeltaTranslate.IsNearlyZero()) return false;
    if (!SelectedActor.IsValid() || SelectedPointIndex == INDEX_NONE) return false;
    if (SelectedPointIndex >= SelectedActor->ControlPoints.Num()) return false;

    const UCurveScribeScene* Scene = SelectedActor->CurveTargetScene;
    if (!Scene) return false;

    SelectedActor->Modify();

    // World-space delta → Scene 本地空间增量（处理 Scene 旋转/缩放）
    const FVector LocalDelta = Scene->GetComponentTransform().InverseTransformVector(DeltaTranslate);
    SelectedActor->ControlPoints[SelectedPointIndex] += LocalDelta;

    SelectedActor->NotifyControlPointsChanged();
    GEditor->RedrawAllViewports();
    return true;
}

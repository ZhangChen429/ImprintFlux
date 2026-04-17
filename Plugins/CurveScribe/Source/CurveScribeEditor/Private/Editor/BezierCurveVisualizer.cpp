#include "Editor/BezierCurveVisualizer.h"
#include "BezierCurveActor.h"
#include "CurveTargetScene.h"
#include "Components/SplineComponent.h"
#include "EditorViewportClient.h"
#include "SceneManagement.h"
#include "Editor.h"

// 定义 Hit Proxy
IMPLEMENT_HIT_PROXY(HBezierControlPointHitProxy, HComponentVisProxy);

void FBezierCurveVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    if (!Component || !IsValid(Component))
        return;

    AActor* Owner = Component->GetOwner();
    if (!Owner || !IsValid(Owner))
        return;

    const ABezierCurveActor* Actor = Cast<ABezierCurveActor>(Owner);
    if (!Actor)
        return;

    const FVector ActorLocation = Actor->GetActorLocation();

    // 绘制控制点之间的连线（虚线）
    if (Actor->ControlPoints.Num() > 1)
    {
        for (int32 i = 0; i < Actor->ControlPoints.Num() - 1; ++i)
        {
            FVector Start = ActorLocation + Actor->ControlPoints[i];
            FVector End = ActorLocation + Actor->ControlPoints[i + 1];
            PDI->DrawLine(Start, End, FLinearColor(0.3f, 0.3f, 0.3f, 0.5f), SDPG_Foreground, 1.0f, 0.5f);
        }
    }

    // 绘制每个控制点
    for (int32 i = 0; i < Actor->ControlPoints.Num(); ++i)
    {
        const FVector Location = ActorLocation + Actor->ControlPoints[i];
        const bool bSelected = (Actor->SelectedControlPointIndex == i);

        DrawControlPoint(PDI, Component, Location, i, bSelected, View);
    }
}

void FBezierCurveVisualizer::DrawControlPoint(FPrimitiveDrawInterface* PDI, const UActorComponent* Component, const FVector& Location, int32 Index, bool bSelected, const FSceneView* View)
{
    // 设置 Hit Proxy，让点击可以检测到这个控制点（必须传入 Component）
    PDI->SetHitProxy(new HBezierControlPointHitProxy(Component, Index));

    // 根据选中状态选择颜色
    FLinearColor Color = bSelected ? FLinearColor::Yellow : FLinearColor(0.0f, 0.5f, 1.0f);

    // 绘制线框球体
    DrawWireSphereAutoSides(PDI, Location, Color, ControlPointRadius, SDPG_Foreground, 2.0f);

    // 绘制中心点
    PDI->DrawPoint(Location, Color, 8.0f, SDPG_Foreground);

    // 绘制索引文字（在球体上方）
    FString IndexText = FString::Printf(TEXT("%d"), Index);
    // 注意：文字绘制需要在 DrawVisualization 中通过其他方式实现，这里简化处理

    PDI->SetHitProxy(nullptr);
}

bool FBezierCurveVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
    if (HBezierControlPointHitProxy* Proxy = static_cast<HBezierControlPointHitProxy*>(VisProxy))
    {
        // 通过组件获取 Actor
        if (VisProxy->Component.IsValid())
        {
            ABezierCurveActor* Actor = Cast<ABezierCurveActor>(VisProxy->Component->GetOwner());
            if (Actor && Proxy->ControlPointIndex >= 0
                && Proxy->ControlPointIndex < Actor->ControlPoints.Num())
            {
                // 更新选中状态
                Actor->SelectedControlPointIndex = Proxy->ControlPointIndex;
                SelectedActor = Actor;
                SelectedPointIndex = Proxy->ControlPointIndex;

                // 刷新视口
                GEditor->RedrawAllViewports();

                return true;
            }
        }
    }

    // 点击空白处，取消选择
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
    if (SelectedActor.IsValid() && SelectedPointIndex != INDEX_NONE
        && SelectedPointIndex < SelectedActor->ControlPoints.Num())
    {
        // 检查是否启用 3D Widget
        if (!SelectedActor->bShow3DWidget)
        {
            return false;
        }
        OutLocation = SelectedActor->GetActorLocation() + SelectedActor->ControlPoints[SelectedPointIndex];
        return true;
    }
    return false;
}

bool FBezierCurveVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
    // 只处理平移，控制点不需要旋转和缩放
    if (DeltaTranslate.IsNearlyZero())
    {
        return false;
    }

    if (SelectedActor.IsValid() && SelectedPointIndex != INDEX_NONE
        && SelectedPointIndex < SelectedActor->ControlPoints.Num())
    {
        // 标记修改，支持撤销（必须在修改数据之前调用）
        SelectedActor->Modify();

        // 更新控制点位置
        SelectedActor->ControlPoints[SelectedPointIndex] += DeltaTranslate;

        // 通过委托通知刷新样条线
        SelectedActor->NotifyControlPointsChanged();

        // 刷新视口
        GEditor->RedrawAllViewports();

        return true;
    }

    return false;
}

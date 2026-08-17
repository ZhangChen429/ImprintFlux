#include "Editor/FCurveScribeDataAssetViewportClient.h"
#include "Editor/CurveScribeDataAssetEditorToolkit.h"
#include "CurveScribeScene.h"
#include "CurveScribeActor.h"
#include "AdvancedPreviewScene.h"
#include "SceneManagement.h"
#include "Engine/World.h"

IMPLEMENT_HIT_PROXY(HCurveScribeControlPointProxy, HHitProxy);

HCurveScribeControlPointProxy::HCurveScribeControlPointProxy(int32 InIndex)
    : HHitProxy(HPP_Wireframe)
    , ControlPointIndex(InIndex)
{
}

FCurveScribeDataAssetViewportClient::FCurveScribeDataAssetViewportClient(FAdvancedPreviewScene& InPreviewScene, const TSharedRef<FCurveScribeDataAssetEditorToolkit>& InToolkit)
    : FEditorViewportClient(nullptr, &InPreviewScene)
    , ToolkitWeak(InToolkit)
    , PreviewScene(&InPreviewScene)
{
    SetRealtime(true);
    SetViewMode(VMI_Lit);
    SetWidgetMode(UE::Widget::WM_Translate);
    SetWidgetCoordSystemSpace(COORD_World);
    bDrawAxes = false;
}

void FCurveScribeDataAssetViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);

    if (PreviewScene)
    {
        PreviewScene->Tick(DeltaSeconds);
    }
}

void FCurveScribeDataAssetViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
    FEditorViewportClient::Draw(View, PDI);

    UCurveScribeDataAsset* Asset = GetEditingAsset();
    if (!Asset || Asset->ControlPoints.Num() == 0)
    {
        return;
    }

    const UCurveScribeScene* Scene = GetPreviewSceneComponent();
    const FTransform SceneTransform = Scene ? Scene->GetComponentTransform() : FTransform::Identity;

    // 控制点之间的连线
    for (int32 i = 0; i < Asset->ControlPoints.Num() - 1; ++i)
    {
        const FVector Start = SceneTransform.TransformPosition(Asset->ControlPoints[i]);
        const FVector End = SceneTransform.TransformPosition(Asset->ControlPoints[i + 1]);
        PDI->DrawLine(Start, End, FLinearColor(0.3f, 0.3f, 0.3f, 0.5f), SDPG_Foreground, 1.0f, 0.5f);
    }

    // 控制点
    for (int32 i = 0; i < Asset->ControlPoints.Num(); ++i)
    {
        const FVector WorldLocation = SceneTransform.TransformPosition(Asset->ControlPoints[i]);
        const bool bSelected = (i == SelectedControlPointIndex);
        const FLinearColor Color = bSelected ? FLinearColor::Yellow : FLinearColor(0.0f, 0.5f, 1.0f);

        PDI->SetHitProxy(new HCurveScribeControlPointProxy(i));
        DrawWireSphereAutoSides(PDI, WorldLocation, Color, 20.0f, SDPG_Foreground, 2.0f);
        PDI->DrawPoint(WorldLocation, Color, 8.0f, SDPG_Foreground);
        PDI->SetHitProxy(nullptr);
    }
}

bool FCurveScribeDataAssetViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    if (EventArgs.Key == EKeys::LeftMouseButton && EventArgs.Event == IE_Pressed)
    {
        const int32 MouseX = EventArgs.Viewport->GetMouseX();
        const int32 MouseY = EventArgs.Viewport->GetMouseY();

        HHitProxy* HitProxy = EventArgs.Viewport->GetHitProxy(MouseX, MouseY);
        if (HCurveScribeControlPointProxy* Proxy = HitProxyCast<HCurveScribeControlPointProxy>(HitProxy))
        {
            SetSelectedControlPoint(Proxy->ControlPointIndex);
            return true;
        }
        else
        {
            SetSelectedControlPoint(INDEX_NONE);
        }
    }

    return FEditorViewportClient::InputKey(EventArgs);
}

bool FCurveScribeDataAssetViewportClient::InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale)
{
    if (SelectedControlPointIndex == INDEX_NONE)
    {
        return false;
    }

    if (Drag.IsNearlyZero())
    {
        return false;
    }

    UCurveScribeDataAsset* Asset = GetEditingAsset();
    if (!Asset || !Asset->ControlPoints.IsValidIndex(SelectedControlPointIndex))
    {
        return false;
    }

    const UCurveScribeScene* Scene = GetPreviewSceneComponent();
    const FTransform SceneTransform = Scene ? Scene->GetComponentTransform() : FTransform::Identity;
    const FVector LocalDelta = SceneTransform.InverseTransformVector(Drag);

    Asset->Modify();
    Asset->ControlPoints[SelectedControlPointIndex] += LocalDelta;
    Asset->OnDataChanged.Broadcast();

    return true;
}

void FCurveScribeDataAssetViewportClient::TrackingStarted(const FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge)
{
    bIsTracking = true;
    FEditorViewportClient::TrackingStarted(InInputState, bIsDraggingWidget, bNudge);
}

void FCurveScribeDataAssetViewportClient::TrackingStopped()
{
    bIsTracking = false;
    FEditorViewportClient::TrackingStopped();
}

FVector FCurveScribeDataAssetViewportClient::GetWidgetLocation() const
{
    if (SelectedControlPointIndex == INDEX_NONE)
    {
        return FVector::ZeroVector;
    }
    return GetControlPointWorldLocation(SelectedControlPointIndex);
}

void FCurveScribeDataAssetViewportClient::SetSelectedControlPoint(int32 InIndex)
{
    if (SelectedControlPointIndex != InIndex)
    {
        SelectedControlPointIndex = InIndex;
        ShowWidget(SelectedControlPointIndex != INDEX_NONE);
        Invalidate();
    }
}

void FCurveScribeDataAssetViewportClient::FocusViewportOnCurve()
{
    UCurveScribeDataAsset* Asset = GetEditingAsset();
    if (!Asset || Asset->ControlPoints.Num() == 0)
    {
        return;
    }

    const UCurveScribeScene* Scene = GetPreviewSceneComponent();
    const FTransform SceneTransform = Scene ? Scene->GetComponentTransform() : FTransform::Identity;

    FVector Center = FVector::ZeroVector;
    for (const FVector& Point : Asset->ControlPoints)
    {
        Center += SceneTransform.TransformPosition(Point);
    }
    Center /= Asset->ControlPoints.Num();

    float Radius = 0.0f;
    for (const FVector& Point : Asset->ControlPoints)
    {
        Radius = FMath::Max(Radius, FVector::Dist(Center, SceneTransform.TransformPosition(Point)));
    }
    Radius = FMath::Max(Radius, 100.0f);

    const FVector CameraLocation = Center + FVector(-Radius * 2.0f, Radius * 2.0f, Radius);
    const FRotator CameraRotation = (Center - CameraLocation).Rotation();

    SetViewLocation(CameraLocation);
    SetViewRotation(CameraRotation);
}

UCurveScribeDataAsset* FCurveScribeDataAssetViewportClient::GetEditingAsset() const
{
    TSharedPtr<FCurveScribeDataAssetEditorToolkit> Toolkit = ToolkitWeak.Pin();
    return Toolkit ? Toolkit->GetEditingAsset() : nullptr;
}

UCurveScribeScene* FCurveScribeDataAssetViewportClient::GetPreviewSceneComponent() const
{
    TSharedPtr<FCurveScribeDataAssetEditorToolkit> Toolkit = ToolkitWeak.Pin();
    return Toolkit ? Toolkit->GetPreviewSceneComponent() : nullptr;
}

FVector FCurveScribeDataAssetViewportClient::GetControlPointWorldLocation(int32 InIndex) const
{
    UCurveScribeDataAsset* Asset = GetEditingAsset();
    if (!Asset || !Asset->ControlPoints.IsValidIndex(InIndex))
    {
        return FVector::ZeroVector;
    }

    const UCurveScribeScene* Scene = GetPreviewSceneComponent();
    const FTransform SceneTransform = Scene ? Scene->GetComponentTransform() : FTransform::Identity;
    return SceneTransform.TransformPosition(Asset->ControlPoints[InIndex]);
}

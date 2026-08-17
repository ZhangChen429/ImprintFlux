#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "CurveScribeDataAsset.h"

class FCurveScribeDataAssetEditorToolkit;
class FAdvancedPreviewScene;
class UCurveScribeScene;

/**
 * 控制点命中代理
 */
struct HCurveScribeControlPointProxy : public HHitProxy
{
    DECLARE_HIT_PROXY();

    int32 ControlPointIndex;

    HCurveScribeControlPointProxy(int32 InIndex);
};

/**
 * CurveScribe 数据资产编辑器的 3D 预览视口 Client
 * 负责绘制控制点、处理选中 / 拖拽、与数据资产同步
 */
class CURVESCRIBEEDITOR_API FCurveScribeDataAssetViewportClient : public FEditorViewportClient
{
public:
    FCurveScribeDataAssetViewportClient(FAdvancedPreviewScene& InPreviewScene, const TSharedRef<FCurveScribeDataAssetEditorToolkit>& InToolkit);

    // FEditorViewportClient interface
    virtual void Tick(float DeltaSeconds) override;
    virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual bool InputWidgetDelta(FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale) override;
    virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge) override;
    virtual void TrackingStopped() override;
    virtual FVector GetWidgetLocation() const override;

    void SetSelectedControlPoint(int32 InIndex);
    int32 GetSelectedControlPointIndex() const { return SelectedControlPointIndex; }

    /** 将相机聚焦到整个曲线内容 */
    void FocusViewportOnCurve();

private:
    TWeakPtr<FCurveScribeDataAssetEditorToolkit> ToolkitWeak;
    FAdvancedPreviewScene* PreviewScene;

    int32 SelectedControlPointIndex = INDEX_NONE;
    bool bIsTracking = false;

    UCurveScribeDataAsset* GetEditingAsset() const;
    UCurveScribeScene* GetPreviewSceneComponent() const;
    FVector GetControlPointWorldLocation(int32 InIndex) const;
};

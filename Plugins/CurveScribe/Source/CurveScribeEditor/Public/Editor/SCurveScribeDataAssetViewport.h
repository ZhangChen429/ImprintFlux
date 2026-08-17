#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FCurveScribeDataAssetEditorToolkit;
class FCurveScribeDataAssetViewportClient;
class FAdvancedPreviewScene;

/**
 * CurveScribe 数据资产编辑器的 3D 预览 Slate 视口
 */
class CURVESCRIBEEDITOR_API SCurveScribeDataAssetViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SCurveScribeDataAssetViewport) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<FCurveScribeDataAssetEditorToolkit>& InToolkit, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene);

    TSharedPtr<FAdvancedPreviewScene> GetPreviewScene() const { return PreviewScene; }
    TSharedPtr<FCurveScribeDataAssetViewportClient> GetViewportClient() const { return ViewportClient; }

protected:
    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
    virtual void OnFocusViewportToSelection() override;

private:
    TWeakPtr<FCurveScribeDataAssetEditorToolkit> ToolkitWeak;
    TSharedPtr<FAdvancedPreviewScene> PreviewScene;
    TSharedPtr<FCurveScribeDataAssetViewportClient> ViewportClient;
};

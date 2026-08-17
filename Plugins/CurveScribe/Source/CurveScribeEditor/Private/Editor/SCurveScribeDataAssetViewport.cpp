#include "Editor/SCurveScribeDataAssetViewport.h"
#include "Editor/FCurveScribeDataAssetViewportClient.h"
#include "Editor/CurveScribeDataAssetEditorToolkit.h"

void SCurveScribeDataAssetViewport::Construct(const FArguments& InArgs, const TSharedRef<FCurveScribeDataAssetEditorToolkit>& InToolkit, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene)
{
    ToolkitWeak = InToolkit;
    PreviewScene = InPreviewScene;

    SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SCurveScribeDataAssetViewport::MakeEditorViewportClient()
{
    check(PreviewScene.IsValid());
    ViewportClient = MakeShareable(new FCurveScribeDataAssetViewportClient(*PreviewScene, ToolkitWeak.Pin().ToSharedRef()));
    return ViewportClient.ToSharedRef();
}

void SCurveScribeDataAssetViewport::OnFocusViewportToSelection()
{
    if (ViewportClient.IsValid())
    {
        ViewportClient->FocusViewportOnCurve();
    }
}

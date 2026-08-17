#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UCurveScribeDataAsset;
class IDetailsView;
class SDockTab;
class FCurveEditor;
class SCurveEditorPanel;
class SCurveScribeDataAssetViewport;
class FAdvancedPreviewScene;
class ACurveScribeActor;
class UCurveScribeScene;

/**
 * UCurveScribeDataAsset 的独立资产编辑器窗口。
 * 包含 Details Tab、CurveEditor Tab（编辑 TubeScaleCurve）以及 3D 预览视口 Tab。
 */
class CURVESCRIBEEDITOR_API FCurveScribeDataAssetEditorToolkit : public FAssetEditorToolkit
{
public:
    virtual ~FCurveScribeDataAssetEditorToolkit();

    void InitEditor(EToolkitMode::Type Mode,
                    const TSharedPtr<IToolkitHost>& InitToolkitHost,
                    UCurveScribeDataAsset* InAsset);

    // FAssetEditorToolkit
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;

    // 供 ViewportClient 访问
    UCurveScribeDataAsset* GetEditingAsset() const { return EditingAsset; }
    ACurveScribeActor* GetPreviewActor() const { return PreviewActor; }
    UCurveScribeScene* GetPreviewSceneComponent() const;
    TSharedPtr<FAdvancedPreviewScene> GetPreviewScene() const { return PreviewScene; }

    /** 数据资产变更时由 ViewportClient 或 Detail 面板触发 */
    void OnAssetDataChanged();

private:
    TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_Curve(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);

    void CreatePreviewSceneAndActor();
    void DestroyPreviewActor();
    void SyncAssetToPreviewActor();

    TObjectPtr<UCurveScribeDataAsset> EditingAsset = nullptr;
    TSharedPtr<IDetailsView> DetailsView;

    TSharedPtr<FCurveEditor> CurveEditor;
    TSharedPtr<SCurveEditorPanel> CurveEditorPanel;

    TSharedPtr<SCurveScribeDataAssetViewport> ViewportWidget;
    TSharedPtr<FAdvancedPreviewScene> PreviewScene;
    TObjectPtr<ACurveScribeActor> PreviewActor = nullptr;

    FDelegateHandle OnAssetChangedHandle;

    static const FName DetailsTabId;
    static const FName CurveTabId;
    static const FName ViewportTabId;
};

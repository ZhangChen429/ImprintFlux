#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UCurveScribeDataAsset;
class IDetailsView;
class SDockTab;
class FCurveEditor;
class SCurveEditorPanel;

/**
 * UCurveScribeDataAsset 的独立资产编辑器窗口。
 * Step 1：Details Tab；Step 2：+ CurveEditor Tab（编辑 TubeScaleCurve）。
 */
class CURVESCRIBEEDITOR_API FCurveScribeDataAssetEditorToolkit : public FAssetEditorToolkit
{
public:
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

private:
    TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_Curve(const FSpawnTabArgs& Args);

    TObjectPtr<UCurveScribeDataAsset> EditingAsset = nullptr;
    TSharedPtr<IDetailsView> DetailsView;

    TSharedPtr<FCurveEditor> CurveEditor;
    TSharedPtr<SCurveEditorPanel> CurveEditorPanel;

    static const FName DetailsTabId;
    static const FName CurveTabId;
};

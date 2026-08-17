#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class IDetailsView;
class SDockTab;
class SFormScribeViewport;
class UFormScribeDataAsset;

/** FormScribe DataAsset 的独立资产编辑器。 */
class FORMSCRIBEEDITOR_API FFormScribeDataAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	void InitEditor(
		EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		UFormScribeDataAsset* InAsset);

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	TObjectPtr<UFormScribeDataAsset> EditingAsset = nullptr;
	TSharedPtr<SFormScribeViewport> ViewportWidget;
	TSharedPtr<IDetailsView> DetailsView;

	static const FName ViewportTabId;
	static const FName DetailsTabId;
};

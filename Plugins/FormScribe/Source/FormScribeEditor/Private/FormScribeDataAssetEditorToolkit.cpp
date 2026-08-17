#include "FormScribeDataAssetEditorToolkit.h"

#include "FormScribeDataAsset.h"
#include "FormScribeViewport.h"
#include "Framework/Docking/TabManager.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FormScribeDataAssetEditor"

const FName FFormScribeDataAssetEditorToolkit::ViewportTabId(TEXT("FormScribeDataAssetEditor_Viewport"));
const FName FFormScribeDataAssetEditorToolkit::DetailsTabId(TEXT("FormScribeDataAssetEditor_Details"));

void FFormScribeDataAssetEditorToolkit::InitEditor(
	EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UFormScribeDataAsset* InAsset)
{
	EditingAsset = InAsset;

	const TSharedRef<FTabManager::FLayout> Layout =
		FTabManager::NewLayout(TEXT("FormScribeDataAssetEditor_Layout_v1"))
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.72f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
				->SetHideTabWell(true))
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.28f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)));

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		TEXT("FormScribeDataAssetEditorApp"),
		Layout,
		true,
		true,
		InAsset);
}

void FFormScribeDataAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
		LOCTEXT("WorkspaceMenu", "FormScribe Data Asset Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(
		ViewportTabId,
		FOnSpawnTab::CreateSP(this, &FFormScribeDataAssetEditorToolkit::SpawnViewportTab))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("LevelEditor.Tabs.Viewports")));

	InTabManager->RegisterTabSpawner(
		DetailsTabId,
		FOnSpawnTab::CreateSP(this, &FFormScribeDataAssetEditorToolkit::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("LevelEditor.Tabs.Details")));
}

void FFormScribeDataAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
}

FName FFormScribeDataAssetEditorToolkit::GetToolkitFName() const
{
	return TEXT("FormScribeDataAssetEditor");
}

FText FFormScribeDataAssetEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("BaseToolkitName", "FormScribe Data Asset Editor");
}

FString FFormScribeDataAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "FormScribe ").ToString();
}

FLinearColor FFormScribeDataAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.25f, 0.75f, 0.55f, 0.5f);
}

TSharedRef<SDockTab> FFormScribeDataAssetEditorToolkit::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	SAssignNew(ViewportWidget, SFormScribeViewport)
		.Asset(EditingAsset);

	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab", "Viewport"))
		[
			ViewportWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FFormScribeDataAssetEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
	DetailsView->SetObject(EditingAsset);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

#undef LOCTEXT_NAMESPACE

#include "Editor/CurveScribeDataAssetEditorToolkit.h"
#include "CurveScribeDataAsset.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "CurveEditor.h"
#include "SCurveEditorPanel.h"
#include "RichCurveEditorModel.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "CurveScribeDataAssetEditor"

const FName FCurveScribeDataAssetEditorToolkit::DetailsTabId(TEXT("CurveScribeDataAssetEditor_Details"));
const FName FCurveScribeDataAssetEditorToolkit::CurveTabId(TEXT("CurveScribeTubeDataAssetEditor_Curve"));

void FCurveScribeDataAssetEditorToolkit::InitEditor(EToolkitMode::Type Mode,
                                                   const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                   UCurveScribeDataAsset* InAsset)
{
    EditingAsset = InAsset;

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("CurveScribeDataAssetEditor_Layout_v2")
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split(
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.35f)
                ->AddTab(DetailsTabId, ETabState::OpenedTab)
                ->SetHideTabWell(false)
            )
            ->Split(
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.65f)
                ->AddTab(CurveTabId, ETabState::OpenedTab)
                ->SetHideTabWell(false)
            )
        );

    constexpr bool bCreateDefaultStandaloneMenu = true;
    constexpr bool bCreateDefaultToolbar = true;
    InitAssetEditor(
        Mode,
        InitToolkitHost,
        TEXT("CurveScribeDataAssetEditorApp"),
        Layout,
        bCreateDefaultStandaloneMenu,
        bCreateDefaultToolbar,
        InAsset
    );
    
    if (CurveEditorPanel.IsValid())
    {
        AddToolbarExtender(CurveEditorPanel->GetToolbarExtender());
        RegenerateMenusAndToolbars();
    }
}

void FCurveScribeDataAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(
        LOCTEXT("WorkspaceMenu", "CurveScribe Data Asset Editor"));

    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(DetailsTabId,
        FOnSpawnTab::CreateSP(this, &FCurveScribeDataAssetEditorToolkit::SpawnTab_Details))
        .SetDisplayName(LOCTEXT("DetailsTab", "Details"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

    InTabManager->RegisterTabSpawner(CurveTabId,
        FOnSpawnTab::CreateSP(this, &FCurveScribeDataAssetEditorToolkit::SpawnTab_Curve))
        .SetDisplayName(LOCTEXT("CurveTab", "Curve Editor"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCurveEditor.TabIcon"));
}

void FCurveScribeDataAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
    InTabManager->UnregisterTabSpawner(CurveTabId);
}

FName FCurveScribeDataAssetEditorToolkit::GetToolkitFName() const
{
    return FName("CurveScribeDataAssetEditor");
}

FText FCurveScribeDataAssetEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("BaseToolkitName", "CurveScribe Data Asset Editor");
}

FString FCurveScribeDataAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
    return LOCTEXT("WorldCentricTabPrefix", "CurveScribe ").ToString();
}

FLinearColor FCurveScribeDataAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(0.3f, 0.6f, 0.9f, 0.5f);
}

TSharedRef<SDockTab> FCurveScribeDataAssetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bHideSelectionTip = true;

    DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DetailsView->SetObject(EditingAsset);

    return SNew(SDockTab)
        .Label(LOCTEXT("DetailsTab", "Details"))
        [
            DetailsView.ToSharedRef()
        ];
}

TSharedRef<SDockTab> FCurveScribeDataAssetEditorToolkit::SpawnTab_Curve(const FSpawnTabArgs& Args)
{
    CurveEditor = MakeShared<FCurveEditor>();

    FCurveEditorInitParams InitParams;
    CurveEditor->InitCurveEditor(InitParams);

    // X 轴标签：默认带 "s"（秒），这里改成纯数值
    CurveEditor->GridLineLabelFormatXAttribute = LOCTEXT("GridXLabelFormat", "{0}");
    
    CurveEditorPanel = SNew(SCurveEditorPanel, CurveEditor.ToSharedRef());

    if (EditingAsset)
    {
        TUniquePtr<FRichCurveEditorModelRaw> Model = MakeUnique<FRichCurveEditorModelRaw>(
            &EditingAsset->TubeScaleCurve, EditingAsset);
        Model->SetShortDisplayName(LOCTEXT("TubeScaleCurve", "TubeScale"));
        Model->SetColor(FLinearColor(0.3f, 0.8f, 0.3f));

        const FCurveModelID CurveID = CurveEditor->AddCurve(MoveTemp(Model));
        CurveEditor->PinCurve(CurveID);
    }

    return SNew(SDockTab)
        .Label(LOCTEXT("CurveTab", "Curve Editor"))
        [
            
            CurveEditorPanel.ToSharedRef()
            
        ];
}

#undef LOCTEXT_NAMESPACE

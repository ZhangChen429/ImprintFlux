#include "Editor/CurveScribeDataAssetEditorToolkit.h"
#include "CurveScribeDataAsset.h"
#include "CurveScribeActor.h"
#include "CurveScribeScene.h"
#include "Editor/SCurveScribeDataAssetViewport.h"
#include "Editor/FCurveScribeDataAssetViewportClient.h"
#include "AdvancedPreviewScene.h"
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
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "CurveScribeDataAssetEditor"

const FName FCurveScribeDataAssetEditorToolkit::DetailsTabId(TEXT("CurveScribeDataAssetEditor_Details"));
const FName FCurveScribeDataAssetEditorToolkit::CurveTabId(TEXT("CurveScribeTubeDataAssetEditor_Curve"));
const FName FCurveScribeDataAssetEditorToolkit::ViewportTabId(TEXT("CurveScribeDataAssetEditor_Viewport"));

void FCurveScribeDataAssetEditorToolkit::InitEditor(EToolkitMode::Type Mode,
                                                   const TSharedPtr<IToolkitHost>& InitToolkitHost,
                                                   UCurveScribeDataAsset* InAsset)
{
    EditingAsset = InAsset;

    // 创建预览场景与 Actor，供 3D 视口使用
    CreatePreviewSceneAndActor();

    // 绑定数据资产变更，刷新预览
    if (EditingAsset)
    {
        OnAssetChangedHandle = EditingAsset->OnDataChanged.AddRaw(this, &FCurveScribeDataAssetEditorToolkit::OnAssetDataChanged);
    }

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("CurveScribeDataAssetEditor_Layout_v3")
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split(
                FTabManager::NewStack()
                ->SetSizeCoefficient(0.25f)
                ->AddTab(DetailsTabId, ETabState::OpenedTab)
                ->SetHideTabWell(false)
            )
            ->Split(
                FTabManager::NewSplitter()
                ->SetOrientation(Orient_Vertical)
                ->SetSizeCoefficient(0.75f)
                ->Split(
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.65f)
                    ->AddTab(ViewportTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(false)
                )
                ->Split(
                    FTabManager::NewStack()
                    ->SetSizeCoefficient(0.35f)
                    ->AddTab(CurveTabId, ETabState::OpenedTab)
                    ->SetHideTabWell(false)
                )
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

    InTabManager->RegisterTabSpawner(ViewportTabId,
        FOnSpawnTab::CreateSP(this, &FCurveScribeDataAssetEditorToolkit::SpawnTab_Viewport))
        .SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));
}

void FCurveScribeDataAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
    InTabManager->UnregisterTabSpawner(CurveTabId);
    InTabManager->UnregisterTabSpawner(ViewportTabId);
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

FCurveScribeDataAssetEditorToolkit::~FCurveScribeDataAssetEditorToolkit()
{
    if (EditingAsset)
    {
        EditingAsset->OnDataChanged.Remove(OnAssetChangedHandle);
    }
    DestroyPreviewActor();
}

UCurveScribeScene* FCurveScribeDataAssetEditorToolkit::GetPreviewSceneComponent() const
{
    return PreviewActor ? PreviewActor->CurveTargetScene : nullptr;
}

void FCurveScribeDataAssetEditorToolkit::CreatePreviewSceneAndActor()
{
    if (!EditingAsset)
    {
        return;
    }

    PreviewScene = MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues()));

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    PreviewActor = PreviewScene->GetWorld()->SpawnActor<ACurveScribeActor>(SpawnParams);

    if (PreviewActor && PreviewActor->CurveTargetScene)
    {
        SyncAssetToPreviewActor();
    }
}

void FCurveScribeDataAssetEditorToolkit::DestroyPreviewActor()
{
    if (PreviewActor && PreviewScene.IsValid())
    {
        PreviewScene->GetWorld()->DestroyActor(PreviewActor);
        PreviewActor = nullptr;
    }
    PreviewScene.Reset();
}

void FCurveScribeDataAssetEditorToolkit::SyncAssetToPreviewActor()
{
    if (!EditingAsset || !PreviewActor || !PreviewActor->CurveTargetScene)
    {
        return;
    }

    UCurveScribeScene* Scene = PreviewActor->CurveTargetScene;
    Scene->ControlPoints = EditingAsset->ControlPoints;
    Scene->CurveResolution = EditingAsset->CurveResolution;
    Scene->SplinePointType = EditingAsset->SplinePointType;
    Scene->FillSegmentCount = EditingAsset->FillSegmentCount;
    Scene->MaxDeviationAngle = EditingAsset->MaxDeviationAngle;
    Scene->TargetStepDistance = EditingAsset->TargetStepDistance;
    Scene->CorridorRadius = EditingAsset->CorridorRadius;
    Scene->RandomOffsetMinRadius = EditingAsset->RandomOffsetMinRadius;
    Scene->CurveData = EditingAsset;

    Scene->RebuildCurve();
}

void FCurveScribeDataAssetEditorToolkit::OnAssetDataChanged()
{
    SyncAssetToPreviewActor();

    if (ViewportWidget.IsValid())
    {
        TSharedPtr<FCurveScribeDataAssetViewportClient> ViewportClient = ViewportWidget->GetViewportClient();
        if (ViewportClient.IsValid())
        {
            ViewportClient->Invalidate();
        }
    }
}

TSharedRef<SDockTab> FCurveScribeDataAssetEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
    if (!ViewportWidget.IsValid())
    {
        ViewportWidget = SNew(SCurveScribeDataAssetViewport, SharedThis(this), PreviewScene.ToSharedRef());
    }

    return SNew(SDockTab)
        .Label(LOCTEXT("ViewportTab", "Viewport"))
        [
            ViewportWidget.ToSharedRef()
        ];
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

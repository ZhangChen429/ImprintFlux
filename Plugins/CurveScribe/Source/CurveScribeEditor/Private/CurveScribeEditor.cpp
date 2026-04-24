
#include "CurveScribeEditor.h"
#include "Misc/CoreDelegates.h"
#include "Editor/BezierCurveCommand.h"
#include "Editor/BezierCurveVisualizer.h"
#include "Editor/CurveScribeSceneCustomization.h"
#include "Editor/CurveScribeDataAssetCustomization.h"
#include "Style/BlenderPluginStyle.h"
#include "PropertyEditorModule.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "CurveScribeActor.h"
#include "CurveScribeScene.h"
#include "CurveScribeDataAsset.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWidget.h"
#include "Engine/World.h"
#include "Engine/SplineMeshActor.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Actor.h"
#include "EditorViewportClient.h"


DEFINE_LOG_CATEGORY(LogCurveScribeTool);
#define LOCTEXT_NAMESPACE "FCurveScribeEditorModule"

const FName FCurveScribeEditorModule::BezierCurveTabName = FName(TEXT("BezierCurveWindow"));

// 贝塞尔曲线参数
namespace BezierCurveSettings
{
	static int32 ControlPointCount = 4;
	static int32 CurveResolution = 100;
	static float CurveThickness = 10.0f;
	static bool bCloseLoop = false;
	static TArray<FVector> ControlPoints;
}

void FCurveScribeEditorModule::StartupModule()
{

	// 注册贝塞尔曲线 DockTab
	RegisterBezierCurveTab();

	// 注册 UCurveScribeScene 的 Details Panel 自定义布局
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(
			UCurveScribeScene::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FCurveScribeSceneCustomization::MakeInstance)
		);
		//PropertyModule.RegisterCustomClassLayout(
		//	UCurveScribeDataAsset::StaticClass()->GetFName(),
		//	FOnGetDetailCustomizationInstance::CreateStatic(&FCurveScribeDataAssetCustomization::MakeInstance)
		//);
	}

	// 注册 Component Visualizer（立即尝试 + PostEngineInit 后备）
	auto RegisterVisualizer = [this]()
	{
		if (GUnrealEd && !BezierCurveVisualizer.IsValid())
		{
			BezierCurveVisualizer = MakeShared<FBezierCurveVisualizer>();
			GUnrealEd->RegisterComponentVisualizer(
				UCurveScribeScene::StaticClass()->GetFName(),
				BezierCurveVisualizer
			);
			UE_LOG(LogCurveScribeTool, Log, TEXT("BezierCurve Visualizer registered for UCurveScribeScene"));
		}
	};

	// 如果引擎已初始化，立即注册；否则等委托
	if (GEngine && GUnrealEd)
	{
		RegisterVisualizer();
	}
	else
	{
		FCoreDelegates::OnPostEngineInit.AddLambda(RegisterVisualizer);
	}
}

void FCurveScribeEditorModule::ShutdownModule()
{
	// 清理菜单
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);


	// 注销贝塞尔曲线 DockTab
	if (FGlobalTabmanager::Get()->HasTabSpawner(BezierCurveTabName))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BezierCurveTabName);
	}

	// 注销 UCurveScribeScene 的 Details Panel 自定义布局
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UCurveScribeScene::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(UCurveScribeDataAsset::StaticClass()->GetFName());
	}

	// 注销命令
	FBezierCurveCommand::Unregister();
	if (BezierCurveCommands.IsValid())
	{
		BezierCurveCommands.Reset();
	}

	// 清理样式
	FBlenderPluginStyle::Shutdown();

	// 注销贝塞尔曲线可视化工具
	if (GUnrealEd && BezierCurveVisualizer.IsValid())
	{
		GUnrealEd->UnregisterComponentVisualizer(UCurveScribeScene::StaticClass()->GetFName());
		BezierCurveVisualizer.Reset();
	}
}

void FCurveScribeEditorModule::RegisterBezierCurveTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		BezierCurveTabName,
		FOnSpawnTab::CreateRaw(this, &FCurveScribeEditorModule::OnSpawnBezierCurveTab))
		.SetDisplayName(LOCTEXT("BezierCurveTabTitle", "Bezier Curve"))
		.SetTooltipText(LOCTEXT("BezierCurveTabTooltip", "Open the Bezier Curve Generator"))
		.SetIcon(FSlateIcon(FBlenderPluginStyle::GetStyleSetName(), "BlenderPlugin.OpenBezierCurveWindow"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FCurveScribeEditorModule::OnBezierCurveButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(BezierCurveTabName);
}

// 计算贝塞尔曲线上的点（通用实现，支持任意控制点数量）
static FVector CalculateBezierPoint(const TArray<FVector>& ControlPoints, float T)
{
	if (ControlPoints.Num() < 2)
	{
		return ControlPoints.Num() == 1 ? ControlPoints[0] : FVector::ZeroVector;
	}

	// 使用德卡斯特里奥算法计算贝塞尔曲线点
	TArray<FVector> Points = ControlPoints;
	int32 N = Points.Num() - 1;

	for (int32 k = 1; k <= N; ++k)
	{
		for (int32 i = 0; i <= N - k; ++i)
		{
			Points[i] = FMath::Lerp(Points[i], Points[i + 1], T);
		}
	}

	return Points[0];
}

// 生成贝塞尔曲线Actor
static void GenerateBezierCurve(const TArray<FVector>& ControlPoints, int32 Resolution, float Thickness, bool bLoop)
{
	if (ControlPoints.Num() < 2)
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("BezierCurve", "NotEnoughPoints", "至少需要2个控制点"));
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return;
	}

	// 计算曲线上的点
	TArray<FVector> CurvePoints;
	if (bLoop)
	{
		// 闭合循环：额外采样最后一个段来连接首尾
		for (int32 i = 0; i <= Resolution; ++i)
		{
			float T = static_cast<float>(i) / Resolution;
			CurvePoints.Add(CalculateBezierPoint(ControlPoints, T));
		}
	}
	else
	{
		for (int32 i = 0; i <= Resolution; ++i)
		{
			float T = static_cast<float>(i) / Resolution;
			CurvePoints.Add(CalculateBezierPoint(ControlPoints, T));
		}
	}

	// 创建Spline Mesh Actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASplineMeshActor* SplineActor = World->SpawnActor<ASplineMeshActor>(ASplineMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!SplineActor)
	{
		return;
	}

	SplineActor->SetActorLabel(TEXT("BezierCurve_" + FDateTime::Now().ToString(TEXT("%H%M%S"))));

	// 设置材质（如果可能）
	UMaterial* CurveMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));

	// 为每段曲线创建 Spline Mesh
	for (int32 i = 0; i < CurvePoints.Num() - 1; ++i)
	{
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(SplineActor);
		if (SplineMesh)
		{
			SplineMesh->RegisterComponent();
			SplineActor->AddInstanceComponent(SplineMesh);

			// 设置起始和结束位置
			FVector StartPos = CurvePoints[i];
			FVector EndPos = CurvePoints[i + 1];

			// 计算切线方向
			FVector StartTangent = (i == 0) ? (CurvePoints[i + 1] - CurvePoints[i]) : (CurvePoints[i + 1] - CurvePoints[i - 1]) * 0.5f;
			FVector EndTangent = (i == CurvePoints.Num() - 2) ? (CurvePoints[i + 1] - CurvePoints[i]) : (CurvePoints[i + 2] - CurvePoints[i]) * 0.5f;

			SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

			// 设置圆柱体作为默认网格
			UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder"));
			if (CylinderMesh)
			{
				SplineMesh->SetStaticMesh(CylinderMesh);
			}

			// 设置材质
			if (CurveMaterial)
			{
				SplineMesh->SetMaterial(0, CurveMaterial);
			}

			// 设置缩放来模拟线粗细
			SplineMesh->SetStartScale(FVector2D(Thickness / 50.0f));
			SplineMesh->SetEndScale(FVector2D(Thickness / 50.0f));
		}
	}

	// 显示成功通知
	FNotificationInfo Info(FText::Format(
		NSLOCTEXT("BezierCurve", "Generated", "已生成贝塞尔曲线，控制点: {0}, 曲线段: {1}"),
		FText::AsNumber(ControlPoints.Num()),
		FText::AsNumber(CurvePoints.Num() - 1)
	));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// 选中生成的Actor
	GEditor->SelectNone(false, true);
	GEditor->SelectActor(SplineActor, true, true);
}

TSharedRef<SDockTab> FCurveScribeEditorModule::OnSpawnBezierCurveTab(const FSpawnTabArgs& Args)
{
	// 默认控制点位置（在当前视口中心附近）
	FVector DefaultCenter = FVector::ZeroVector;
	if (GEditor && GEditor->GetActiveViewport())
	{
		FViewportClient* ViewportClient = GEditor->GetActiveViewport()->GetClient();
		FEditorViewportClient* Client = static_cast<FEditorViewportClient*>(ViewportClient);
		if (Client)
		{
			DefaultCenter = Client->GetViewLocation() + Client->GetViewRotation().Vector() * 500.0f;
		}
	}

	// 初始化默认控制点
	BezierCurveSettings::ControlPoints.SetNum(4);
	BezierCurveSettings::ControlPoints[0] = FVector(DefaultCenter.X, DefaultCenter.Y, DefaultCenter.Z);
	BezierCurveSettings::ControlPoints[1] = FVector(DefaultCenter.X + 100.0f, DefaultCenter.Y + 200.0f, DefaultCenter.Z);
	BezierCurveSettings::ControlPoints[2] = FVector(DefaultCenter.X + 200.0f, DefaultCenter.Y - 100.0f, DefaultCenter.Z);
	BezierCurveSettings::ControlPoints[3] = FVector(DefaultCenter.X + 300.0f, DefaultCenter.Y, DefaultCenter.Z);
	BezierCurveSettings::CurveResolution = 100;
	BezierCurveSettings::CurveThickness = 10.0f;

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBorder)
			.Padding(10)
			[
				SNew(SVerticalBox)

				// 标题
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 10)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BezierCurveTitle", "贝塞尔曲线生成器"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				]

				// 控制点设置
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 5)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ControlPointsLabel", "控制点位置:"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]

				// 控制点 0
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 3)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("P0", "P0:"))
						.MinDesiredWidth(30)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.X)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[0].X = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Y)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[0].Y = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Z)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[0].Z = Val; })
					]
				]

				// 控制点 1
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 3)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("P1", "P1:"))
						.MinDesiredWidth(30)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.X + 100.0f)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[1].X = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Y + 200.0f)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[1].Y = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Z)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[1].Z = Val; })
					]
				]

				// 控制点 2
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 3)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("P2", "P2:"))
						.MinDesiredWidth(30)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.X + 200.0f)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[2].X = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Y - 100.0f)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[2].Y = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Z)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[2].Z = Val; })
					]
				]

				// 控制点 3
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 3)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("P3", "P3:"))
						.MinDesiredWidth(30)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.X + 300.0f)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[3].X = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Y)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[3].Y = Val; })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2, 0)
					[
						SNew(SSpinBox<float>)
						.Value(DefaultCenter.Z)
						.MinValue(-10000.0f)
						.MaxValue(10000.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::ControlPoints[3].Z = Val; })
					]
				]

				// 分隔线
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 10)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
					[
						SNew(SBox)
						.HeightOverride(1)
					]
				]

				// 曲线分辨率设置
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 10, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ResolutionLabel", "曲线精度:"))
						.MinDesiredWidth(80)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SSpinBox<int32>)
						.Value(100)
						.MinValue(10)
						.MaxValue(1000)
						.OnValueChanged_Lambda([](int32 Val) { BezierCurveSettings::CurveResolution = Val; })
					]
				]

				// 线粗细设置
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 10, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ThicknessLabel", "曲线粗细:"))
						.MinDesiredWidth(80)
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SSpinBox<float>)
						.Value(10.0f)
						.MinValue(0.1f)
						.MaxValue(100.0f)
						.OnValueChanged_Lambda([](float Val) { BezierCurveSettings::CurveThickness = Val; })
					]
				]

				// 生成按钮
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 15)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("GenerateButton", "生成贝塞尔曲线"))
					.OnClicked_Lambda([]() -> FReply
					{
						GenerateBezierCurve(BezierCurveSettings::ControlPoints,
						                   BezierCurveSettings::CurveResolution,
						                   BezierCurveSettings::CurveThickness,
						                   false);
						return FReply::Handled();
					})
				]

				// 说明文字
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 10)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BezierInfo", "使用德卡斯特里奥算法生成标准贝塞尔曲线\n支持4个控制点的三次贝塞尔曲线"))
					.AutoWrapText(true)
					.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				]
			]
		];
}



#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCurveScribeEditorModule, CurveScribeEditor)
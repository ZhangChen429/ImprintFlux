#include "PythonHotEditor.h"


#include "Editor/PythonDebugCommand.h"
#include "Style/PythonHotEditorStyle.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#include "EditorUtilityBlueprint.h"
#include "EditorUtilityObject.h"

DEFINE_LOG_CATEGORY(LogPythonHotEditor);
#define LOCTEXT_NAMESPACE "FPythonHotEditorModule"

const FName FPythonHotEditorModule::PythonDebugTabName = FName(TEXT("PythonDebugWindow"));

void FPythonHotEditorModule::StartupModule()
{
	// 初始化样式（包含图标）
	FPythonHotEditorStyle::Initialize();

	// 初始化 PythonDebug 命令列表
	PythonDebugCommands = MakeShareable(new FUICommandList);

	// 注册 PythonDebug 命令
	FPythonDebugCommand::Register();

	// 绑定 PythonDebug 命令到操作
	PythonDebugCommands->MapAction(
		FPythonDebugCommand::Get().OpenPythonDebugWindow,
		FExecuteAction::CreateRaw(this, &FPythonHotEditorModule::OnPythonDebugButtonClicked),
		FCanExecuteAction()
	);

	// 注册 PythonDebug DockTab
	RegisterDockTab();

	// 注册菜单扩展
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPythonHotEditorModule::RegisterMenus)
	);
}

void FPythonHotEditorModule::ShutdownModule()
{
	// 清理菜单
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	// 注销 PythonDebug DockTab
	if (FGlobalTabmanager::Get()->HasTabSpawner(PythonDebugTabName))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PythonDebugTabName);
	}

	// 注销命令
	FPythonDebugCommand::Unregister();

	// 清理命令列表
	if (PythonDebugCommands.IsValid())
	{
		PythonDebugCommands.Reset();
	}

	// 清理样式
	FPythonHotEditorStyle::Shutdown();
}

void FPythonHotEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// 在工具栏中添加 PythonDebug 按钮
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	if (ToolbarMenu)
	{
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
		FToolMenuEntry& Entry = Section.AddEntry(
			FToolMenuEntry::InitToolBarButton(FPythonDebugCommand::Get().OpenPythonDebugWindow)
		);
		Entry.SetCommandList(PythonDebugCommands);
	}
}

void FPythonHotEditorModule::RegisterDockTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		PythonDebugTabName,
		FOnSpawnTab::CreateRaw(this, &FPythonHotEditorModule::OnSpawnPythonDebugTab))
		.SetDisplayName(LOCTEXT("PythonDebugTabTitle", "Python Debug"))
		.SetTooltipText(LOCTEXT("PythonDebugTabTooltip", "Open the Python Debug window"))
		.SetIcon(FSlateIcon(FPythonHotEditorStyle::GetStyleSetName(), "PythonHotEditor.OpenPythonDebugWindow"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FPythonHotEditorModule::OnPythonDebugButtonClicked()
{
	// 1. 正确的蓝图路径（去掉多余前缀）
	const FString BlueprintPath = TEXT("/PythonHot/PythonDebugObject.PythonDebugObject");

	// 2. 加载蓝图资产
	if (UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath))
	{
		// 3. 检查蓝图是否生成了有效类
		if (UClass* GeneratedClass = Blueprint->GeneratedClass)
		{
			// 4. 正确的类型判断：检查生成的类是否继承自 UEditorUtilityObject
			if (GeneratedClass->IsChildOf(UEditorUtilityObject::StaticClass()))
			{
				// 5. 获取默认对象，调用 Run()
				UEditorUtilityObject* EditorUtilityObj = Cast<UEditorUtilityObject>(GeneratedClass->GetDefaultObject());
				if (EditorUtilityObj)
				{
					EditorUtilityObj->Run();
					return;
				}
			}
			// 类型不匹配的错误
			UE_LOG(LogPythonHotEditor, Error, TEXT("Python Debug: BP %s No Cast Editor Utility Object "), *BlueprintPath);
		}
		else
		{
			UE_LOG(LogPythonHotEditor, Error, TEXT("Python Debug: BP %s invalid "), *BlueprintPath);
		}
	}
	else
	{
		UE_LOG(LogPythonHotEditor, Error, TEXT("Python Debug: No BP %s"), *BlueprintPath);
	}
}

TSharedRef<SDockTab> FPythonHotEditorModule::OnSpawnPythonDebugTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)

			// 标题
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PythonDebugTitle", "Python Debug Panel"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]

			// 说明文字
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 5)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PythonDebugInfo", "Python 热重载和调试功能"))
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
			]

			// 按钮示例（可扩展更多功能）
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 20)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("StartDebug", "开始调试"))
				.OnClicked_Lambda([]() -> FReply
				{
					UE_LOG(LogPythonHotEditor, Log, TEXT("Python Debug: Start Debug clicked"));
					return FReply::Handled();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 5)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("StopDebug", "停止调试"))
				.OnClicked_Lambda([]() -> FReply
				{
					UE_LOG(LogPythonHotEditor, Log, TEXT("Python Debug: Stop Debug clicked"));
					return FReply::Handled();
				})
			]

			// 状态显示
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 20)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DebugStatus", "状态: 未连接"))
				.ColorAndOpacity(FLinearColor(0.8f, 0.2f, 0.2f, 1.0f))
			]
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPythonHotEditorModule, PythonHotEditor)
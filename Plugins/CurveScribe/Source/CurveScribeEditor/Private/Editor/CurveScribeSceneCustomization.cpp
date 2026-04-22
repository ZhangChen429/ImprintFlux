#include "Editor/CurveScribeSceneCustomization.h"
#include "CurveScribeScene.h"
#include "CurveScribeDataAsset.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Editor.h"
#include "Components/SplineComponent.h"

#define LOCTEXT_NAMESPACE "CurveScribeSceneCustomization"

TSharedRef<IDetailCustomization> FCurveScribeSceneCustomization::MakeInstance()
{
    return MakeShared<FCurveScribeSceneCustomization>();
}

void FCurveScribeSceneCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);
    
    TSharedRef<IPropertyHandle> CurveDataProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, CurveData)
    );
    TSharedRef<IPropertyHandle> CircularTubeProperty = DetailBuilder.GetProperty(
           GET_MEMBER_NAME_CHECKED(UCurveScribeScene, CircularTubeData)
       );
    // 在 "Bezier Curve" 类别下添加自定义操作按钮
    IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
        TEXT("Bezier Curve"),
        LOCTEXT("BezierCurveCategory", "Bezier Curve"),
        ECategoryPriority::Important
    );
 
    Category.AddProperty(CurveDataProperty);
    Category.AddProperty(CircularTubeProperty);
    // 添加操作按钮行
    Category.AddCustomRow(LOCTEXT("ActionsRow", "Actions"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0, 0, 4, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("Rebuild", "重建曲线"))
                .ToolTipText(LOCTEXT("RebuildTooltip", "根据当前控制点重新生成样条线"))
                .OnClicked(this, &FCurveScribeSceneCustomization::OnRebuildCurve)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0, 0, 4, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("ClearCorridor", "保存控制点"))
                .ToolTipText(LOCTEXT("ClearCorridorTooltip", "保存目前的控制点"))
                .OnClicked(this, &FCurveScribeSceneCustomization::SaveCurveScribeData)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("LoadData", "加载资产"))
                .ToolTipText(LOCTEXT("LoadDataTooltip", "从绑定的数据资产加载配置"))
                .OnClicked(this, &FCurveScribeSceneCustomization::OnLoadFromDataAsset)
            ]
            
        ];
}

TWeakObjectPtr<UCurveScribeScene> FCurveScribeSceneCustomization::GetEditedScene() const
{
    for (const TWeakObjectPtr<UObject>& Obj : SelectedObjects)
    {
        if (UCurveScribeScene* Scene = Cast<UCurveScribeScene>(Obj.Get()))
        {
            return Scene;
        }
    }
    return nullptr;
}

FReply FCurveScribeSceneCustomization::OnRebuildCurve()
{
    if (UCurveScribeScene* Scene = GetEditedScene().Get())
    {   
        Scene->Modify();
        Scene->RandomOffsetControlPoints();
        Scene->NotifyControlPointsChanged();

        if (GEditor)
        {
            GEditor->RedrawLevelEditingViewports();
        }
    }
    return FReply::Handled();
}

FReply FCurveScribeSceneCustomization::SaveCurveScribeData()
{
    if (UCurveScribeScene* Scene = GetEditedScene().Get())
    {
        Scene->Modify();
        Scene->SaveToDataAsset();
        
        if (GEditor)
        {
            GEditor->RedrawLevelEditingViewports();
        }
    }
    return FReply::Handled();
}

FReply FCurveScribeSceneCustomization::OnLoadFromDataAsset()
{
    if (UCurveScribeScene* Scene = GetEditedScene().Get())
    {
        if (Scene->CurveData)
        {
            Scene->LoadFromDataAsset();

            if (GEditor)
            {
                GEditor->RedrawLevelEditingViewports();
            }
        }
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

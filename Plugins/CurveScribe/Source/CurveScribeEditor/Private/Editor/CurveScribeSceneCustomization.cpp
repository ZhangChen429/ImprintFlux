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
    TSharedRef<IPropertyHandle> FillSegmentCountProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, FillSegmentCount)
    );
    TSharedRef<IPropertyHandle> TargetStepDistanceProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, TargetStepDistance)
    );
    TSharedRef<IPropertyHandle> MaxDeviationAngleProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, MaxDeviationAngle)
    );
    TSharedRef<IPropertyHandle> OffsetMinProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, ControlPointRandomOffsetMin)
    );
    TSharedRef<IPropertyHandle> OffsetMaxProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, ControlPointRandomOffsetMax)
    );
    TSharedRef<IPropertyHandle> CorridorRadiusProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, CorridorRadius)
    );
    TSharedRef<IPropertyHandle> RandomOffsetMinRadiusProperty = DetailBuilder.GetProperty(
        GET_MEMBER_NAME_CHECKED(UCurveScribeScene, RandomOffsetMinRadius)
    );

    IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
        TEXT("Bezier Curve"),
        LOCTEXT("BezierCurveCategory", "Bezier Curve"),
        ECategoryPriority::Important
    );

    Category.AddProperty(CircularTubeProperty);
    Category.AddProperty(CurveDataProperty);
    Category.AddProperty(CorridorRadiusProperty);
    Category.AddProperty(RandomOffsetMinRadiusProperty);
    
    Category.AddCustomRow(LOCTEXT("ActionsRow", "Actions"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0, 0, 4, 0)
            [
                SNew(SButton)
                .Text(LOCTEXT("SaveControlPoints", "保存控制点"))
                .ToolTipText(LOCTEXT("SaveControlPointsTooltip", "保存目前的控制点"))
                .OnClicked(this, &FCurveScribeSceneCustomization::CurveScribeSaveCurveScribeData)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("LoadDataAsset", "加载资产"))
                .ToolTipText(LOCTEXT("LoadDataAssetTooltip", "从绑定的数据资产加载配置"))
                .OnClicked(this, &FCurveScribeSceneCustomization::CurveScribeOnLoadFromDataAsset)
            ]
        ];

    auto MakePropPair = [](TSharedRef<IPropertyHandle> Handle, bool bLastInRow) -> TSharedRef<SWidget>
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 0, 4, 0)
            [
                Handle->CreatePropertyNameWidget()
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .Padding(0, 0, bLastInRow ? 0 : 8, 0)
            [
                Handle->CreatePropertyValueWidget()
            ];
    };

    // 行1：沿线均匀填充 + FillSegmentCount
    Category.AddCustomRow(LOCTEXT("FillToTargetRow", "FillToTarget"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 2, 8, 2)
            [
                SNew(SButton)
                .Text(LOCTEXT("FillToTarget", "沿线填充控制点到Target"))
                .ToolTipText(LOCTEXT("FillToTargetTooltip", "均匀填充"))
                .OnClicked(this, &FCurveScribeSceneCustomization::CurveScribeFillPointsToTarget)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                MakePropPair(FillSegmentCountProperty, true)
            ]
        ];

    // 行2：随机步进填充 + TargetStepDistance + MaxDeviationAngle
    Category.AddCustomRow(LOCTEXT("FillRandomRow", "FillRandom"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 2, 8, 2)
            [
                SNew(SButton)
                .Text(LOCTEXT("FillRandomStep", "随机步进填充到目标点"))
                .ToolTipText(LOCTEXT("FillRandomStepTooltip", "根据步进距离填充"))
                .OnClicked(this, &FCurveScribeSceneCustomization::CurveScribeFillPointsRandomToTarget)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                MakePropPair(TargetStepDistanceProperty, false)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                MakePropPair(MaxDeviationAngleProperty, true)
            ]
        ];

    // 行3：随机偏转控制点 + OffsetMin + OffsetMax
    Category.AddCustomRow(LOCTEXT("RandomOffsetRow", "RandomOffset"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0, 2, 8, 2)
            [
                SNew(SButton)
                .Text(LOCTEXT("RandomOffsetCP", "随机偏转控制点"))
                .ToolTipText(LOCTEXT("RandomOffsetCPTooltip", "根据控制点偏转半径随机填充"))
                .OnClicked(this, &FCurveScribeSceneCustomization::CurveScribeRandomOffsetControlPoints)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                MakePropPair(OffsetMinProperty, false)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                MakePropPair(OffsetMaxProperty, true)
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


FReply FCurveScribeSceneCustomization::CurveScribeSaveCurveScribeData()
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

FReply FCurveScribeSceneCustomization::CurveScribeOnLoadFromDataAsset()
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

FReply FCurveScribeSceneCustomization::CurveScribeFillPointsRandomToTarget()
{
    if (UCurveScribeScene* Scene = GetEditedScene().Get())
    {
        if (Scene->CurveData)
        {
            Scene->FillPointsRandomToTarget();

            if (GEditor)
            {
                GEditor->RedrawLevelEditingViewports();
            }
        }
    }
    return FReply::Handled();
}


FReply FCurveScribeSceneCustomization::CurveScribeRandomOffsetControlPoints()
{
    if (UCurveScribeScene* Scene = GetEditedScene().Get())
    {
        if (Scene->CurveData)
        {
            Scene->RandomOffsetControlPoints();

            if (GEditor)
            {
                GEditor->RedrawLevelEditingViewports();
            }
        }
    }
    return FReply::Handled();
}


FReply FCurveScribeSceneCustomization::CurveScribeFillPointsToTarget()
{
    if (UCurveScribeScene* Scene = GetEditedScene().Get())
    {
        if (Scene->CurveData)
        {
            Scene->FillPointsToTarget();

            if (GEditor)
            {
                GEditor->RedrawLevelEditingViewports();
            }
        }
    }
    return FReply::Handled();
}
#undef LOCTEXT_NAMESPACE

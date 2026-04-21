#include "Editor/CurveScribeDataAssetCustomization.h"
#include "CurveScribeDataAsset.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "CurveScribeDataAssetCustomization"

TSharedRef<IDetailCustomization> FCurveScribeDataAssetCustomization::MakeInstance()
{
    return MakeShared<FCurveScribeDataAssetCustomization>();
}

void FCurveScribeDataAssetCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // 获取选中的对象
    DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

    // 隐藏默认的 ControlPoints 属性（我们将用自定义 UI 替代）
    DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, ControlPoints));

    // 构建控制点自定义区域
    BuildControlPointsSection(DetailBuilder);
}

TWeakObjectPtr<UCurveScribeDataAsset> FCurveScribeDataAssetCustomization::GetEditedAsset() const
{
    for (const TWeakObjectPtr<UObject>& Obj : SelectedObjects)
    {
        if (UCurveScribeDataAsset* Asset = Cast<UCurveScribeDataAsset>(Obj.Get()))
        {
            return Asset;
        }
    }
    return nullptr;
}

void FCurveScribeDataAssetCustomization::BuildControlPointsSection(IDetailLayoutBuilder& DetailBuilder)
{
    IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
        TEXT("Bezier Control Points"),
        LOCTEXT("ControlPointsCategory", "控制点编辑"),
        ECategoryPriority::Important
    );

    // 为每个控制点添加一行
    TWeakObjectPtr<UCurveScribeDataAsset> Asset = GetEditedAsset();
    if (!Asset.IsValid())
    {
        return;
    }

    const int32 Count = Asset->ControlPoints.Num();
    for (int32 i = 0; i < Count; ++i)
    {
        Category.AddCustomRow(FText::Format(
            LOCTEXT("ControlPointRowFormat", "Control Point {0}"),
            FText::AsNumber(i)
        ))
        .NameContent()
        [
            SNew(STextBlock)
            .Text(FText::Format(LOCTEXT("PointLabel", "P{0}"), FText::AsNumber(i)))
        ]
        .ValueContent()
        .MinDesiredWidth(300)
        [
            BuildControlPointRow(i)
        ];
    }

    // 添加按钮行
    Category.AddCustomRow(LOCTEXT("AddButton", "Add Point"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("AddPoint", "+ 添加控制点"))
                .OnClicked(this, &FCurveScribeDataAssetCustomization::OnAddControlPoint)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(8, 0, 0, 0)
            [
                SNew(STextBlock)
                .Text(FText::Format(
                    LOCTEXT("PointCount", "共 {0} 个控制点"),
                    FText::AsNumber(Count)
                ))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
        ];
}

TSharedRef<SWidget> FCurveScribeDataAssetCustomization::BuildControlPointRow(int32 Index)
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(2, 0)
        [
            SNew(SNumericEntryBox<float>)
            .AllowSpin(false)
            .MinValue(-100000.0f)
            .MaxValue(100000.0f)
            .Value(this, &FCurveScribeDataAssetCustomization::GetControlPointValue, Index, EAxis::Type::X)
            .OnValueCommitted(this, &FCurveScribeDataAssetCustomization::OnControlPointChanged, Index, EAxis::Type::X)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(2, 0)
        [
            SNew(SNumericEntryBox<float>)
            .AllowSpin(false)
            .MinValue(-100000.0f)
            .MaxValue(100000.0f)
            .Value(this, &FCurveScribeDataAssetCustomization::GetControlPointValue, Index, EAxis::Type::Y)
            .OnValueCommitted(this, &FCurveScribeDataAssetCustomization::OnControlPointChanged, Index, EAxis::Type::Y)
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(2, 0)
        [
            SNew(SNumericEntryBox<float>)
            .AllowSpin(false)
            .MinValue(-100000.0f)
            .MaxValue(100000.0f)
            .Value(this, &FCurveScribeDataAssetCustomization::GetControlPointValue, Index, EAxis::Type::Z)
            .OnValueCommitted(this, &FCurveScribeDataAssetCustomization::OnControlPointChanged, Index, EAxis::Type::Z)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4, 0, 0, 0)
        [
            SNew(SButton)
            .Text(LOCTEXT("Remove", "-"))
            .ToolTipText(LOCTEXT("RemoveTooltip", "删除此控制点"))
            .OnClicked(this, &FCurveScribeDataAssetCustomization::OnRemoveControlPoint, Index)
        ];
}

FReply FCurveScribeDataAssetCustomization::OnAddControlPoint()
{
    if (UCurveScribeDataAsset* Asset = GetEditedAsset().Get())
    {
        Asset->Modify();
        FVector NewPoint = FVector::ZeroVector;
        if (Asset->ControlPoints.Num() > 0)
        {
            NewPoint = Asset->ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f);
        }
        Asset->ControlPoints.Add(NewPoint);
        Asset->OnDataChanged.Broadcast();

        // 刷新 Details Panel
        if (GEditor)
        {
            GEditor->RedrawLevelEditingViewports();
        }
    }
    return FReply::Handled();
}

FReply FCurveScribeDataAssetCustomization::OnRemoveControlPoint(int32 Index)
{
    if (UCurveScribeDataAsset* Asset = GetEditedAsset().Get())
    {
        if (Asset->ControlPoints.IsValidIndex(Index) && Asset->ControlPoints.Num() > 2)
        {
            Asset->Modify();
            Asset->ControlPoints.RemoveAt(Index);
            Asset->OnDataChanged.Broadcast();

            if (GEditor)
            {
                GEditor->RedrawLevelEditingViewports();
            }
        }
    }
    return FReply::Handled();
}

void FCurveScribeDataAssetCustomization::OnControlPointChanged(float NewValue, ETextCommit::Type CommitType, int32 Index, EAxis::Type Axis)
{
    if (UCurveScribeDataAsset* Asset = GetEditedAsset().Get())
    {
        if (Asset->ControlPoints.IsValidIndex(Index))
        {
            Asset->Modify();
            FVector& Point = Asset->ControlPoints[Index];
            switch (Axis)
            {
            case EAxis::Type::X: Point.X = NewValue; break;
            case EAxis::Type::Y: Point.Y = NewValue; break;
            case EAxis::Type::Z: Point.Z = NewValue; break;
            default: break;
            }
            Asset->OnDataChanged.Broadcast();
        }
    }
}


TOptional<float> FCurveScribeDataAssetCustomization::GetControlPointValue(int32 Index, EAxis::Type Axis) const
{
    if (UCurveScribeDataAsset* Asset = GetEditedAsset().Get())
    {
        if (Asset->ControlPoints.IsValidIndex(Index))
        {
            const FVector& Point = Asset->ControlPoints[Index];
            switch (Axis)
            {
                case EAxis::Type::X: return Point.X;
                case EAxis::Type::Y: return Point.Y;
                case EAxis::Type::Z: return Point.Z;
                default: break;
            }
        }
    }
    return 0.0f;
}

#undef LOCTEXT_NAMESPACE

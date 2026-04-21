#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UCurveScribeDataAsset;

/**
 * UCurveScribeDataAsset 的 Details Panel 自定义布局
 * 为控制点数组提供更友好的编辑界面
 */
class FCurveScribeDataAssetCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    // 获取当前编辑的 DataAsset（支持多选时取第一个）
    TWeakObjectPtr<UCurveScribeDataAsset> GetEditedAsset() const;

    // 构建控制点列表区域
    void BuildControlPointsSection(IDetailLayoutBuilder& DetailBuilder);

    // 构建单个控制点的行
    TSharedRef<SWidget> BuildControlPointRow(int32 Index);

    // 添加新控制点
    FReply OnAddControlPoint();

    // 删除指定控制点
    FReply OnRemoveControlPoint(int32 Index);

    // 控制点 XYZ 变更回调
    void OnControlPointChanged(float NewValue, ETextCommit::Type CommitType, int32 Index, EAxis::Type Axis);
    TOptional<float> GetControlPointValue(int32 Index, EAxis::Type Axis) const;

    // 选中的对象列表
    TArray<TWeakObjectPtr<UObject>> SelectedObjects;
};

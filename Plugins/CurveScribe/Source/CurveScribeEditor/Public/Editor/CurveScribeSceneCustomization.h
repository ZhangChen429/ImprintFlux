#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UCurveScribeScene;

/**
 * UCurveScribeScene 的 Details Panel 自定义布局
 * 在 CurveTargetScene 属性区域添加实用操作按钮
 */
class FCurveScribeSceneCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    // 获取当前编辑的 Scene（支持多选时取第一个）
    TWeakObjectPtr<UCurveScribeScene> GetEditedScene() const;
    
    // 清除走廊按钮回调
    FReply CurveScribeSaveCurveScribeData();

    // 从数据资产加载按钮回调
    FReply CurveScribeOnLoadFromDataAsset();
    FReply CurveScribeFillPointsRandomToTarget();
    FReply CurveScribeRandomOffsetControlPoints();

    //均匀填充
    FReply CurveScribeFillPointsToTarget();

    // 选中的对象列表
    TArray<TWeakObjectPtr<UObject>> SelectedObjects;
};

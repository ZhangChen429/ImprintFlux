#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "AssetDefinition_CurveScribeDataAsset.generated.h"

/**
 * UCurveScribeDataAsset 的 UAssetDefinition（UE 5.1+ 新 API，5.3 推荐）。
 * 拦截双击，改为打开自定义 FCurveScribeDataAssetEditorToolkit 窗口。
 */
UCLASS()
class UAssetDefinition_CurveScribeDataAsset : public UAssetDefinitionDefault
{
    GENERATED_BODY()

public:
    virtual FText GetAssetDisplayName() const override;
    virtual FLinearColor GetAssetColor() const override;
    virtual TSoftClassPtr<UObject> GetAssetClass() const override;
    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
    virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};

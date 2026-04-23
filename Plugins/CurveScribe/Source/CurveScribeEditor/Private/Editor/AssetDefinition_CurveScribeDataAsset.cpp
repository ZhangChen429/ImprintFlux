#include "Editor/AssetDefinition_CurveScribeDataAsset.h"

#include "CurveScribeDataAsset.h"
#include "Editor/CurveScribeDataAssetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_CurveScribeDataAsset"

FText UAssetDefinition_CurveScribeDataAsset::GetAssetDisplayName() const
{
    return LOCTEXT("DisplayName", "CurveScribe Data Asset");
}

FLinearColor UAssetDefinition_CurveScribeDataAsset::GetAssetColor() const
{
    return FLinearColor(0.3f, 0.6f, 0.9f);
}

TSoftClassPtr<UObject> UAssetDefinition_CurveScribeDataAsset::GetAssetClass() const
{
    return UCurveScribeDataAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_CurveScribeDataAsset::GetAssetCategories() const
{
    static const FAssetCategoryPath Categories[] = {
        FAssetCategoryPath(LOCTEXT("CurveScribeCategory", "CurveScribe"))
    };
    return Categories;
}

EAssetCommandResult UAssetDefinition_CurveScribeDataAsset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UCurveScribeDataAsset* Asset : OpenArgs.LoadObjects<UCurveScribeDataAsset>())
    {
        if (Asset)
        {
            TSharedRef<FCurveScribeDataAssetEditorToolkit> Toolkit = MakeShared<FCurveScribeDataAssetEditorToolkit>();
            Toolkit->InitEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Asset);
        }
    }
    return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE

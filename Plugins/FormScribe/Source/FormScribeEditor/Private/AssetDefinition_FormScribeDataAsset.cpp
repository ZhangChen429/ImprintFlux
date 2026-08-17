#include "AssetDefinition_FormScribeDataAsset.h"

#include "FormScribeDataAsset.h"
#include "FormScribeDataAssetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_FormScribeDataAsset"

FText UAssetDefinition_FormScribeDataAsset::GetAssetDisplayName() const
{
	return LOCTEXT("DisplayName", "FormScribe Data Asset");
}

FLinearColor UAssetDefinition_FormScribeDataAsset::GetAssetColor() const
{
	return FLinearColor(0.25f, 0.75f, 0.55f);
}

TSoftClassPtr<UObject> UAssetDefinition_FormScribeDataAsset::GetAssetClass() const
{
	return UFormScribeDataAsset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_FormScribeDataAsset::GetAssetCategories() const
{
	static const FAssetCategoryPath Categories[] = {
		FAssetCategoryPath(LOCTEXT("FormScribeCategory", "FormScribe"))
	};
	return Categories;
}

EAssetCommandResult UAssetDefinition_FormScribeDataAsset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UFormScribeDataAsset* Asset : OpenArgs.LoadObjects<UFormScribeDataAsset>())
	{
		if (Asset)
		{
			TSharedRef<FFormScribeDataAssetEditorToolkit> Toolkit =
				MakeShared<FFormScribeDataAssetEditorToolkit>();
			Toolkit->InitEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Asset);
		}
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE

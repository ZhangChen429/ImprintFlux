#pragma once

#include "AdvancedPreviewScene.h"
#include "CoreMinimal.h"
#include "SEditorViewport.h"

class FFormScribeViewportClient;
class UFormScribeDataAsset;
class UStaticMeshComponent;

/** FormScribe 组合模型预览视口。 */
class FORMSCRIBEEDITOR_API SFormScribeViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SFormScribeViewport) {}
		SLATE_ARGUMENT(UFormScribeDataAsset*, Asset)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SFormScribeViewport() override;

	void RebuildPreview();

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	void ClearPreview();
	void HandleAssetChanged();

	TWeakObjectPtr<UFormScribeDataAsset> Asset;
	TUniquePtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FFormScribeViewportClient> ViewportClient;
	TArray<TStrongObjectPtr<UStaticMeshComponent>> PreviewComponents;
};

#include "FormScribeViewport.h"

#include "AdvancedPreviewScene.h"
#include "Components/StaticMeshComponent.h"
#include "EditorViewportClient.h"
#include "FormScribeDataAsset.h"

class FFormScribeViewportClient final : public FEditorViewportClient
{
public:
	FFormScribeViewportClient(
		FAdvancedPreviewScene* InPreviewScene,
		const TSharedRef<SEditorViewport>& InViewport)
		: FEditorViewportClient(nullptr, InPreviewScene, InViewport)
		, PreviewScene(InPreviewScene)
	{
		SetViewMode(VMI_Lit);
		SetViewportType(LVT_Perspective);
		SetViewLocation(FVector(-400.0, 400.0, 300.0));
		SetViewRotation(FRotator(-20.0, -45.0, 0.0));
		SetRealtime(true);
		EngineShowFlags.SetSelectionOutline(true);
	}

	virtual void Tick(float DeltaSeconds) override
	{
		FEditorViewportClient::Tick(DeltaSeconds);
		if (PreviewScene && PreviewScene->GetWorld())
		{
			PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
		}
	}

private:
	FAdvancedPreviewScene* PreviewScene = nullptr;
};

void SFormScribeViewport::Construct(const FArguments& InArgs)
{
	Asset = InArgs._Asset;
	PreviewScene = MakeUnique<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
	PreviewScene->SetFloorVisibility(true);

	SEditorViewport::Construct(SEditorViewport::FArguments());

	if (Asset.IsValid())
	{
		Asset->OnDataChanged.AddRaw(this, &SFormScribeViewport::HandleAssetChanged);
	}

	RebuildPreview();
}

SFormScribeViewport::~SFormScribeViewport()
{
	if (Asset.IsValid())
	{
		Asset->OnDataChanged.RemoveAll(this);
	}
	ClearPreview();
}

TSharedRef<FEditorViewportClient> SFormScribeViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShared<FFormScribeViewportClient>(PreviewScene.Get(), SharedThis(this));
	return ViewportClient.ToSharedRef();
}

void SFormScribeViewport::HandleAssetChanged()
{
	RebuildPreview();
}

void SFormScribeViewport::RebuildPreview()
{
	ClearPreview();

	if (!Asset.IsValid() || !PreviewScene)
	{
		return;
	}

	FBox PreviewBounds(ForceInit);
	for (const FFormScribeStaticMeshPart& Part : Asset->StaticMeshParts)
	{
		if (!Part.StaticMesh)
		{
			continue;
		}

		TStrongObjectPtr<UStaticMeshComponent> MeshComponent(
			NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient));
		MeshComponent->SetStaticMesh(Part.StaticMesh);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewScene->AddComponent(MeshComponent.Get(), Part.RelativeTransform);

		PreviewBounds += MeshComponent->Bounds.GetBox();
		PreviewComponents.Add(MoveTemp(MeshComponent));
	}

	if (ViewportClient.IsValid() && PreviewBounds.IsValid)
	{
		ViewportClient->FocusViewportOnBox(PreviewBounds);
	}

	Invalidate();
}

void SFormScribeViewport::ClearPreview()
{
	if (PreviewScene)
	{
		for (const TStrongObjectPtr<UStaticMeshComponent>& MeshComponent : PreviewComponents)
		{
			if (MeshComponent.IsValid())
			{
				PreviewScene->RemoveComponent(MeshComponent.Get());
			}
		}
	}

	PreviewComponents.Reset();
}

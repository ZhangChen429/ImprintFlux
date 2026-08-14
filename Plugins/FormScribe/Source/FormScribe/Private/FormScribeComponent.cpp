#include "FormScribeComponent.h"

#include "Components/StaticMeshComponent.h"
#include "FormScribeDataAsset.h"
#include "GameFramework/Actor.h"

UFormScribeComponent::UFormScribeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFormScribeComponent::OnRegister()
{
	Super::OnRegister();

	BindToDataAsset();
	RebuildForm();
}

void UFormScribeComponent::OnUnregister()
{
	for (UStaticMeshComponent* MeshComponent : StaticMeshComponents)
	{
		if (MeshComponent && MeshComponent->IsRegistered())
		{
			MeshComponent->UnregisterComponent();
		}
	}

	Super::OnUnregister();
}

void UFormScribeComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	ClearGeneratedComponents();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UFormScribeComponent::PostLoad()
{
	Super::PostLoad();
	BindToDataAsset();
}

void UFormScribeComponent::SetFormData(UFormScribeDataAsset* InFormData)
{
	if (FormData == InFormData)
	{
		return;
	}

	Modify();
	FormData = InFormData;
	BindToDataAsset();
	RebuildForm();
}

void UFormScribeComponent::RebuildForm()
{
	ClearGeneratedComponents();

	AActor* Owner = GetOwner();
	if (!Owner || !FormData)
	{
		return;
	}

	StaticMeshComponents.Reserve(FormData->StaticMeshParts.Num());

	for (int32 PartIndex = 0; PartIndex < FormData->StaticMeshParts.Num(); ++PartIndex)
	{
		const FFormScribeStaticMeshPart& Part = FormData->StaticMeshParts[PartIndex];
		if (!Part.StaticMesh)
		{
			continue;
		}

		const FName ComponentBaseName(*FString::Printf(
			TEXT("FormMesh_%d_%s"), PartIndex, *Part.Name.ToString()));
		const FName ComponentName = MakeUniqueObjectName(
			Owner,
			UStaticMeshComponent::StaticClass(),
			ComponentBaseName);

		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(
			Owner,
			ComponentName,
			RF_Transient | RF_Transactional);

		Owner->AddInstanceComponent(MeshComponent);
		MeshComponent->SetupAttachment(this);
		MeshComponent->SetRelativeTransform(Part.RelativeTransform);
		MeshComponent->SetStaticMesh(Part.StaticMesh);
		MeshComponent->SetMobility(GetMobility());

		if (IsRegistered() && GetWorld())
		{
			MeshComponent->RegisterComponentWithWorld(GetWorld());
		}

		StaticMeshComponents.Add(MeshComponent);
	}
}

void UFormScribeComponent::ClearGeneratedComponents()
{
	for (UStaticMeshComponent* MeshComponent : StaticMeshComponents)
	{
		if (MeshComponent)
		{
			MeshComponent->DestroyComponent();
		}
	}

	StaticMeshComponents.Reset();
}

void UFormScribeComponent::BindToDataAsset()
{
#if WITH_EDITOR
	if (BoundDataAsset.IsValid())
	{
		BoundDataAsset->OnDataChanged.RemoveAll(this);
	}

	BoundDataAsset = FormData;
	if (FormData)
	{
		FormData->OnDataChanged.AddUObject(this, &UFormScribeComponent::HandleDataAssetChanged);
	}
#endif
}

void UFormScribeComponent::HandleDataAssetChanged()
{
	RebuildForm();
}

#if WITH_EDITOR
void UFormScribeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	BindToDataAsset();
	RebuildForm();
}

void UFormScribeComponent::PostEditUndo()
{
	Super::PostEditUndo();
	BindToDataAsset();
	RebuildForm();
}
#endif

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FormScribeComponent.generated.h"

class UFormScribeDataAsset;
class UStaticMeshComponent;

/**
 * FormScribe 的形态编排组件。
 * 组件本身不绑定具体表现类型；当前由一个静态模型子组件呈现 DataAsset 中的模型。
 */
UCLASS(ClassGroup = (FormScribe), meta = (BlueprintSpawnableComponent))
class FORMSCRIBE_API UFormScribeComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFormScribeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormScribe")
	TObjectPtr<UFormScribeDataAsset> FormData = nullptr;

	/** 根据 DataAsset 生成的静态模型部件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "FormScribe|Generated")
	TArray<TObjectPtr<UStaticMeshComponent>> StaticMeshComponents;

	/** 将当前数据资产应用到所有表现子组件。 */
	UFUNCTION(BlueprintCallable, Category = "FormScribe")
	void RebuildForm();

	/** 更换数据资产并立即重建形态。 */
	UFUNCTION(BlueprintCallable, Category = "FormScribe")
	void SetFormData(UFormScribeDataAsset* InFormData);

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
#endif

private:
	void BindToDataAsset();
	void HandleDataAssetChanged();
	void ClearGeneratedComponents();

#if WITH_EDITOR
	TWeakObjectPtr<UFormScribeDataAsset> BoundDataAsset;
#endif
};

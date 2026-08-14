#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FormScribeDataAsset.generated.h"

class UStaticMesh;

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE(FOnFormScribeDataChanged);
#endif

/** 组合形态中的一个静态模型部件。 */
USTRUCT(BlueprintType)
struct FORMSCRIBE_API FFormScribeStaticMeshPart
{
	GENERATED_BODY()

	/** 用于编辑器识别和后续查找部件，不要求唯一。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormScribe")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormScribe")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	/** 相对于 UFormScribeComponent 的局部变换。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormScribe")
	FTransform RelativeTransform = FTransform::Identity;
};

/**
 * 可复用的形态数据资产。
 * 当前由多个带相对变换的静态模型部件组成；后续可以继续扩展其他表现类型。
 */
UCLASS(BlueprintType)
class FORMSCRIBE_API UFormScribeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FormScribe|Form", meta = (TitleProperty = "Name"))
	TArray<FFormScribeStaticMeshPart> StaticMeshParts;

#if WITH_EDITOR
	FOnFormScribeDataChanged OnDataChanged;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

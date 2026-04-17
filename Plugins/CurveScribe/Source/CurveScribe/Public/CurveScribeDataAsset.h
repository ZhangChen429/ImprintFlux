// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CurveScribeDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class CURVESCRIBE_API UCurveScribeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (MakeEditWidget = true))
	TArray<FVector> ControlPoints;

#if WITH_EDITOR
	// 声明委托，当数据改变时通知外界（如 Actor）
	DECLARE_MULTICAST_DELEGATE(FOnBezierDataChanged);
	FOnBezierDataChanged OnDataChanged;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

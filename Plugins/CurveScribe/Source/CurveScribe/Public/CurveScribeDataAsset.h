#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/SplineComponent.h"
#include "CurveScribeDataAsset.generated.h"

/**
 * 贝塞尔曲线数据资产 —— 曲线的完整数据定义
 * 可在 Content Browser 中独立创建、共享、序列化
 */
UCLASS()
class CURVESCRIBE_API UCurveScribeDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// ── 控制点 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (MakeEditWidget = true,DisplayName = "控制点位置"))
	TArray<FVector> ControlPoints;

	// ── 样条线参数 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (ClampMin = "10", ClampMax = "500" , DisplayName = "样条线参数"))
	int32 CurveResolution = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", DisplayName = "样条线类型")
	TEnumAsByte<ESplinePointType::Type> SplinePointType = ESplinePointType::Curve;

	// ── 填充参数 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (ClampMin = "1", DisplayName = "填充参数"))
	int32 FillSegmentCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (ClampMin = "0", ClampMax = "90",DisplayName = "最大偏转角度"))
	float MaxDeviationAngle = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", DisplayName = "步进长度")
	float TargetStepDistance = 20.0f;

	// ── Debug ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (ClampMin = "0", DisplayName = "Debug 圆管半径"))
	float DebugCircleRadius = 20.0f;

#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE(FOnBezierDataChanged);
	FOnBezierDataChanged OnDataChanged;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

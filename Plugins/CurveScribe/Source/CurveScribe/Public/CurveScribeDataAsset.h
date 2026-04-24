#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataAsset.h"
#include "Components/SplineComponent.h"
#include "Curves/RichCurve.h"
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
	
	UCurveScribeDataAsset();
	
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

	// ── 安全走廊 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (ClampMin = "0", DisplayName = "走廊半径"))
	float CorridorRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (ClampMin = "0", DisplayName = "随机最小偏移半径"))
	float RandomOffsetMinRadius = 10.0f;

	// ── 随机 A/B 样条的偏移方式 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (DisplayName = "用 Noise 采样替代纯随机"))
	bool bUseNoiseOffset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (DisplayName = "Noise 频率（沿曲线 wiggle 次数）", ClampMin = "0.1"))
	float NoiseFrequency = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (DisplayName = "Noise 种子 A"))
	float NoiseSeedA = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier", meta = (DisplayName = "Noise 种子 B"))
	float NoiseSeedB = 100.f;

	// ── 沿弧长 T∈[0,1] 的管径缩放曲线（inline RichCurve，替代外部 UCurveFloat 依赖） ──
	UPROPERTY(EditAnywhere, Category = "Bezier", DisplayName = "管径缩放曲线")
	FRichCurve TubeScaleCurve;

#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE(FOnBezierDataChanged);
	FOnBezierDataChanged OnDataChanged;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

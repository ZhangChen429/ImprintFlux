#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "CurveTargetScene.generated.h"

class USplineComponent;
class UBillboardComponent;

/**
 * 曲线目标组件 —— 只负责根据传入的控制点生成样条线
 * 控制点数据由 ABezierCurveActor 管理
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, PrioritizeCategories = "BezierActions"))
class CURVESCRIBE_API UCurveTargetScene : public USceneComponent
{
    GENERATED_BODY()

public:
    UCurveTargetScene();

    // ── 样条线参数 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (ClampMin = "10", ClampMax = "500"))
    int32 CurveResolution = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions")
    TEnumAsByte<ESplinePointType::Type> SplinePointType = ESplinePointType::Curve;

    // ── 子组件 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<UBillboardComponent> BillboardComponentEnd;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponent;

    // ── 根据外部传入的控制点重建样条线 ──
    UFUNCTION(BlueprintCallable, Category = "BezierActions|BezierFill")
    void RebuildCurve(const TArray<FVector>& InControlPoints);

private:
    FVector CalculateBezierPoint(const TArray<FVector>& Points, float T) const;
};

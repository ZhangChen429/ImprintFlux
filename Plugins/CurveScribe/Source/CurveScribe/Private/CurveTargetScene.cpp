#include "CurveTargetScene.h"
#include "Components/SplineComponent.h"


UCurveTargetScene::UCurveTargetScene()
{
}

void UCurveTargetScene::RebuildCurve(const TArray<FVector>& InControlPoints)
{
    // 1. 基础检查
    if (InControlPoints.Num() < 2 || !SplineComponent) return;

    // 2. 计算贝塞尔路径点
    TArray<FVector> CurvePoints;
    for (int32 i = 0; i <= CurveResolution; ++i)
    {
        float T = static_cast<float>(i) / (float)CurveResolution;
        CurvePoints.Add(CalculateBezierPoint(InControlPoints, T));
    }

    // 3. 使用世界坐标写入样条线点
    const FVector WorldOrigin = GetOwner() ? GetOwner()->GetActorLocation() : GetComponentLocation();
    SplineComponent->ClearSplinePoints(false);

    for (int32 i = 0; i < CurvePoints.Num(); ++i)
    {
        SplineComponent->AddSplinePoint(WorldOrigin + CurvePoints[i], ESplineCoordinateSpace::World, false);
        SplineComponent->SetSplinePointType(i, SplinePointType, false);
    }

    SplineComponent->UpdateSpline();
}

FVector UCurveTargetScene::CalculateBezierPoint(const TArray<FVector>& Points, float T) const
{
    if (Points.Num() < 2)
    {
        return Points.Num() == 1 ? Points[0] : FVector::ZeroVector;
    }

    TArray<FVector> TempPoints = Points;
    int32 N = TempPoints.Num() - 1;

    for (int32 k = 1; k <= N; ++k)
    {
        for (int32 i = 0; i <= N - k; ++i)
        {
            TempPoints[i] = FMath::Lerp(TempPoints[i], TempPoints[i + 1], T);
        }
    }

    return TempPoints[0];
}

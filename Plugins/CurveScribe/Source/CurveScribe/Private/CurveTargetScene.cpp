#include "CurveTargetScene.h"
#include "BezierCurveActor.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

UCurveTargetScene::UCurveTargetScene()
{
}

void UCurveTargetScene::OnRegister()
{
    Super::OnRegister();
    BindToOwner();
}

void UCurveTargetScene::BindToOwner()
{
    // 避免重复绑定
    if (DelegateHandle.IsValid())
    {
        return;
    }

    ABezierCurveActor* OwnerActor = Cast<ABezierCurveActor>(GetOwner());
    if (OwnerActor)
    {
        DelegateHandle = OwnerActor->OnControlPointsChanged.AddUObject(this, &UCurveTargetScene::HandleControlPointsChanged);
    }
}

void UCurveTargetScene::HandleControlPointsChanged(const TArray<FVector>& InControlPoints)
{
    RebuildCurve(InControlPoints);
}

void UCurveTargetScene::RebuildCurve(const TArray<FVector>& InControlPoints)
{
    if (InControlPoints.Num() < 2 || !SplineComponent)
    {
        return;
    }

    TArray<FVector> CurvePoints;
    for (int32 i = 0; i <= CurveResolution; ++i)
    {
        float T = static_cast<float>(i) / static_cast<float>(CurveResolution);
        CurvePoints.Add(CalculateBezierPoint(InControlPoints, T));
    }

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

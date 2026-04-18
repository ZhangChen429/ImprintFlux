#include "CurveScribeScene.h"
#include "CurveScribeActor.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

UCurveScribeScene::UCurveScribeScene()
{
    // 关键：允许组件 Tick
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    // 如果你想在编辑器里没点运行时也能看到 Debug 线
#if WITH_EDITOR
    bTickInEditor = true; 
#endif
}

void UCurveScribeScene::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 1. 确保 Spline 存在
    if (!SplineComponent) return;

    // 2. 遍历所有样条线点
    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FQuat PointRotation = SplineComponent->GetQuaternionAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector RightAxis = PointRotation.GetAxisY();
        FVector UpAxis = PointRotation.GetAxisZ();
        DrawDebugCircle(
            GetWorld(),
            PointLocation,
            DebugCircleRadius,
            32,
            DebugColor,
            false,
            -1.f, // 生命周期为 -1 表示仅存在于当前帧（配合 Tick）
            0,
            1.5f,
            RightAxis,           // 轴1：决定圆环平面
            UpAxis,              // 轴2：决定圆环平面
            false             // 是否绘制坐标轴
        );
    }
}


void UCurveScribeScene::OnRegister()
{
    Super::OnRegister();
    BindToOwner();
}

void UCurveScribeScene::BindToOwner()
{
    // 避免重复绑定
    if (DelegateHandle.IsValid())
    {
        return;
    }

    ACurveScribeActor* OwnerActor = Cast<ACurveScribeActor>(GetOwner());
    if (OwnerActor)
    {
        DelegateHandle = OwnerActor->OnControlPointsChanged.AddUObject(this, &UCurveScribeScene::HandleControlPointsChanged);
    }
}

void UCurveScribeScene::HandleControlPointsChanged(const TArray<FVector>& InControlPoints)
{
    RebuildCurve(InControlPoints);
}

void UCurveScribeScene::RebuildCurve(const TArray<FVector>& InControlPoints)
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

FVector UCurveScribeScene::CalculateBezierPoint(const TArray<FVector>& Points, float T) const
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

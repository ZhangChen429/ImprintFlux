#include "CurveScribeScene.h"
#include "CurveScribeActor.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

UCurveScribeScene* UCurveScribeScene::CreateAndAttach(
    AActor* Owner,
    USceneComponent* AttachTo,
    const FCurveScribeSubcomponentNames& Names)
{
    if (!Owner)
    {
        return nullptr;
    }

    UCurveScribeScene* Scene = Owner->CreateDefaultSubobject<UCurveScribeScene>(Names.Scene);
    if (!Scene)
    {
        return nullptr;
    }
    if (AttachTo)
    {
        Scene->SetupAttachment(AttachTo);
    }

    // 注意：子组件的 Outer 必须是 Owner（Actor），而非 Scene；否则注册阶段不进入 Actor 组件表，transform 不传播
    auto MakeBillboard = [Owner, Scene](FName InName) -> UBillboardComponent*
    {
        UBillboardComponent* B = Owner->CreateDefaultSubobject<UBillboardComponent>(InName);
        if (B) { B->SetupAttachment(Scene); }
        return B;
    };

    auto MakeSpline = [Owner, Scene](FName InName, FLinearColor Color) -> USplineComponent*
    {
        USplineComponent* S = Owner->CreateDefaultSubobject<USplineComponent>(InName);
        if (S)
        {
            S->SetupAttachment(Scene);
            S->EditorUnselectedSplineSegmentColor = Color;
            S->EditorSelectedSplineSegmentColor   = Color;
        }
        return S;
    };

    const FLinearColor WallColor  = FLinearColor(1.0f, 0.85f, 0.0f); // 黄 - 走廊左右
    const FLinearColor RandAColor = FLinearColor(1.0f, 0.2f,  0.2f); // 红 - 随机 A
    const FLinearColor RandBColor = FLinearColor(0.2f, 0.5f,  1.0f); // 蓝 - 随机 B

    Scene->BillboardComponentBegin = MakeBillboard(Names.BillboardBegin);
    Scene->BillboardComponentEnd   = MakeBillboard(Names.BillboardEnd);

    Scene->SplineComponent = Owner->CreateDefaultSubobject<USplineComponent>(Names.Spline);
    if (Scene->SplineComponent)
    {
        Scene->SplineComponent->SetupAttachment(Scene);
    }

    Scene->SplineComponentLeft    = MakeSpline(Names.SplineLeft,    WallColor);
    Scene->SplineComponentRight   = MakeSpline(Names.SplineRight,   WallColor);
    Scene->SplineComponentRandomA = MakeSpline(Names.SplineRandomA, RandAColor);
    Scene->SplineComponentRandomB = MakeSpline(Names.SplineRandomB, RandBColor);

    return Scene;
}

UCurveScribeScene::UCurveScribeScene()
{
    // 关键：允许组件 Tick
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;


#if WITH_EDITOR
    bTickInEditor = true;
#endif
}

void UCurveScribeScene::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 1. 确保 Spline 存在且 Debug 开关打开
    if (!SplineComponent || !bShowDebugCircles) return;

    // 2. 遍历所有样条线点
    const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
    const float ClampedInnerR = FMath::Clamp(RandomOffsetMinRadius, 0.f, CorridorRadius);
    for (int32 i = 0; i < NumPoints; ++i)
    {
        FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
        FQuat PointRotation = SplineComponent->GetQuaternionAtSplinePoint(i, ESplineCoordinateSpace::World);
        FVector RightAxis = PointRotation.GetAxisY();
        FVector UpAxis = PointRotation.GetAxisZ();

        // 外圈：走廊半径
        DrawDebugCircle(
            GetWorld(),
            PointLocation,
            CorridorRadius,
            32,
            DebugColor,
            false,
            -1.f,
            0,
            1.5f,
            RightAxis,
            UpAxis,
            false
        );

        // 内圈：随机最小偏移半径
        if (ClampedInnerR > KINDA_SMALL_NUMBER)
        {
            DrawDebugCircle(
                GetWorld(),
                PointLocation,
                ClampedInnerR,
                32,
                DebugInnerColor,
                false,
                -1.f,
                0,
                1.5f,
                RightAxis,
                UpAxis,
                false
            );
        }
    }
}


void UCurveScribeScene::OnRegister()
{
    Super::OnRegister();
    BindToOwner();
}

void UCurveScribeScene::SetShowDebugCircles(bool bNewShow)
{
    if (bShowDebugCircles == bNewShow)
    {
        return;
    }
    bShowDebugCircles = bNewShow;
    OnShowDebugCirclesChanged.Broadcast(bShowDebugCircles);
}

#if WITH_EDITOR
void UCurveScribeScene::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCurveScribeScene, bShowDebugCircles))
    {
        OnShowDebugCirclesChanged.Broadcast(bShowDebugCircles);
    }
}
#endif

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

    RebuildCorridor(CurvePoints, WorldOrigin);
    RebuildRandomInCorridor(InControlPoints, WorldOrigin);
}

void UCurveScribeScene::RebuildCorridor(const TArray<FVector>& InCurvePoints, const FVector& WorldOrigin)
{
    if (InCurvePoints.Num() < 2 || !SplineComponentLeft || !SplineComponentRight || CorridorRadius <= KINDA_SMALL_NUMBER)
    {
        if (SplineComponentLeft)  { SplineComponentLeft->ClearSplinePoints(true); }
        if (SplineComponentRight) { SplineComponentRight->ClearSplinePoints(true); }
        return;
    }

    SplineComponentLeft->ClearSplinePoints(false);
    SplineComponentRight->ClearSplinePoints(false);

    const int32 Last = InCurvePoints.Num() - 1;
    for (int32 i = 0; i <= Last; ++i)
    {
        // 中心差分求切向，端点退化为前向/后向差分
        FVector Tangent;
        if (i == 0)           { Tangent = InCurvePoints[1] - InCurvePoints[0]; }
        else if (i == Last)   { Tangent = InCurvePoints[Last] - InCurvePoints[Last - 1]; }
        else                  { Tangent = InCurvePoints[i + 1] - InCurvePoints[i - 1]; }
        Tangent = Tangent.GetSafeNormal();

        // 以世界 Z 为参考 Up，Right = T × Z；若切向接近竖直则退化用世界 X
        FVector RightDir = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
        if (RightDir.IsNearlyZero())
        {
            RightDir = FVector::CrossProduct(Tangent, FVector::ForwardVector).GetSafeNormal();
            if (RightDir.IsNearlyZero()) { RightDir = FVector::RightVector; }
        }

        const FVector CenterWorld = WorldOrigin + InCurvePoints[i];
        const FVector LeftWorld   = CenterWorld - RightDir * CorridorRadius;
        const FVector RightWorld  = CenterWorld + RightDir * CorridorRadius;

        SplineComponentLeft->AddSplinePoint(LeftWorld,   ESplineCoordinateSpace::World, false);
        SplineComponentRight->AddSplinePoint(RightWorld, ESplineCoordinateSpace::World, false);
        SplineComponentLeft->SetSplinePointType(i,  SplinePointType, false);
        SplineComponentRight->SetSplinePointType(i, SplinePointType, false);
    }

    SplineComponentLeft->UpdateSpline();
    SplineComponentRight->UpdateSpline();
}

void UCurveScribeScene::RebuildRandomInCorridor(const TArray<FVector>& InControlPoints, const FVector& WorldOrigin)
{
    auto BuildOne = [this, &InControlPoints, &WorldOrigin](USplineComponent* Spline)
    {
        if (!Spline) { return; }
        Spline->ClearSplinePoints(false);

        if (InControlPoints.Num() < 2 || CorridorRadius <= KINDA_SMALL_NUMBER)
        {
            Spline->UpdateSpline();
            return;
        }

        // 对每个贝塞尔控制点做随机左右偏移（首末端保持不动，保证起止点一致）
        const int32 Last = InControlPoints.Num() - 1;
        TArray<FVector> OffsetCPs;
        OffsetCPs.Reserve(InControlPoints.Num());
        for (int32 i = 0; i <= Last; ++i)
        {
            if (i == 0 || i == Last)
            {
                OffsetCPs.Add(InControlPoints[i]);
                continue;
            }

            FVector Tangent;
            if (i == 0)         { Tangent = InControlPoints[1] - InControlPoints[0]; }
            else if (i == Last) { Tangent = InControlPoints[Last] - InControlPoints[Last - 1]; }
            else                { Tangent = InControlPoints[i + 1] - InControlPoints[i - 1]; }
            Tangent = Tangent.GetSafeNormal();

            FVector RightDir = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
            if (RightDir.IsNearlyZero())
            {
                RightDir = FVector::CrossProduct(Tangent, FVector::ForwardVector).GetSafeNormal();
                if (RightDir.IsNearlyZero()) { RightDir = FVector::RightVector; }
            }
            // 法平面的第二个基向量（与 Tangent、Right 正交）
            const FVector NormalUp = FVector::CrossProduct(RightDir, Tangent).GetSafeNormal();

            // 在法平面内随机取方向和幅值：角度 [0, 2π)、幅值 [MinR, CorridorRadius]
            const float MinR = FMath::Clamp(RandomOffsetMinRadius, 0.f, CorridorRadius);
            const float Angle = FMath::FRandRange(0.f, 2.f * PI);
            const float RandMag = FMath::FRandRange(MinR, CorridorRadius);
            const FVector OffsetDir = FMath::Cos(Angle) * RightDir + FMath::Sin(Angle) * NormalUp;
            OffsetCPs.Add(InControlPoints[i] + OffsetDir * RandMag);
        }

        // 用偏移后的控制点做贝塞尔采样
        for (int32 i = 0; i <= CurveResolution; ++i)
        {
            const float T = static_cast<float>(i) / static_cast<float>(CurveResolution);
            const FVector P = CalculateBezierPoint(OffsetCPs, T);
            Spline->AddSplinePoint(WorldOrigin + P, ESplineCoordinateSpace::World, false);
            Spline->SetSplinePointType(i, SplinePointType, false);
        }
        Spline->UpdateSpline();
    };

    BuildOne(SplineComponentRandomA);
    BuildOne(SplineComponentRandomB);
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

#include "CurveScribeScene.h"
#include "CurveScribeActor.h"
#include "CurveScribeDataAsset.h"
#include "Curves/CurveFloat.h"
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
    FillSegmentCount = 4;
    // 控制点初始数据
    ControlPoints.Add(FVector(0.0f, 0.0f, 0.0f));
    ControlPoints.Add(FVector(100.0f, 200.0f, 0.0f));
    ControlPoints.Add(FVector(200.0f, -100.0f, 0.0f));
    ControlPoints.Add(FVector(300.0f, 0.0f, 0.0f));
    if (BillboardComponentEnd)
    {
        BillboardComponentEnd->SetRelativeLocation(
        ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f));
    }
#if WITH_EDITOR
    bTickInEditor = true;
#endif
}

void UCurveScribeScene::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const FTransform SceneXform = GetComponentTransform();

    // ── 走廊 Debug 圆环（只画随机最小偏移半径） ──
    if (SplineComponent && bShowDebugCircles)
    {
        const int32 NumPoints = SplineComponent->GetNumberOfSplinePoints();
        const int32 LastIdx = NumPoints - 1;
        for (int32 i = 0; i < NumPoints; ++i)
        {
            FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
            FQuat PointRotation = SplineComponent->GetQuaternionAtSplinePoint(i, ESplineCoordinateSpace::World);
            FVector RightAxis = PointRotation.GetAxisY();
            FVector UpAxis = PointRotation.GetAxisZ();

            const float T = (LastIdx > 0) ? static_cast<float>(i) / static_cast<float>(LastIdx) : 0.f;
            const float MinR = GetMinOffsetRadiusAt(T);

            if (MinR > KINDA_SMALL_NUMBER)
            {
                DrawDebugCircle(
                    GetWorld(),
                    PointLocation,
                    MinR,
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

    // ── 控制点切向 Debug 圆环 ──
    if (bShowControlPointCircles && ControlPoints.Num() > 0 )
    {
        const int32 Last = ControlPoints.Num() - 1;
        for (int32 i = 0; i <= Last; ++i)
        {
            const FVector CenterW = SceneXform.TransformPosition(ControlPoints[i]);

            // 切向：朝向下一个控制点；末点沿前一段方向
            FVector TangentLocal;
            if (i < Last)
            {
                TangentLocal = ControlPoints[i + 1] - ControlPoints[i];
            }
            else if (Last > 0)
            {
                TangentLocal = ControlPoints[i] - ControlPoints[i - 1];
            }
            else
            {
                TangentLocal = FVector::ForwardVector;
            }

            FVector Tangent = SceneXform.TransformVector(TangentLocal).GetSafeNormal();
            if (Tangent.IsNearlyZero())
            {
                Tangent = FVector::ForwardVector;
            }

            // 圆环所在法平面的两个基向量（与 Tangent 正交）
            FVector YAxis = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
            if (YAxis.IsNearlyZero())
            {
                YAxis = FVector::CrossProduct(Tangent, FVector::ForwardVector).GetSafeNormal();
                if (YAxis.IsNearlyZero())
                {
                    YAxis = FVector::RightVector;
                }
            }
            const FVector ZAxis = FVector::CrossProduct(Tangent, YAxis).GetSafeNormal();

            DrawDebugCircle(
                GetWorld(),
                CenterW,
                ControlPointRandomOffsetMin,
                32,
                ControlPointCircleColor,
                false,
                -1.f,
                0,
                1.5f,
                YAxis,
                ZAxis,
                false);
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

void UCurveScribeScene::NotifyControlPointsChanged()
{
    OnControlPointsChanged.Broadcast(ControlPoints);
}

#if WITH_EDITOR
void UCurveScribeScene::PreEditChange(FProperty* PropertyAboutToChange)
{
    Super::PreEditChange(PropertyAboutToChange);

    if (PropertyAboutToChange &&
        PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UCurveScribeScene, RandomOffsetMinRadius))
    {
        PreEditRandomOffsetMinRadius = RandomOffsetMinRadius;
    }
}

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

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCurveScribeScene, ControlPoints))
    {
        if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
        {
            int32 NewIndex = ControlPoints.Num() - 1;
            if (NewIndex > 0)
            {
                ControlPoints[NewIndex] = ControlPoints[NewIndex - 1] + FVector(200.0f, 0.0f, 0.0f);
            }
            if (BillboardComponentEnd)
            {
                BillboardComponentEnd->SetRelativeLocation(
                    ControlPoints[NewIndex] + FVector(100.0f, 0.0f, 0.0f));
                BillboardComponentEnd->UpdateComponentToWorld();
            }
        }

        RebuildCurve();
        NotifyControlPointsChanged();
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCurveScribeScene, CircularTubeData))
    {
        RebuildCurve();
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCurveScribeScene, RandomOffsetMinRadius))
    {
        if (RandomOffsetMinRadius > CorridorRadius)
        {
            RandomOffsetMinRadius = PreEditRandomOffsetMinRadius;
        }
    }
}

void UCurveScribeScene::PostEditUndo()
{
    Super::PostEditUndo();
    RebuildCurve();
    NotifyControlPointsChanged();
}
#endif

void UCurveScribeScene::BindToOwner()
{
    // Scene 现在是数据权威源，无需再绑定到 Actor 的委托
    // 只做初始曲线重建
    RebuildCurve();
}

void UCurveScribeScene::RebuildCurve()
{
    if (ControlPoints.Num() < 2 || !SplineComponent)
    {
        return;
    }

    // 控制点视为 Scene 本地空间；所有 spline 点写 Local，整体跟随 Scene 的 ComponentTransform
    TArray<FVector> CurvePoints;
    CurvePoints.Reserve(CurveResolution + 1);
    for (int32 i = 0; i <= CurveResolution; ++i)
    {
        float T = static_cast<float>(i) / static_cast<float>(CurveResolution);
        CurvePoints.Add(CalculateBezierPoint(ControlPoints, T));
    }

    SplineComponent->ClearSplinePoints(false);
    for (int32 i = 0; i < CurvePoints.Num(); ++i)
    {
        SplineComponent->AddSplinePoint(CurvePoints[i], ESplineCoordinateSpace::Local, false);
        SplineComponent->SetSplinePointType(i, SplinePointType, false);
    }
    SplineComponent->UpdateSpline();

    RebuildCorridor(CurvePoints);
    RebuildRandomInCorridor(ControlPoints);
}

void UCurveScribeScene::RebuildCorridor(const TArray<FVector>& InCurvePoints)
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
        // 中心差分求切向，端点退化为前向/后向差分（Scene 本地空间）
        FVector Tangent;
        if (i == 0)           { Tangent = InCurvePoints[1] - InCurvePoints[0]; }
        else if (i == Last)   { Tangent = InCurvePoints[Last] - InCurvePoints[Last - 1]; }
        else                  { Tangent = InCurvePoints[i + 1] - InCurvePoints[i - 1]; }
        Tangent = Tangent.GetSafeNormal();

        // 以本地 Z 为参考 Up，Right = T × Z；若切向接近竖直则退化用本地 X
        FVector RightDir = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
        if (RightDir.IsNearlyZero())
        {
            RightDir = FVector::CrossProduct(Tangent, FVector::ForwardVector).GetSafeNormal();
            if (RightDir.IsNearlyZero()) { RightDir = FVector::RightVector; }
        }

        const float T = (Last > 0) ? static_cast<float>(i) / static_cast<float>(Last) : 0.f;
        const float RadiusAtT = GetCorridorRadiusAt(T);

        const FVector CenterLocal = InCurvePoints[i];
        const FVector LeftLocal   = CenterLocal - RightDir * RadiusAtT;
        const FVector RightLocal  = CenterLocal + RightDir * RadiusAtT;

        SplineComponentLeft->AddSplinePoint(LeftLocal,   ESplineCoordinateSpace::Local, false);
        SplineComponentRight->AddSplinePoint(RightLocal, ESplineCoordinateSpace::Local, false);
        SplineComponentLeft->SetSplinePointType(i,  SplinePointType, false);
        SplineComponentRight->SetSplinePointType(i, SplinePointType, false);
    }

    SplineComponentLeft->UpdateSpline();
    SplineComponentRight->UpdateSpline();
}

void UCurveScribeScene::RebuildRandomInCorridor(const TArray<FVector>& InControlPoints)
{
    auto BuildOne = [this, &InControlPoints](USplineComponent* Spline)
    {
        if (!Spline) { return; }
        Spline->ClearSplinePoints(false);

        if (InControlPoints.Num() < 2 || CorridorRadius <= KINDA_SMALL_NUMBER)
        {
            Spline->UpdateSpline();
            return;
        }

        // 对每个贝塞尔控制点做随机偏移（首末端保持不动，保证起止点一致）
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

            // 在法平面内随机取方向和幅值：角度 [0, 2π)、幅值 [MinR, RadiusAtT]
            const float T = (Last > 0) ? static_cast<float>(i) / static_cast<float>(Last) : 0.f;
            const float RadiusAtT = GetCorridorRadiusAt(T);
            const float MinR = FMath::Clamp(GetMinOffsetRadiusAt(T), 0.f, RadiusAtT);
            const float Angle = FMath::FRandRange(0.f, 2.f * PI);
            const float RandMag = FMath::FRandRange(MinR, RadiusAtT);
            const FVector OffsetDir = FMath::Cos(Angle) * RightDir + FMath::Sin(Angle) * NormalUp;
            OffsetCPs.Add(InControlPoints[i] + OffsetDir * RandMag);
        }

        // 用偏移后的控制点做贝塞尔采样，写入 Scene 本地空间
        for (int32 i = 0; i <= CurveResolution; ++i)
        {
            const float T = static_cast<float>(i) / static_cast<float>(CurveResolution);
            const FVector P = CalculateBezierPoint(OffsetCPs, T);
            Spline->AddSplinePoint(P, ESplineCoordinateSpace::Local, false);
            Spline->SetSplinePointType(i, SplinePointType, false);
        }
        Spline->UpdateSpline();
    };

    BuildOne(SplineComponentRandomA);
    BuildOne(SplineComponentRandomB);
}

float UCurveScribeScene::GetTubeScaleAt(float T) const
{
    if (!CircularTubeData)
    {
        return 1.f;
    }
    return CircularTubeData->GetFloatValue(FMath::Clamp(T, 0.f, 1.f));
}

float UCurveScribeScene::GetCorridorRadiusAt(float T) const
{
    return CorridorRadius * GetTubeScaleAt(T);
}

float UCurveScribeScene::GetMinOffsetRadiusAt(float T) const
{
    return RandomOffsetMinRadius * GetTubeScaleAt(T);
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

void UCurveScribeScene::FillPointsToTarget()
{
    if (ControlPoints.Num() == 0 || !BillboardComponentEnd || FillSegmentCount <= 0)
    {
        return;
    }

    const FVector LastPoint = ControlPoints.Last();
    const FVector EndLocation = BillboardComponentEnd->GetRelativeLocation();
    const FVector Direction = EndLocation - LastPoint;

    if (Direction.Size() < KINDA_SMALL_NUMBER)
    {
        return;
    }

    Modify();

    const FVector Step = Direction / (FillSegmentCount + 1);

    for (int32 i = 1; i <= FillSegmentCount; ++i)
    {
        ControlPoints.Add(LastPoint + Step * i);
    }

    BillboardComponentEnd->SetRelativeLocation(ControlPoints.Last() + Step);
    RebuildCurve();
    NotifyControlPointsChanged();
}

void UCurveScribeScene::FillPointsRandomToTarget()
{
    if (ControlPoints.Num() == 0 || !BillboardComponentEnd || TargetStepDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    Modify();

    const FVector EndPos = BillboardComponentEnd->GetRelativeLocation();

    while (FVector::Dist(ControlPoints.Last(), EndPos) > TargetStepDistance * 1.2f)
    {
        FVector CurrentPoint = ControlPoints.Last();
        FVector TargetDir = (EndPos - CurrentPoint).GetSafeNormal();

        FRotator BaseRot = TargetDir.Rotation();
        float RandomPitch = FMath::FRandRange(-MaxDeviationAngle, MaxDeviationAngle);
        float RandomYaw = FMath::FRandRange(-MaxDeviationAngle, MaxDeviationAngle);

        FVector NewDirection = (BaseRot + FRotator(RandomPitch, RandomYaw, 0.f)).Vector();
        FVector NextPoint = CurrentPoint + (NewDirection * TargetStepDistance);
        ControlPoints.Add(NextPoint);

        if (ControlPoints.Num() > 500) break;
    }

    ControlPoints.Add(EndPos);
    RebuildCurve();
    NotifyControlPointsChanged();
}

void UCurveScribeScene::RandomOffsetControlPoints()
{
    if (ControlPoints.Num() < 3 || ControlPointRandomOffsetMax <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    Modify();

    const int32 Last = ControlPoints.Num() - 1;
    const float MinR = FMath::Clamp(ControlPointRandomOffsetMin, 0.f, ControlPointRandomOffsetMax);

    // 首末端保持不动，保证起止位置不变
    for (int32 i = 1; i < Last; ++i)
    {
        // 切向：中心差分
        FVector Tangent = (ControlPoints[i + 1] - ControlPoints[i - 1]).GetSafeNormal();
        if (Tangent.IsNearlyZero())
        {
            Tangent = FVector::ForwardVector;
        }

        // 法平面基向量（与 Tangent 正交）
        FVector RightDir = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
        if (RightDir.IsNearlyZero())
        {
            RightDir = FVector::CrossProduct(Tangent, FVector::ForwardVector).GetSafeNormal();
            if (RightDir.IsNearlyZero())
            {
                RightDir = FVector::RightVector;
            }
        }
        const FVector NormalUp = FVector::CrossProduct(RightDir, Tangent).GetSafeNormal();

        // 法平面极坐标：角度 [0, 2π)、半径 [Min, Max]
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const float Mag = FMath::FRandRange(MinR, ControlPointRandomOffsetMax);
        const FVector OffsetDir = FMath::Cos(Angle) * RightDir + FMath::Sin(Angle) * NormalUp;

        ControlPoints[i] += OffsetDir * Mag;
    }

    RebuildCurve();
    NotifyControlPointsChanged();
}

void UCurveScribeScene::LoadFromDataAsset()
{
    if (!CurveData)
    {
        return;
    }

    Modify();

    ControlPoints = CurveData->ControlPoints;
    CurveResolution = CurveData->CurveResolution;
    SplinePointType = CurveData->SplinePointType;
    CorridorRadius = CurveData->CorridorRadius;
    RandomOffsetMinRadius = CurveData->RandomOffsetMinRadius;

    if (BillboardComponentEnd && ControlPoints.Num() > 0)
    {
        BillboardComponentEnd->SetRelativeLocation(
            ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f));
    }

    RebuildCurve();
    NotifyControlPointsChanged();
}

void UCurveScribeScene::SaveToDataAsset()
{
    if (!CurveData)
    {
        return;
    }

    CurveData->Modify();

    CurveData->ControlPoints = ControlPoints;
    CurveData->CurveResolution = CurveResolution;
    CurveData->SplinePointType = SplinePointType;
    CurveData->CorridorRadius = CorridorRadius;
    CurveData->RandomOffsetMinRadius = RandomOffsetMinRadius;

    CurveData->MarkPackageDirty();
}

void UCurveScribeScene::BakeTransformIntoControlPoints()
{
    const FTransform SceneRel = GetRelativeTransform();
    if (SceneRel.Equals(FTransform::Identity))
    {
        return;
    }

    Modify();

    // 控制点：把 Scene 的相对变换吸收进本地坐标
    for (FVector& P : ControlPoints)
    {
        P = SceneRel.TransformPosition(P);
    }

    // Billboard：保持其在父空间下的位置
    auto BakeBillboard = [&SceneRel](UBillboardComponent* B)
    {
        if (!B) { return; }
        B->Modify();
        const FVector NewLocal = SceneRel.TransformPosition(B->GetRelativeLocation());
        B->SetRelativeLocation(NewLocal);
    };
    BakeBillboard(BillboardComponentBegin);
    BakeBillboard(BillboardComponentEnd);

    // 重置 Scene，使其与父级对齐
    SetRelativeTransform(FTransform::Identity);

    // 触发 spline 重建
    RebuildCurve();
    NotifyControlPointsChanged();
}

#include "CurveScribeActor.h"
#include "CurveScribeScene.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"

ACurveScribeActor::ACurveScribeActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bRunConstructionScriptOnDrag = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    // 一行完成 Scene + 全部子组件的创建与装配
    CurveTargetScene = UCurveScribeScene::CreateAndAttach(this, RootComponent);

    // 控制点初始数据
    FillSegmentCount = 4;
    ControlPoints.Add(FVector(0.0f, 0.0f, 0.0f));
    ControlPoints.Add(FVector(100.0f, 200.0f, 0.0f));
    ControlPoints.Add(FVector(200.0f, -100.0f, 0.0f));
    ControlPoints.Add(FVector(300.0f, 0.0f, 0.0f));

    if (CurveTargetScene && CurveTargetScene->BillboardComponentEnd)
    {
        CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(
            ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f));
    }
}

void ACurveScribeActor::PostInitProperties()
{
    Super::PostInitProperties();
}

void ACurveScribeActor::NotifyControlPointsChanged()
{
    OnControlPointsChanged.Broadcast(ControlPoints);
}

void ACurveScribeActor::LoadFromDataAsset()
{
    if (!CurveData)
    {
        return;
    }

    Modify();

    ControlPoints = CurveData->ControlPoints;
    FillSegmentCount = CurveData->FillSegmentCount;
    MaxDeviationAngle = CurveData->MaxDeviationAngle;
    TargetStepDistance = CurveData->TargetStepDistance;

    if (CurveTargetScene)
    {
        CurveTargetScene->CurveResolution = CurveData->CurveResolution;
        CurveTargetScene->SplinePointType = CurveData->SplinePointType;
        CurveTargetScene->CorridorRadius = CurveData->CorridorRadius;
        CurveTargetScene->RandomOffsetMinRadius = CurveData->RandomOffsetMinRadius;

        if (CurveTargetScene->BillboardComponentEnd && ControlPoints.Num() > 0)
        {
            CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(
                ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f));
        }
    }

    NotifyControlPointsChanged();
}

void ACurveScribeActor::BakeSceneTransformIntoControlPoints()
{
    if (!CurveTargetScene)
    {
        return;
    }

    const FTransform SceneRel = CurveTargetScene->GetRelativeTransform();
    if (SceneRel.Equals(FTransform::Identity))
    {
        return;
    }

    Modify();
    CurveTargetScene->Modify();

    // 控制点：把 Scene 的相对变换吸收进 actor-local 坐标
    for (FVector& P : ControlPoints)
    {
        P = SceneRel.TransformPosition(P);
    }

    // Billboard：保持其在 actor 下的世界位置（Scene 重置为 identity 后世界变换会变）
    auto BakeBillboard = [&SceneRel](UBillboardComponent* B)
    {
        if (!B) { return; }
        B->Modify();
        const FVector NewLocal = SceneRel.TransformPosition(B->GetRelativeLocation());
        B->SetRelativeLocation(NewLocal);
    };
    BakeBillboard(CurveTargetScene->BillboardComponentBegin);
    BakeBillboard(CurveTargetScene->BillboardComponentEnd);

    // 重置 Scene，使其与 actor 对齐；MakeEditWidget 此时会与曲线重新对齐
    CurveTargetScene->SetRelativeTransform(FTransform::Identity);

    // 触发 spline 重建
    NotifyControlPointsChanged();
}

void ACurveScribeActor::SaveToDataAsset()
{
    if (!CurveData)
    {
        return;
    }

    CurveData->Modify();

    CurveData->ControlPoints = ControlPoints;
    CurveData->FillSegmentCount = FillSegmentCount;
    CurveData->MaxDeviationAngle = MaxDeviationAngle;
    CurveData->TargetStepDistance = TargetStepDistance;

    if (CurveTargetScene)
    {
        CurveData->CurveResolution = CurveTargetScene->CurveResolution;
        CurveData->SplinePointType = CurveTargetScene->SplinePointType;
        CurveData->CorridorRadius = CurveTargetScene->CorridorRadius;
        CurveData->RandomOffsetMinRadius = CurveTargetScene->RandomOffsetMinRadius;
    }

    CurveData->MarkPackageDirty();
}

#if WITH_EDITOR
void ACurveScribeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACurveScribeActor, ControlPoints))
    {
        if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
        {
            int32 NewIndex = ControlPoints.Num() - 1;
            if (NewIndex > 0)
            {
                ControlPoints[NewIndex] = ControlPoints[NewIndex - 1] + FVector(200.0f, 0.0f, 0.0f);
            }
            if (CurveTargetScene && CurveTargetScene->BillboardComponentEnd)
            {
                CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(
                    ControlPoints[NewIndex] + FVector(100.0f, 0.0f, 0.0f));
                CurveTargetScene->BillboardComponentEnd->UpdateComponentToWorld();
            }
        }

        NotifyControlPointsChanged();
    }
}

void ACurveScribeActor::PostEditUndo()
{
    Super::PostEditUndo();
    NotifyControlPointsChanged();
}
#endif

void ACurveScribeActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    NotifyControlPointsChanged();
}

void ACurveScribeActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bShowControlPointCircles
        || ControlPoints.Num() == 0
        || ControlPointCircleRadius <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FTransform ActorXform = GetActorTransform();
    const int32 Last = ControlPoints.Num() - 1;

    for (int32 i = 0; i <= Last; ++i)
    {
        const FVector CenterW = ActorXform.TransformPosition(ControlPoints[i]);

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

        FVector Tangent = ActorXform.TransformVector(TangentLocal).GetSafeNormal();
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
            ControlPointCircleRadius,
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

void ACurveScribeActor::FillPointsToTarget()
{
    if (ControlPoints.Num() == 0 || !CurveTargetScene || !CurveTargetScene->BillboardComponentEnd || FillSegmentCount <= 0)
    {
        return;
    }

    const FVector LastPoint = ControlPoints.Last();
    const FVector EndLocation = CurveTargetScene->BillboardComponentEnd->GetRelativeLocation();
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

    CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(ControlPoints.Last() + Step);
    NotifyControlPointsChanged();
}

void ACurveScribeActor::FillPointsRandomToTarget()
{
    if (ControlPoints.Num() == 0 || !CurveTargetScene || !CurveTargetScene->BillboardComponentEnd
        || TargetStepDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    Modify();

    const FVector EndPos = CurveTargetScene->BillboardComponentEnd->GetRelativeLocation();

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
    NotifyControlPointsChanged();
}

void ACurveScribeActor::RandomOffsetControlPoints()
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
        const float Angle  = FMath::FRandRange(0.f, 2.f * PI);
        const float Mag    = FMath::FRandRange(MinR, ControlPointRandomOffsetMax);
        const FVector OffsetDir = FMath::Cos(Angle) * RightDir + FMath::Sin(Angle) * NormalUp;

        ControlPoints[i] += OffsetDir * Mag;
    }

    NotifyControlPointsChanged();
}

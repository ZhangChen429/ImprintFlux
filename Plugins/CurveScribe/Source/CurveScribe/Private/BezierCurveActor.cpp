#include "BezierCurveActor.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

ABezierCurveActor::ABezierCurveActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bRunConstructionScriptOnDrag = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    BillboardComponentBegin = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardBeginComponent"));
    BillboardComponentBegin->SetupAttachment(RootComponent);

    // 曲线目标子树
    CurveTargetScene = CreateDefaultSubobject<UCurveTargetScene>(TEXT("CurveTargetScene"));
    CurveTargetScene->SetupAttachment(RootComponent);

    // 子组件在 Actor 中创建，挂到 CurveTargetScene 下
    CurveTargetScene->BillboardComponentEnd = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardEndComponent"));
    CurveTargetScene->BillboardComponentEnd->SetupAttachment(CurveTargetScene);

    CurveTargetScene->SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    CurveTargetScene->SplineComponent->SetupAttachment(CurveTargetScene);

    // 控制点初始数据
    FillSegmentCount = 4;
    ControlPoints.Add(FVector(0.0f, 0.0f, 0.0f));
    ControlPoints.Add(FVector(100.0f, 200.0f, 0.0f));
    ControlPoints.Add(FVector(200.0f, -100.0f, 0.0f));
    ControlPoints.Add(FVector(300.0f, 0.0f, 0.0f));

    CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(
        ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f));
}

void ABezierCurveActor::PostInitProperties()
{
    Super::PostInitProperties();
}

#if WITH_EDITOR
void ABezierCurveActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ABezierCurveActor, ControlPoints))
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

        RebuildCurve();
    }
}
#endif

void ABezierCurveActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildCurve();
}

void ABezierCurveActor::RebuildCurve()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->RebuildCurve(ControlPoints);
    }
}

void ABezierCurveActor::FillPointsToTarget()
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

    const FVector Step = Direction / (FillSegmentCount + 1);

    for (int32 i = 1; i <= FillSegmentCount; ++i)
    {
        ControlPoints.Add(LastPoint + Step * i);
    }

    CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(ControlPoints.Last() + Step);
    RebuildCurve();
}

void ABezierCurveActor::FillPointsRandomToTarget()
{
    if (ControlPoints.Num() == 0 || !CurveTargetScene || !CurveTargetScene->BillboardComponentEnd
        || TargetStepDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

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
    RebuildCurve();
}

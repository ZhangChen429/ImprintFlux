#include "CurveScribeActor.h"
#include "CurveScribeScene.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"

ACurveScribeActor::ACurveScribeActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bRunConstructionScriptOnDrag = true;

    
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    BillboardComponentBegin = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardBeginComponent"));
    BillboardComponentBegin->SetupAttachment(RootComponent);

    // 曲线目标子树
    CurveTargetScene = CreateDefaultSubobject<UCurveScribeScene>(TEXT("CurveTargetScene"));
    CurveTargetScene->SetupAttachment(RootComponent);

    // 子组件在 Actor 中创建，挂到 CurveTargetScene 下
    CurveTargetScene->BillboardComponentEnd = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardEndComponent"));
    CurveTargetScene->BillboardComponentEnd->SetupAttachment(CurveTargetScene);

    CurveTargetScene->SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    CurveTargetScene->SplineComponent->SetupAttachment(CurveTargetScene);

    CurveTargetScene->SplineComponentLeft = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponentLeft"));
    CurveTargetScene->SplineComponentLeft->SetupAttachment(CurveTargetScene);
    CurveTargetScene->SplineComponentLeft->EditorUnselectedSplineSegmentColor  = FLinearColor(1.0f, 0.85f, 0.0f); // 黄 - 左墙
    CurveTargetScene->SplineComponentLeft->EditorSelectedSplineSegmentColor    = FLinearColor(1.0f, 0.85f, 0.0f);

    CurveTargetScene->SplineComponentRight = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponentRight"));
    CurveTargetScene->SplineComponentRight->SetupAttachment(CurveTargetScene);
    CurveTargetScene->SplineComponentRight->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 0.85f, 0.0f); // 黄 - 右墙
    CurveTargetScene->SplineComponentRight->EditorSelectedSplineSegmentColor   = FLinearColor(1.0f, 0.85f, 0.0f);

    CurveTargetScene->SplineComponentRandomA = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponentRandomA"));
    CurveTargetScene->SplineComponentRandomA->SetupAttachment(CurveTargetScene);
    CurveTargetScene->SplineComponentRandomA->EditorUnselectedSplineSegmentColor = FLinearColor(1.0f, 0.2f, 0.2f); // 红 - 随机 A
    CurveTargetScene->SplineComponentRandomA->EditorSelectedSplineSegmentColor   = FLinearColor(1.0f, 0.2f, 0.2f);

    CurveTargetScene->SplineComponentRandomB = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponentRandomB"));
    CurveTargetScene->SplineComponentRandomB->SetupAttachment(CurveTargetScene);
    CurveTargetScene->SplineComponentRandomB->EditorUnselectedSplineSegmentColor = FLinearColor(0.2f, 0.5f, 1.0f); // 蓝 - 随机 B
    CurveTargetScene->SplineComponentRandomB->EditorSelectedSplineSegmentColor   = FLinearColor(0.2f, 0.5f, 1.0f);

    // 控制点初始数据
    FillSegmentCount = 4;
    ControlPoints.Add(FVector(0.0f, 0.0f, 0.0f));
    ControlPoints.Add(FVector(100.0f, 200.0f, 0.0f));
    ControlPoints.Add(FVector(200.0f, -100.0f, 0.0f));
    ControlPoints.Add(FVector(300.0f, 0.0f, 0.0f));

    CurveTargetScene->BillboardComponentEnd->SetRelativeLocation(
        ControlPoints.Last() + FVector(100.0f, 0.0f, 0.0f));
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

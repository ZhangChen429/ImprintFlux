#include "CurveScribeActor.h"
#include "CurveScribeScene.h"
#include "Components/BillboardComponent.h"

ACurveScribeActor::ACurveScribeActor()
{
    bRunConstructionScriptOnDrag = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    // 一行完成 Scene + 全部子组件的创建与装配
    CurveTargetScene = UCurveScribeScene::CreateAndAttach(this, RootComponent);

    // 填充参数初始值
}

void ACurveScribeActor::PostInitProperties()
{
    Super::PostInitProperties();
}

void ACurveScribeActor::LoadFromDataAsset()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->LoadFromDataAsset();
    }
}

void ACurveScribeActor::BakeSceneTransformIntoControlPoints()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->BakeTransformIntoControlPoints();
    }
}

void ACurveScribeActor::SaveToDataAsset()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->SaveToDataAsset();
    }
}

#if WITH_EDITOR
void ACurveScribeActor::PostEditUndo()
{
    Super::PostEditUndo();
    if (CurveTargetScene)
    {
        CurveTargetScene->RebuildCurve();
        CurveTargetScene->NotifyControlPointsChanged();
    }
}
#endif

void ACurveScribeActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (CurveTargetScene)
    {
        CurveTargetScene->RebuildCurve();
        CurveTargetScene->NotifyControlPointsChanged();
    }
}

void ACurveScribeActor::FillPointsToTarget()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->FillPointsToTarget();
    }
}

void ACurveScribeActor::FillPointsRandomToTarget()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->FillPointsRandomToTarget();
    }
}

void ACurveScribeActor::RandomOffsetControlPoints()
{
    if (CurveTargetScene)
    {
        CurveTargetScene->RandomOffsetControlPoints();
    }
}

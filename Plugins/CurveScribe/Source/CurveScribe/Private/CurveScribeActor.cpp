#include "CurveScribeActor.h"
#include "CurveScribeScene.h"

ACurveScribeActor::ACurveScribeActor()
{
   
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

    // 一行完成 Scene + 全部子组件的创建与装配
#if WITH_EDITOR
    bRunConstructionScriptOnDrag = true;
    CurveTargetScene = UCurveScribeScene::CreateAndAttach(this, RootComponent);
#endif
    // 填充参数初始值
}

void ACurveScribeActor::PostInitProperties()
{
    Super::PostInitProperties();
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

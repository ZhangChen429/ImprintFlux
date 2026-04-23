#pragma once

#include "CoreMinimal.h"
#include "CurveScribeDataAsset.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "CurveScribeActor.generated.h"

class UCurveScribeScene;
class UBillboardComponent;

/**
 * 贝塞尔曲线 Actor
 * 控制点数据由 UCurveScribeScene 管理，Actor 作为宿主提供编辑器操作接口
 */
UCLASS(Meta = (PrioritizeCategories = "BezierData BezierActions BezierFill"))
class CURVESCRIBE_API ACurveScribeActor : public AActor
{
    GENERATED_BODY()

public:
    ACurveScribeActor();
    // ── 组件 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<UCurveScribeScene> CurveTargetScene;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;

    virtual void PostInitProperties() override;

#if WITH_EDITOR
    virtual void PostEditUndo() override;
#endif
};

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
UCLASS(Meta = (PrioritizeCategories = "BezierActions BezierFill BezierData"))
class CURVESCRIBE_API ACurveScribeActor : public AActor
{
    GENERATED_BODY()

public:
    ACurveScribeActor();
    
    // ── 操作函数 ──
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierFill", meta = (DisplayName = "根据目标点位生成沿线控制点"))
    void FillPointsToTarget();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierFill", meta = (DisplayName = "根据目标点位生成偏转控制点"))
    void FillPointsRandomToTarget();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierFill", meta = (DisplayName = "对所有控制点施加随机偏移"))
    void RandomOffsetControlPoints();

    // ── 数据资产引用 ──
    
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierData", meta = (DisplayName = "从数据资产加载"))
    void LoadFromDataAsset();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierData", meta = (DisplayName = "保存到数据资产"))
    void SaveToDataAsset();

    // 把 CurveTargetScene 的 RelativeTransform 烘焙进 ControlPoints / Billboard 本地位置，
    // 然后把 Scene 重置为 identity。结果：曲线视觉位置不变，MakeEditWidget 与曲线重新对齐。
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|Transform", meta = (DisplayName = "把 Scene Transform 烘焙到控制点"))
    void BakeSceneTransformIntoControlPoints();

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

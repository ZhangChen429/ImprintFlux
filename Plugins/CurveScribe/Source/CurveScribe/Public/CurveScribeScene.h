#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "CurveScribeScene.generated.h"

class USplineComponent;
class UBillboardComponent;

/**
 * 曲线目标组件 —— 只负责根据控制点生成样条线
 * 通过绑定 Actor 的 OnControlPointsChanged 委托自动刷新
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, PrioritizeCategories = "BezierActions"))
class CURVESCRIBE_API UCurveScribeScene : public USceneComponent
{
    GENERATED_BODY()

public:
    UCurveScribeScene();
    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    
    // ── Debug  ──
    UPROPERTY(EditAnywhere, Category = "Debug")
    float DebugCircleRadius = 20.f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    FColor DebugColor = FColor::Cyan;
    
    // ── 样条线参数 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (ClampMin = "10", ClampMax = "500"))
    int32 CurveResolution = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions")
    TEnumAsByte<ESplinePointType::Type> SplinePointType = ESplinePointType::Curve;

    // ── 子组件 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<UBillboardComponent> BillboardComponentEnd;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponent;

    // 根据传入的控制点重建样条线
    void RebuildCurve(const TArray<FVector>& InControlPoints);

    // 绑定到 Owner Actor 的委托
    void BindToOwner();

protected:
    virtual void OnRegister() override;

private:
    // 委托回调
    void HandleControlPointsChanged(const TArray<FVector>& InControlPoints);

    FVector CalculateBezierPoint(const TArray<FVector>& Points, float T) const;

    FDelegateHandle DelegateHandle;
};

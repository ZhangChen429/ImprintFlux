#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "CurveScribeScene.generated.h"

class USplineComponent;
class UBillboardComponent;

// Debug 圆环开关变更委托：广播新的显示状态
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowDebugCirclesChanged, bool);

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


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (DisplayName = "显示 Debug 圆环"))
    bool bShowDebugCircles = true;

    UPROPERTY(EditAnywhere, Category = "Debug")
    FColor DebugColor = FColor::Cyan;

    UPROPERTY(EditAnywhere, Category = "Debug")
    FColor DebugInnerColor = FColor::Red;

    // Debug 开关变更委托（外部可绑定以同步状态）
    FOnShowDebugCirclesChanged OnShowDebugCirclesChanged;

    // 运行时切换 Debug 显示（会触发委托广播）
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void SetShowDebugCircles(bool bNewShow);

    // ── 样条线参数 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (ClampMin = "10", ClampMax = "500"))
    int32 CurveResolution = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions")
    TEnumAsByte<ESplinePointType::Type> SplinePointType = ESplinePointType::Curve;

    // ── 安全走廊 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|Corridor", meta = (ClampMin = "0", DisplayName = "走廊半径"))
    float CorridorRadius = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|Corridor", meta = (ClampMin = "0", DisplayName = "随机最小偏移半径"))
    float RandomOffsetMinRadius = 10.f;

    // ── 子组件 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<UBillboardComponent> BillboardComponentEnd;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponentLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponentRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponentRandomA;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<USplineComponent> SplineComponentRandomB;

    void RebuildCurve(const TArray<FVector>& InControlPoints);

    // 根据内层采样点重建左右两条走廊管壁
    void RebuildCorridor(const TArray<FVector>& InCurvePoints, const FVector& WorldOrigin);

    // 在走廊内生成两条随机偏移的贝塞尔样条线
    void RebuildRandomInCorridor(const TArray<FVector>& InControlPoints, const FVector& WorldOrigin);

    // 绑定到 Owner Actor 的委托
    void BindToOwner();

protected:
    virtual void OnRegister() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    // 委托回调
    void HandleControlPointsChanged(const TArray<FVector>& InControlPoints);

    FVector CalculateBezierPoint(const TArray<FVector>& Points, float T) const;

    FDelegateHandle DelegateHandle;
};

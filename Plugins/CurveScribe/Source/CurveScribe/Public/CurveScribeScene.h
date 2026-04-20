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
 * CreateAndAttach 使用的子组件命名配置。
 * 默认值保持与历史 Actor 一致；同一 Actor 需挂多套时请自定义以避免 FName 冲突。
 */
struct CURVESCRIBE_API FCurveScribeSubcomponentNames
{
    FName Scene          = TEXT("CurveTargetScene");
    FName BillboardBegin = TEXT("BillboardBeginComponent");
    FName BillboardEnd   = TEXT("BillboardEndComponent");
    FName Spline         = TEXT("SplineComponent");
    FName SplineLeft     = TEXT("SplineComponentLeft");
    FName SplineRight    = TEXT("SplineComponentRight");
    FName SplineRandomA  = TEXT("SplineComponentRandomA");
    FName SplineRandomB  = TEXT("SplineComponentRandomB");
};

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

    /**
     * 一步创建并装配 Scene + 全部子 SceneComponent。
     * 必须在宿主 Actor 的构造函数中调用（否则 CreateDefaultSubobject 会断言失败）。
     * @param Owner     宿主 Actor（当前正在被构造）
     * @param AttachTo  Scene 要附加到的父级（通常是 Owner->RootComponent）
     */

    static UCurveScribeScene* CreateAndAttach(
        AActor* Owner,
        USceneComponent* AttachTo,
        const FCurveScribeSubcomponentNames& Names = FCurveScribeSubcomponentNames());

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (ClampMin = "10", ClampMax = "500",DisplayName = "曲线分段数量"))
    int32 CurveResolution = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions")
    TEnumAsByte<ESplinePointType::Type> SplinePointType = ESplinePointType::Curve;
    
    // ── 安全走廊 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|Corridor", meta = (ClampMin = "0", DisplayName = "走廊半径"))
    float CorridorRadius = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|Corridor", meta = (ClampMin = "0", DisplayName = "随机最小偏移半径"))
    float RandomOffsetMinRadius = 10.f;

    // ── 子组件 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<UBillboardComponent> BillboardComponentBegin;

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

    // 根据内层采样点重建左右两条走廊管壁（点存于 Scene 本地空间）
    void RebuildCorridor(const TArray<FVector>& InCurvePoints);

    // 在走廊内生成两条随机偏移的贝塞尔样条线（点存于 Scene 本地空间）
    void RebuildRandomInCorridor(const TArray<FVector>& InControlPoints);

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

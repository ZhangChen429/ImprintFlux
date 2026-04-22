#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "CurveScribeScene.generated.h"

class USplineComponent;
class UBillboardComponent;
class UCurveScribeDataAsset;

// 控制点变更委托：广播时携带最新的 ControlPoints
DECLARE_MULTICAST_DELEGATE_OneParam(FOnControlPointsChanged, const TArray<FVector>&);

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
 * 曲线目标组件 —— 持有控制点数据并负责生成样条线
 * 数据权威源：ControlPoints、SelectedControlPointIndex 均在此管理
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, PrioritizeCategories = "BezierActions"))
class CURVESCRIBE_API UCurveScribeScene : public USceneComponent
{
    GENERATED_BODY()

public:
    UCurveScribeScene();

    // ── 控制点变更委托 ──
    FOnControlPointsChanged OnControlPointsChanged;

    // ── 控制点数据 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (DisplayName = "控制点数组"))
    TArray<FVector> ControlPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (MakeEditWidget = true))
    bool bShow3DWidget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions")
    int32 SelectedControlPointIndex = INDEX_NONE;

    // 广播控制点变更，通知所有绑定的监听者刷新
    void NotifyControlPointsChanged();

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

    // ── Debug：控制点朝向圆环 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (DisplayName = "显示控制点圆环"))
    bool bShowControlPointCircles = false;

    UPROPERTY(EditAnywhere, Category = "Debug", meta = (DisplayName = "控制点圆环颜色"))
    FColor ControlPointCircleColor = FColor::Green;

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

    // ── Offset ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierFill", meta = (DisplayName = "填充段数", ClampMin = "1"))
    int32 FillSegmentCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierFill", meta = (DisplayName = "最大偏转角度", ClampMin = "0", ClampMax = "90"))
    float MaxDeviationAngle = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierFill", meta = (DisplayName = "步进距离"))
    float TargetStepDistance = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierFill", meta = (DisplayName = "控制点随机偏移最小半径", ClampMin = "0"))
    float ControlPointRandomOffsetMin = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierFill", meta = (DisplayName = "控制点随机偏移最大半径", ClampMin = "0"))
    float ControlPointRandomOffsetMax = 600.f;
    
    //============================核心函数重建曲线=================================
    void RebuildCurve();

    // 根据内层采样点重建左右两条走廊管壁（点存于 Scene 本地空间）
    void RebuildCorridor(const TArray<FVector>& InCurvePoints);

    // 在走廊内生成两条随机偏移的贝塞尔样条线（点存于 Scene 本地空间）
    void RebuildRandomInCorridor(const TArray<FVector>& InControlPoints);

    // ── 填充操作 ──
    UFUNCTION(BlueprintCallable, Category = "BezierFill", meta = (DisplayName = "根据目标点位生成沿线控制点"))
    void FillPointsToTarget();

    UFUNCTION(BlueprintCallable, Category = "BezierFill", meta = (DisplayName = "根据目标点位生成偏转控制点"))
    void FillPointsRandomToTarget();

    UFUNCTION(BlueprintCallable, Category = "BezierFill", meta = (DisplayName = "对所有控制点施加随机偏移"))
    void RandomOffsetControlPoints();
    
    // ── 数据资产 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierData", meta = (DisplayName = "曲线数据资产"))
    TObjectPtr<UCurveScribeDataAsset> CurveData;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierData", meta = (DisplayName = "曲线数据资产"))
    TObjectPtr<UCurveFloat> CircularTubeData;
    
    UFUNCTION(BlueprintCallable, Category = "BezierData", meta = (DisplayName = "从数据资产加载"))
    void LoadFromDataAsset();

    UFUNCTION(BlueprintCallable, Category = "BezierData", meta = (DisplayName = "保存到数据资产"))
    void SaveToDataAsset();
    
    UFUNCTION(BlueprintCallable, Category = "BezierActions|Transform", meta = (DisplayName = "把 Scene Transform 烘焙到控制点"))
    void BakeTransformIntoControlPoints();

    // 绑定到 Owner Actor 的委托
    void BindToOwner();

protected:
    virtual void OnRegister() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditUndo() override;
#endif

private:
    FVector CalculateBezierPoint(const TArray<FVector>& Points, float T) const;

    // 归一化参数 T∈[0,1] 处的管径乘数；CircularTubeData 为空时返回 1
    float GetTubeScaleAt(float T) const;

    // 沿曲线归一化参数 T∈[0,1] 处的走廊半径：CorridorRadius * GetTubeScaleAt(T)
    float GetCorridorRadiusAt(float T) const;

    // 沿曲线归一化参数 T∈[0,1] 处的最小偏移半径：RandomOffsetMinRadius * GetTubeScaleAt(T)
    float GetMinOffsetRadiusAt(float T) const;
};

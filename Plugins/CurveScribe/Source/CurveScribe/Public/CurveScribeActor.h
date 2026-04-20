#pragma once

#include "CoreMinimal.h"
#include "CurveScribeDataAsset.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "CurveScribeActor.generated.h"

class UCurveScribeScene;
class UBillboardComponent;

/**
 * 专属标记组件，只挂载在 ABezierCurveActor 上
 * FBezierCurveVisualizer 注册到此类型，确保 DrawVisualization 仅对贝塞尔曲线 Actor 触发
 */
UCLASS(ClassGroup=(Custom), HideCategories=(Activation, Cooking, AssetUserData, Collision))
class CURVESCRIBE_API UBezierVisualizerComponent : public UActorComponent
{
    GENERATED_BODY()
};

// 控制点变更委托：广播时携带最新的 ControlPoints
DECLARE_MULTICAST_DELEGATE_OneParam(FOnControlPointsChanged, const TArray<FVector>&);

/**
 * 贝塞尔曲线 Actor
 * 控制点数据由 Actor 管理，通过委托通知 UCurveTargetScene 刷新样条线
 */
UCLASS(Meta = (PrioritizeCategories = "BezierActions"))
class CURVESCRIBE_API ACurveScribeActor : public AActor
{
    GENERATED_BODY()

public:
    ACurveScribeActor();

    // ── 控制点变更委托 ──
    FOnControlPointsChanged OnControlPointsChanged;

    // ── 控制点数据 ──
    // 注意：去掉 MakeEditWidget，3D 编辑由 FBezierCurveVisualizer 接管（基底=Scene 的 ComponentTransform）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (DisplayName = "控制点数组"))
    TArray<FVector> ControlPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions", meta = (MakeEditWidget = true))
    bool bShow3DWidget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierActions")
    int32 SelectedControlPointIndex = INDEX_NONE;

    // ── 填充参数 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierFill", meta = (DisplayName = "填充段数", ClampMin = "1"))
    int32 FillSegmentCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierFill", meta = (DisplayName = "最大偏转角度", ClampMin = "0", ClampMax = "90"))
    float MaxDeviationAngle = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierFill", meta = (DisplayName = "步进距离"))
    float TargetStepDistance = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierFill", meta = (DisplayName = "控制点随机偏移最小半径", ClampMin = "0"))
    float ControlPointRandomOffsetMin = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierFill", meta = (DisplayName = "控制点随机偏移最大半径", ClampMin = "0"))
    float ControlPointRandomOffsetMax = 150.f;

    // ── 操作函数 ──
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|BezierFill", meta = (DisplayName = "根据目标点位生成沿线控制点"))
    void FillPointsToTarget();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|BezierFill", meta = (DisplayName = "根据目标点位生成偏转控制点"))
    void FillPointsRandomToTarget();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|BezierFill", meta = (DisplayName = "对所有控制点施加随机偏移"))
    void RandomOffsetControlPoints();
    
    // ── 数据资产引用 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|BezierData", meta = (DisplayName = "曲线数据资产"))
    TObjectPtr<UCurveScribeDataAsset> CurveData;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|BezierData", meta = (DisplayName = "从数据资产加载"))
    void LoadFromDataAsset();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|BezierData", meta = (DisplayName = "保存到数据资产"))
    void SaveToDataAsset();

    // 把 CurveTargetScene 的 RelativeTransform 烘焙进 ControlPoints / Billboard 本地位置，
    // 然后把 Scene 重置为 identity。结果：曲线视觉位置不变，MakeEditWidget 与曲线重新对齐。
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "BezierActions|Transform",meta = (DisplayName = "把 Scene Transform 烘焙到控制点"))
    void BakeSceneTransformIntoControlPoints();

    // 广播控制点变更，通知所有绑定的监听者刷新
    void NotifyControlPointsChanged();

    // ── Debug：控制点朝向圆环 ──
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|Debug", meta = (DisplayName = "显示控制点圆环"))
    bool bShowControlPointCircles = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierActions|Debug", meta = (DisplayName = "控制点圆环半径", ClampMin = "0"))
    float ControlPointCircleRadius = 30.f;

    UPROPERTY(EditAnywhere, Category = "BezierActions|Debug", meta = (DisplayName = "控制点圆环颜色"))
    FColor ControlPointCircleColor = FColor::Green;

    // ── 组件 ──
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    TObjectPtr<UCurveScribeScene> CurveTargetScene;

protected:

    virtual void OnConstruction(const FTransform& Transform) override;

    virtual void PostInitProperties() override;

    virtual void Tick(float DeltaTime) override;

    // 在编辑器视口中（非 PIE）也让 Actor Tick，用于持续绘制 Debug 圆环
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditUndo() override;
#endif

    //// 为了保证工具稳定暂时关闭编辑器可视化工具
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bezier Curve")
    //TObjectPtr<UBezierVisualizerComponent> BezierVisualizerComponent;
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/SplineComponent.h"
#include "UEBlenderBlueprintLibrary.generated.h"

/**
 * UEBlender 蓝图函数库
 */
UCLASS()
class CURVESCRIBE_API UUEBlenderBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * 从 Transform 数组创建样条线组件
     * @param WorldContext 世界上下文对象
     * @param Transforms Transform 数组（位置、旋转、缩放）
     * @param bClosedLoop 是否闭合样条线
     * @param SplineType 样条线类型
     * @return 创建的样条线组件
     */
    UFUNCTION(BlueprintCallable, Category = "UEBlender|Spline", meta = (WorldContext = "WorldContext", DeterminesOutputType = "SplineClass"))
    static USplineComponent* CreateSplineFromTransforms(
        UObject* WorldContext,
        const TArray<FTransform>& Transforms,
        bool bClosedLoop = false,
        TEnumAsByte<ESplineCoordinateSpace::Type> CoordinateSpace = ESplineCoordinateSpace::World
    );

    /**
     * 从 Transform 数组更新现有样条线
     * @param SplineComponent 要更新的样条线组件
     * @param Transforms Transform 数组
     * @param bClosedLoop 是否闭合样条线
     */
    UFUNCTION(BlueprintCallable, Category = "UEBlender|Spline")
    static void UpdateSplineFromTransforms(
        USplineComponent* SplineComponent,
        const TArray<FTransform>& Transforms,
        bool bClosedLoop = false
    );

    /**
     * 创建带样条线的 Actor
     * @param WorldContext 世界上下文对象
     * @param Transforms Transform 数组
     * @param bClosedLoop 是否闭合样条线
     * @return 创建的 Actor
     */
    UFUNCTION(BlueprintCallable, Category = "UEBlender|Spline", meta = (WorldContext = "WorldContext"))
    static AActor* CreateSplineActorFromTransforms(
        UObject* WorldContext,
        const TArray<FTransform>& Transforms,
        bool bClosedLoop = false
    );

    /**
     * 获取样条线在指定位置的 Transform
     * @param SplineComponent 样条线组件
     * @param Distance 沿样条线的距离
     * @param CoordinateSpace 坐标空间
     * @return 该位置的 Transform
     */
    UFUNCTION(BlueprintPure, Category = "UEBlender|Spline")
    static FTransform GetSplineTransformAtDistance(
        USplineComponent* SplineComponent,
        float Distance,
        TEnumAsByte<ESplineCoordinateSpace::Type> CoordinateSpace = ESplineCoordinateSpace::World
    );

    /**
     * 将 Actor 数组转换为 Transform 数组
     * @param Actors Actor 数组
     * @return Transform 数组
     */
    UFUNCTION(BlueprintPure, Category = "UEBlender|Spline")
    static TArray<FTransform> ActorsToTransforms(const TArray<AActor*>& Actors);
};

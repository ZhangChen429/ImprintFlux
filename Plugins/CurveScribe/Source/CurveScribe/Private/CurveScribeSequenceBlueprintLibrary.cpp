#include "CurveScribeSequenceBlueprintLibrary.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

USplineComponent* UCurveScribeSequenceBlueprintLibrary::CreateSplineFromTransforms(
    UObject* WorldContext,
    const TArray<FTransform>& Transforms,
    bool bClosedLoop,
    TEnumAsByte<ESplineCoordinateSpace::Type> CoordinateSpace)
{
    if (!WorldContext || Transforms.Num() < 2)
    {
        return nullptr;
    }

    UWorld* World = WorldContext->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 创建一个新的 Actor 来存放样条线组件
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* SplineActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (!SplineActor)
    {
        return nullptr;
    }

    // 创建样条线组件
    USplineComponent* SplineComponent = NewObject<USplineComponent>(SplineActor, TEXT("SplineComponent"));
    SplineComponent->RegisterComponent();
    SplineComponent->SetMobility(EComponentMobility::Movable);

    // 设置样条线点
    UpdateSplineFromTransforms(SplineComponent, Transforms, bClosedLoop);

    return SplineComponent;
}

void UCurveScribeSequenceBlueprintLibrary::UpdateSplineFromTransforms(
    USplineComponent* SplineComponent,
    const TArray<FTransform>& Transforms,
    bool bClosedLoop)
{
    if (!SplineComponent || Transforms.Num() < 2)
    {
        return;
    }

    // 清空现有点
    SplineComponent->ClearSplinePoints(false);

    // 添加新的点
    for (int32 i = 0; i < Transforms.Num(); ++i)
    {
        const FTransform& Transform = Transforms[i];

        // 添加样条点 (新版UE返回void)
        SplineComponent->AddSplinePoint(Transform.GetLocation(), ESplineCoordinateSpace::World, false);

        // 获取刚添加的点索引
        const int32 PointIndex = SplineComponent->GetNumberOfSplinePoints() - 1;

        // 设置切线类型
        SplineComponent->SetSplinePointType(PointIndex, ESplinePointType::Curve, false);

        // 设置点的旋转
        SplineComponent->SetRotationAtSplinePoint(PointIndex, Transform.GetRotation().Rotator(), ESplineCoordinateSpace::World, false);

        // 设置点的缩放
        SplineComponent->SetScaleAtSplinePoint(PointIndex, Transform.GetScale3D(), false);
    }

    // 设置是否闭合
    SplineComponent->SetClosedLoop(bClosedLoop, false);

    // 更新样条线
    SplineComponent->UpdateSpline();
}

AActor* UCurveScribeSequenceBlueprintLibrary::CreateSplineActorFromTransforms(
    UObject* WorldContext,
    const TArray<FTransform>& Transforms,
    bool bClosedLoop)
{
    if (!WorldContext || Transforms.Num() < 2)
    {
        return nullptr;
    }

    UWorld* World = WorldContext->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 创建一个新的 Actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 使用第一个 Transform 的位置作为 Actor 的生成位置
    const FVector SpawnLocation = Transforms.Num() > 0 ? Transforms[0].GetLocation() : FVector::ZeroVector;

    AActor* SplineActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    if (!SplineActor)
    {
        return nullptr;
    }

    // 创建样条线组件作为根组件
    USplineComponent* SplineComponent = NewObject<USplineComponent>(SplineActor, TEXT("SplineComponent"));
    SplineComponent->RegisterComponent();
    SplineActor->SetRootComponent(SplineComponent);

    // 更新样条线
    UpdateSplineFromTransforms(SplineComponent, Transforms, bClosedLoop);

    return SplineActor;
}

FTransform UCurveScribeSequenceBlueprintLibrary::GetSplineTransformAtDistance(
    USplineComponent* SplineComponent,
    float Distance,
    TEnumAsByte<ESplineCoordinateSpace::Type> CoordinateSpace)
{
    if (!SplineComponent)
    {
        return FTransform::Identity;
    }

    const float SplineLength = SplineComponent->GetSplineLength();
    if (SplineLength <= 0.0f)
    {
        return FTransform::Identity;
    }

    // 限制距离在有效范围内
    const float ClampedDistance = FMath::Clamp(Distance, 0.0f, SplineLength);

    // 获取位置
    const FVector Location = SplineComponent->GetLocationAtDistanceAlongSpline(ClampedDistance, CoordinateSpace);

    // 获取切线方向计算旋转
    const FVector Tangent = SplineComponent->GetTangentAtDistanceAlongSpline(ClampedDistance, CoordinateSpace);
    const FRotator Rotation = Tangent.Rotation();

    // 获取缩放
    const float InputKey = SplineComponent->GetInputKeyAtDistanceAlongSpline(ClampedDistance);
    const FVector Scale = SplineComponent->GetScaleAtSplinePoint(FMath::TruncToInt32(InputKey));

    return FTransform(Rotation.Quaternion(), Location, Scale);
}

TArray<FTransform> UCurveScribeSequenceBlueprintLibrary::ActorsToTransforms(const TArray<AActor*>& Actors)
{
    TArray<FTransform> Transforms;
    Transforms.Reserve(Actors.Num());

    for (AActor* Actor : Actors)
    {
        if (Actor)
        {
            Transforms.Add(Actor->GetActorTransform());
        }
    }

    return Transforms;
}

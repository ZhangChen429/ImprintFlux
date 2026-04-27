#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CurveScribeSequenceBuilder.generated.h"

class ULevelSequence;
class ACineCameraActor;

/**
 * CurveScribe → LevelSequence 工具集。
 * 流水线分步骤推进：先创建含相机和 CurveActor 绑定的 Sequence 骨架，后续再把相机绑到曲线上。
 */
UCLASS()
class CAMERATRACKTOOLEDITOR_API UCurveScribeSequenceBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * 步骤 1：对每个传入的 SplineComponentName，在指定路径下创建独立的 LevelSequence 资产
     * （命名 = SequenceAssetBaseName + "_" + SplineName），各自生成 CineCameraActor，
     * 把相机与 CurveActor 以 Possessable 绑定到 Sequence，设置 PlaybackRange，
     * 并直接在该 Sequence 上添加 3D Path Track（相机沿对应 spline 走完整个 sequence）。
     *
     * @param CurveActor              场景中已有的曲线 actor
     * @param SequencePackagePath     资产目录，例如 "/Game/Sequences"
     * @param SequenceAssetBaseName   资产名前缀，例如 "Shot01"
     * @param SplineComponentNames    要使用的 spline 组件名数组（每个生成一个 Sequence）
     * @param DurationSeconds         Sequence 时长（秒，>0），用于设置 PlaybackRange
     * @param OutSequences            返回每个 spline 对应的 LevelSequence 资产
     * @param OutSpawnedCameras       返回每个 Sequence 对应的相机 actor
     * @param bFollowPath             是否开启相机跟随
     * @return 全部成功返回 true；任何一个失败返回 false（成功创建的部分仍会写入 Out 数组）
     */
    UFUNCTION(BlueprintCallable, Category = "CurveScribe|Sequence",
              meta = (DisplayName = "创建 Sequence (按 Spline 列表批量)"))
    static bool CreateSequenceWithCameraAndCurveActor(
        AActor* CurveActor,
        const FString& SequencePackagePath,
        const FString& SequenceAssetBaseName,
        const TArray<FName>& SplineComponentNames,
        float DurationSeconds,
        TArray<ULevelSequence*>& OutSequences,
        TArray<ACineCameraActor*>& OutSpawnedCameras,bool bFollowPath);

    /**
     * 步骤 2：在相机 binding 上添加 3D Path Track，约束对象为 Sequence 中的 CurveActor binding。
     * 引擎原生路径约束：相机沿 CurveActor 的 SplineComponent 移动，由 Sequencer 运行时求值，
     */
    UFUNCTION(BlueprintCallable, Category = "CurveScribe|Sequence")
    static bool AddPathTrackFromCurveActor(
        ULevelSequence* Sequence,
        FName SplineComponentName = TEXT("SplineComponent"),bool const bFollowPath = false);
};

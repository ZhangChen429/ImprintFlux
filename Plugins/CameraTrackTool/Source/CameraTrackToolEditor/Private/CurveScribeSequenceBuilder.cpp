#include "CurveScribeSequenceBuilder.h"

#include "LevelSequence.h"
#include "MovieScene.h"
#include "MovieSceneObjectBindingID.h"
#include "CineCameraActor.h"
#include "CurveScribeActor.h"

#include "Tracks/MovieScene3DPathTrack.h"
#include "Sections/MovieScene3DPathSection.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Editor.h"
#include "Engine/World.h"

bool UCurveScribeSequenceBuilder::CreateSequenceWithCameraAndCurveActor(
    ACurveScribeActor* CurveActor,
    const FString& SequencePackagePath,
    const FString& SequenceAssetBaseName,
    const TArray<FName>& SplineComponentNames,
    float DurationSeconds,
    TArray<ULevelSequence*>& OutSequences,
    TArray<ACineCameraActor*>& OutSpawnedCameras)
{
    OutSequences.Reset();
    OutSpawnedCameras.Reset();

    if (!CurveActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] CurveActor is null"));
        return false;
    }
    if (SequencePackagePath.IsEmpty() || SequenceAssetBaseName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] PackagePath or BaseName is empty"));
        return false;
    }
    if (SplineComponentNames.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] SplineComponentNames is empty"));
        return false;
    }
    if (DurationSeconds <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] DurationSeconds must be > 0"));
        return false;
    }
    if (!GEditor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] GEditor not available"));
        return false;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] No editor world available"));
        return false;
    }

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    bool bAllSucceeded = true;
    for (const FName& SplineName : SplineComponentNames)
    {
        if (SplineName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] 跳过空 SplineName"));
            bAllSucceeded = false;
            continue;
        }

        const FString AssetName = SequenceAssetBaseName + TEXT("_") + SplineName.ToString();

        // 1. 创建 LevelSequence 资产
        UObject* NewAsset = AssetTools.CreateAsset(AssetName, SequencePackagePath,
                                                   ULevelSequence::StaticClass(), nullptr);
        ULevelSequence* Sequence = Cast<ULevelSequence>(NewAsset);
        if (!Sequence)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] CreateAsset failed: %s/%s"),
                   *SequencePackagePath, *AssetName);
            bAllSucceeded = false;
            continue;
        }
        Sequence->Initialize();

        // 2. 生成 CineCameraActor
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ACineCameraActor* Camera = World->SpawnActor<ACineCameraActor>(
            ACineCameraActor::StaticClass(),
            CurveActor->GetActorTransform(),
            SpawnParams);
        if (!Camera)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] SpawnActor<ACineCameraActor> failed for %s"),
                   *AssetName);
            bAllSucceeded = false;
            continue;
        }
        Camera->SetActorLabel(AssetName + TEXT("_Camera"));

        UMovieScene* MovieScene = Sequence->GetMovieScene();
        if (!MovieScene)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] %s 无 MovieScene"), *AssetName);
            bAllSucceeded = false;
            continue;
        }

        // 3. 设置 PlaybackRange = [0, DurationSeconds]
        {
            const FFrameRate   TickResolution = MovieScene->GetTickResolution();
            const FFrameNumber StartFrame     = FFrameNumber(0);
            const FFrameNumber EndFrame       = (DurationSeconds * TickResolution).FloorToFrame();
            MovieScene->SetPlaybackRange(TRange<FFrameNumber>(StartFrame, EndFrame));
        }

        // 4. 绑定 Camera 与 CurveActor
        const FGuid CameraGuid = MovieScene->AddPossessable(Camera->GetActorLabel(), Camera->GetClass());
        Sequence->BindPossessableObject(CameraGuid, *Camera, World);

        const FGuid CurveGuid = MovieScene->AddPossessable(CurveActor->GetActorLabel(), CurveActor->GetClass());
        Sequence->BindPossessableObject(CurveGuid, *CurveActor, World);

        Sequence->MarkPackageDirty();
        MovieScene->MarkPackageDirty();

        // 5. 直接添加对应 spline 的 PathTrack
        if (!AddPathTrackFromCurveActor(Sequence, SplineName))
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack 失败: %s / %s"),
                   *AssetName, *SplineName.ToString());
            bAllSucceeded = false;
        }

        OutSequences.Add(Sequence);
        OutSpawnedCameras.Add(Camera);

        UE_LOG(LogTemp, Log,
               TEXT("[CurveScribeSequenceBuilder] Created %s/%s; Camera=%s; Spline=%s"),
               *SequencePackagePath, *AssetName, *Camera->GetName(), *SplineName.ToString());
    }

    return bAllSucceeded;
}

bool UCurveScribeSequenceBuilder::AddPathTrackFromCurveActor(
    ULevelSequence* Sequence,
    FName SplineComponentName)
{
    if (!Sequence)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack: Sequence is null"));
        return false;
    }

    UMovieScene* MovieScene = Sequence->GetMovieScene();
    if (!MovieScene)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack: Sequence has no MovieScene"));
        return false;
    }

    // 从 Possessable 中按类查找：第一个 ACineCameraActor 和第一个 ACurveScribeActor
    FGuid CameraBindingGuid;
    FGuid CurveActorBindingGuid;
    const int32 PossessableCount = MovieScene->GetPossessableCount();
    for (int32 i = 0; i < PossessableCount; ++i)
    {
        const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
        const   UClass* PossessedClass = Possessable.GetPossessedObjectClass();
        if (!PossessedClass) { continue; }

        if (!CameraBindingGuid.IsValid() && PossessedClass->IsChildOf(ACineCameraActor::StaticClass()))
        {
            CameraBindingGuid = Possessable.GetGuid();
        }
        else if (!CurveActorBindingGuid.IsValid() && PossessedClass->IsChildOf(ACurveScribeActor::StaticClass()))
        {
            CurveActorBindingGuid = Possessable.GetGuid();
        }

        if (CameraBindingGuid.IsValid() && CurveActorBindingGuid.IsValid()) { break; }
    }

    if (!CameraBindingGuid.IsValid() || !CurveActorBindingGuid.IsValid())
    {
        UE_LOG(LogTemp, Warning,
               TEXT("[CurveScribeSequenceBuilder] AddPathTrack: 未在 Sequence 中找到 ACineCameraActor 或 ACurveScribeActor 的 Possessable"));
        return false;
    }

    // 直接采用 Sequence 的 PlaybackRange 作为 Path Section 范围
    const TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
    if (!PlaybackRange.HasLowerBound() || !PlaybackRange.HasUpperBound())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack: Sequence PlaybackRange 未设置"));
        return false;
    }
    const FFrameNumber StartFrame    = PlaybackRange.GetLowerBoundValue();
    const FFrameNumber EndFrame      = PlaybackRange.GetUpperBoundValue();
    const int32        DurationTicks = (EndFrame - StartFrame).Value;
    if (DurationTicks <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack: PlaybackRange 长度 <= 0"));
        return false;
    }

    // 3. 在相机 binding 上找/建 3D Path Track
    UMovieScene3DPathTrack* PathTrack = MovieScene->FindTrack<UMovieScene3DPathTrack>(CameraBindingGuid);
    if (!PathTrack)
    {
        PathTrack = MovieScene->AddTrack<UMovieScene3DPathTrack>(CameraBindingGuid);
    }
    if (!PathTrack)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack: failed to add UMovieScene3DPathTrack"));
        return false;
    }


    const FMovieSceneObjectBindingID ConstraintBindingID{
        UE::MovieScene::FRelativeObjectBindingID(CurveActorBindingGuid)};
    
    const int32 SectionCountBefore = PathTrack->GetAllSections().Num();
    PathTrack->AddConstraint(StartFrame, DurationTicks, NAME_None /*SocketName*/, SplineComponentName, ConstraintBindingID);

    // 6. 把新 Section 拿出来，配置成"按 Section 时长自动从 0 走到 1"
    UMovieScene3DPathSection* NewPathSection = nullptr;
    const TArray<UMovieSceneSection*>& AllSections = PathTrack->GetAllSections();
    if (AllSections.Num() > SectionCountBefore)
    {
        NewPathSection = Cast<UMovieScene3DPathSection>(AllSections.Last());
    }
    if (NewPathSection)
    {
        NewPathSection->SetRange(TRange<FFrameNumber>(StartFrame, EndFrame));
        NewPathSection->bFollow         = true;   // 朝向沿曲线
        NewPathSection->bReverse        = false;
        NewPathSection->bForceUpright   = false;
        NewPathSection->FrontAxisEnum   = MovieScene3DPathSection_Axis::NEG_X;
        NewPathSection->UpAxisEnum      = MovieScene3DPathSection_Axis::Z;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack: AddConstraint did not yield a new section"));
    }

    Sequence->MarkPackageDirty();
    MovieScene->MarkPackageDirty();

    // === 调试日志：用于排查"相机不沿对应路径移动"问题 ===
    UE_LOG(LogTemp, Warning,
           TEXT("[Debug AddPathTrack] Sequence=%s | SplineComponentName=%s | CameraGUID=%s | CurveActorGUID=%s | TotalSectionsOnTrack=%d | Range=[%d,%d]"),
           *Sequence->GetName(),
           *SplineComponentName.ToString(),
           *CameraBindingGuid.ToString(),
           *CurveActorBindingGuid.ToString(),
           PathTrack->GetAllSections().Num(),
           StartFrame.Value, EndFrame.Value);

    if (NewPathSection)
    {
        UE_LOG(LogTemp, Warning,
               TEXT("[Debug AddPathTrack]   NewSection: ConstraintBindingID.Guid=%s | Range=[%d,%d]"),
               *NewPathSection->GetConstraintBindingID().GetGuid().ToString(),
               NewPathSection->GetInclusiveStartFrame().Value,
               NewPathSection->GetExclusiveEndFrame().Value);
    }

    // 同时打印 MovieScene 里所有 Possessable，确认相机/曲线是不是预期的那两个
    const int32 PossessableCount2 = MovieScene->GetPossessableCount();
    for (int32 i = 0; i < PossessableCount2; ++i)
    {
        const FMovieScenePossessable& P = MovieScene->GetPossessable(i);
        const UClass* Cls = P.GetPossessedObjectClass();
        UE_LOG(LogTemp, Warning,
               TEXT("[Debug AddPathTrack]   Possessable[%d]: Name=%s | Class=%s | GUID=%s"),
               i, *P.GetName(),
               Cls ? *Cls->GetName() : TEXT("<null>"),
               *P.GetGuid().ToString());
    }

    return true;
}


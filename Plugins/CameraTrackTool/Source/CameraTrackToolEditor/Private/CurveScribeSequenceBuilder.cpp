#include "CurveScribeSequenceBuilder.h"

#include "LevelSequence.h"
#include "MovieScene.h"
#include "MovieSceneObjectBindingID.h"
#include "CineCameraActor.h"

#include "Tracks/MovieScene3DPathTrack.h"
#include "Sections/MovieScene3DPathSection.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "Tracks/MovieSceneSpawnTrack.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "Components/SplineComponent.h"

// 通过反射从 CurveActor 获取 SplineComponent
static USplineComponent* GetSplineComponentFromCurveActor(AActor* CurveActor)
{
    if (!CurveActor)
    {
        return nullptr;
    }

    // 尝试获取 CurveTargetScene 属性（UCurveScribeScene* 类型）
    UClass* ActorClass = CurveActor->GetClass();
    FProperty* CurveSceneProperty = ActorClass->FindPropertyByName(TEXT("CurveTargetScene"));
    if (!CurveSceneProperty)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] CurveActor has no 'CurveTargetScene' property"));
        return nullptr;
    }

    // 获取属性值（UObject*）
    UObject* CurveSceneObject = nullptr;
    FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(CurveSceneProperty);
    if (ObjectProperty)
    {
        CurveSceneObject = ObjectProperty->GetObjectPropertyValue_InContainer(CurveActor);
    }

    if (!CurveSceneObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] CurveTargetScene is null"));
        return nullptr;
    }

    // 从 CurveScene 获取 SplineComponent 属性
    UClass* SceneClass = CurveSceneObject->GetClass();
    FProperty* SplineProperty = SceneClass->FindPropertyByName(TEXT("SplineComponent"));
    if (!SplineProperty)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] CurveScene has no 'SplineComponent' property"));
        return nullptr;
    }

    // 获取 SplineComponent
    USplineComponent* SplineComponent = nullptr;
    FObjectPropertyBase* SplineObjectProperty = CastField<FObjectPropertyBase>(SplineProperty);
    if (SplineObjectProperty)
    {
        SplineComponent = Cast<USplineComponent>(SplineObjectProperty->GetObjectPropertyValue_InContainer(CurveSceneObject));
    }

    return SplineComponent;
}

bool UCurveScribeSequenceBuilder::CreateSequenceWithCameraAndCurveActor(
    AActor* CurveActor,
    const FString& SequencePackagePath,
    const FString& SequenceAssetBaseName,
    const TArray<FName>& SplineComponentNames,
    float DurationSeconds,
    TArray<ULevelSequence*>& OutSequences,
    TArray<ACineCameraActor*>& OutSpawnedCameras,
    bool bFollowPath)
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

    // 通过反射获取 SplineComponent 用于初始位置和朝向
    USplineComponent* SplineComponent = GetSplineComponentFromCurveActor(CurveActor);

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

        // 4. 添加 Camera 为 Spawnable（不在关卡中生成，由 Sequencer 控制）
        const FString CameraName = AssetName + TEXT("_Camera");
        ACineCameraActor* CameraTemplate = NewObject<ACineCameraActor>(MovieScene, ACineCameraActor::StaticClass(), *CameraName);
        FGuid CameraGuid = MovieScene->AddSpawnable(CameraName, *CameraTemplate);
        if (!CameraGuid.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] 添加 Camera Spawnable 失败: %s"), *AssetName);
            bAllSucceeded = false;
            continue;
        }

        // 获取主 SplineComponent 起始点的切线方向，用于设置相机初始朝向
        FVector TangentDirection = FVector::XAxisVector;
        FVector InitialLocation = CurveActor->GetActorLocation();
        if (SplineComponent)
        {
            if (SplineComponent->GetNumberOfSplinePoints() >= 2)
            {
                TangentDirection = SplineComponent->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::World).GetSafeNormal();
                InitialLocation = SplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
            }
        }

        // 计算让相机 -X 轴朝向切线方向的旋转
        FRotator InitialRotation = FRotationMatrix::MakeFromX(-TangentDirection).Rotator();

        // 添加 Spawn Track，控制相机在播放开始时生成、结束时销毁
        if (UMovieSceneSpawnTrack* SpawnTrack = MovieScene->AddTrack<UMovieSceneSpawnTrack>(CameraGuid))
        {
            UMovieSceneSpawnSection* SpawnSection = Cast<UMovieSceneSpawnSection>(SpawnTrack->CreateNewSection());
            if (SpawnSection)
            {
                SpawnSection->SetRange(MovieScene->GetPlaybackRange());
                // 在播放范围内设置为 Spawned
                TArrayView<FMovieSceneBoolChannel*> BoolChannels = SpawnSection->GetChannelProxy().GetChannels<FMovieSceneBoolChannel>();
                if (BoolChannels.Num() > 0)
                {
                    FMovieSceneBoolChannel* SpawnChannel = BoolChannels[0];
                    FFrameNumber StartTime = MovieScene->GetPlaybackRange().GetLowerBoundValue();
                    FFrameNumber EndTime = MovieScene->GetPlaybackRange().GetUpperBoundValue();

                }
                SpawnTrack->AddSection(*SpawnSection);
            }
        }

        // 添加 TransformTrack：设置初始位置和旋转
        UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(CameraGuid);
        if (TransformTrack)
        {
            UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
            if (TransformSection)
            {
                TransformSection->SetRange(MovieScene->GetPlaybackRange());
                TransformTrack->AddSection(*TransformSection);
                TArrayView<FMovieSceneFloatChannel*> Channels = TransformSection->GetChannelProxy().GetChannels<FMovieSceneFloatChannel>();
                FFrameNumber StartTime = MovieScene->GetPlaybackRange().GetLowerBoundValue();

                // 位置通道：初始位置在曲线起点
                if (Channels.IsValidIndex(0)) Channels[0]->AddConstantKey(StartTime, InitialLocation.X);
                if (Channels.IsValidIndex(1)) Channels[1]->AddConstantKey(StartTime, InitialLocation.Y);
                if (Channels.IsValidIndex(2)) Channels[2]->AddConstantKey(StartTime, InitialLocation.Z);

                // 旋转通道：初始朝向切线方向
                if (Channels.IsValidIndex(3)) Channels[3]->AddConstantKey(StartTime, InitialRotation.Roll);
                if (Channels.IsValidIndex(4)) Channels[4]->AddConstantKey(StartTime, InitialRotation.Pitch);
                if (Channels.IsValidIndex(5)) Channels[5]->AddConstantKey(StartTime, InitialRotation.Yaw);
            }
        }

        // 添加摄像机剪切轨道
        if (UMovieSceneCameraCutTrack* CameraCutTrack = MovieScene->AddTrack<UMovieSceneCameraCutTrack>())
        {
            UMovieSceneCameraCutSection* CameraCutSection = Cast<UMovieSceneCameraCutSection>(CameraCutTrack->CreateNewSection());
            if (CameraCutSection)
            {
                CameraCutSection->SetRange(MovieScene->GetPlaybackRange());
                CameraCutSection->SetCameraBindingID(FMovieSceneObjectBindingID(CameraGuid));
                CameraCutTrack->AddSection(*CameraCutSection);
            }
        }

        const FGuid CurveGuid = MovieScene->AddPossessable(CurveActor->GetActorLabel(), CurveActor->GetClass());
        Sequence->BindPossessableObject(CurveGuid, *CurveActor, World);

        Sequence->MarkPackageDirty();
        MovieScene->MarkPackageDirty();

        // 5. 直接添加对应 spline 的 PathTrack
        if (!AddPathTrackFromCurveActor(Sequence, SplineName,bFollowPath))
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurveScribeSequenceBuilder] AddPathTrack 失败: %s / %s"),
                   *AssetName, *SplineName.ToString());
            bAllSucceeded = false;
        }

        OutSequences.Add(Sequence);
        // Camera 是 Spawnable，不在关卡中生成，不返回指针

        UE_LOG(LogTemp, Log,
               TEXT("[CurveScribeSequenceBuilder] Created %s/%s; CameraSpawnable=%s; Spline=%s"),
               *SequencePackagePath, *AssetName, *CameraName, *SplineName.ToString());
    }

    return bAllSucceeded;
}

bool UCurveScribeSequenceBuilder::AddPathTrackFromCurveActor(
    ULevelSequence* Sequence,
    FName SplineComponentName,bool const bFollowPath)
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

    // 从 Spawnable 中查找 Camera，从 Possessable 中查找 CurveActor
    FGuid CameraBindingGuid;
    FGuid CurveActorBindingGuid;

    // 查找 Camera Spawnable
    const int32 SpawnableCount = MovieScene->GetSpawnableCount();
    for (int32 i = 0; i < SpawnableCount; ++i)
    {
        const FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(i);
        if (Spawnable.GetObjectTemplate() && Spawnable.GetObjectTemplate()->GetClass()->IsChildOf(ACineCameraActor::StaticClass()))
        {
            CameraBindingGuid = Spawnable.GetGuid();
            break;
        }
    }

    // 查找 CurveActor Possessable - 通过反射检查类名或属性
    const int32 PossessableCount = MovieScene->GetPossessableCount();
    for (int32 i = 0; i < PossessableCount; ++i)
    {
        const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
        const UClass* PossessedClass = Possessable.GetPossessedObjectClass();
        if (PossessedClass)
        {
            // 检查类名是否包含 "CurveScribeActor" 或检查是否有 CurveTargetScene 属性
            if (PossessedClass->GetName().Contains(TEXT("CurveScribeActor")) ||
                PossessedClass->FindPropertyByName(TEXT("CurveTargetScene")) != nullptr)
            {
                CurveActorBindingGuid = Possessable.GetGuid();
                break;
            }
        }
    }

    if (!CameraBindingGuid.IsValid() || !CurveActorBindingGuid.IsValid())
    {
        UE_LOG(LogTemp, Warning,
               TEXT("[CurveScribeSequenceBuilder] AddPathTrack: 未在 Sequence 中找到 Camera Spawnable 或 CurveActor Possessable"));
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
        NewPathSection->bFollow         = bFollowPath;   // 朝向沿曲线
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


    return true;
}

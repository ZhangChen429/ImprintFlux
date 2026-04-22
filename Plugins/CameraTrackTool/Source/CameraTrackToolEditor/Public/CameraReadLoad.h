// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelSequence.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "CameraReadLoad.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FTransformTrackRef
{
	GENERATED_BODY()
	// 所属LevelSequence（软引用）
	UPROPERTY(BlueprintReadOnly, Category = "Track")
	TSoftObjectPtr<ULevelSequence> Sequence;

	// 绑定GUID（唯一标识）
	UPROPERTY(BlueprintReadOnly, Category = "Track")
	FGuid BindingGuid;

	// 轨道索引
	UPROPERTY(BlueprintReadOnly, Category = "Track")
	int32 TrackIndex = INDEX_NONE;

	// 快速判断是否有效
	bool IsValid() const { return Sequence.IsValid() && BindingGuid.IsValid() && TrackIndex != INDEX_NONE; }
};

UCLASS()
class CAMERATRACKTOOLEDITOR_API UCameraReadLoad : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
	
	UFUNCTION(BlueprintCallable, Category = "LevelSequence|CameraTrack|Editor")
	static void GetAllSequenceTracks(const FString& SearchPath,bool bRecursive,
									TArray<FString>& OutSequencePaths,
									TArray<FSoftObjectPath>& OutDuplicatedU,
									TArray<FSoftObjectPath>& OutDuplicatedV);

	UFUNCTION(BlueprintCallable, Category = "LevelSequence|CameraTrack|Editor")
	static void AutoDuplicateLevelSequencesWithUV( const FString& SearchPath, bool bRecursive,float CircularOffset,float AngleOffset,
													TArray<FSoftObjectPath>& OutDuplicatedAssets,TArray<FSoftObjectPath>& OutDuplicatedAssetsV);
	
	UFUNCTION(BlueprintCallable, Category = "LevelSequence|Track")
	static void InspectTransformTrackKeys(UMovieScene3DTransformTrack* InTransformTrack,bool isU,float CircularOffset,float AngleOffset);
	
	UFUNCTION(BlueprintCallable, Category = "Camera|Sequence")
	static int32 CountCameraTracksInSequence(ULevelSequence* InSequence);
	
	UFUNCTION(BlueprintCallable, Category = "LevelSequence|CameraTrack|Editor")
	static TArray<FTransformTrackRef> FindAllTransformTracks(ULevelSequence* InSequence);
	
	UFUNCTION(BlueprintCallable, Category = "LevelSequence|CameraTrack|Editor")
	static UMovieScene3DTransformTrack* GetTransformTrackFromRef(const FTransformTrackRef& TrackRef);
	
	// 读取指定路径下所有 Sequence 的相机 Transform 关键帧，每个 Sequence 导出一份 JSON 到 OutputJsonPath
	UFUNCTION(BlueprintCallable, Category = "LevelSequence|CameraTrack|Editor")
	static void ExportSequenceCameraTransformsToJson(const FString& SearchPath, bool bRecursive,
	                                                 const FString& OutputJsonPath,
	                                                 TArray<FString>& OutSequenceNames);

	void LogLevelSequenceAllTracks(FSoftObjectPath SequencePath);
};

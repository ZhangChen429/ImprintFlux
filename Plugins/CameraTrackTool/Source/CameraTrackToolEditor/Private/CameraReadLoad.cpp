// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraReadLoad.h"
#include "MovieScene.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Tracks/MovieSceneCameraCutTrack.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

void UCameraReadLoad::GetAllSequenceTracks(const FString& SearchPath, bool bRecursive,
                                           TArray<FString>& OutSequencePaths, 
                                           TArray<FSoftObjectPath>& OutDuplicatedU,
                                           TArray<FSoftObjectPath>& OutDuplicatedV)
{
	OutSequencePaths.Empty();
	OutDuplicatedU.Empty();
	OutDuplicatedV.Empty();

	// 资产注册表
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// 过滤LevelSequence
	FARFilter Filter;
	Filter.ClassPaths.Add(ULevelSequence::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(*SearchPath);
	Filter.bRecursivePaths = bRecursive;
	Filter.bIncludeOnlyOnDiskAssets = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	// 遍历统计，并对每个 Sequence 复制两份（_U / _V）
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	for (const FAssetData& Asset : AssetList)
	{
		ULevelSequence* Seq = Cast<ULevelSequence>(Asset.GetAsset());
		if (!Seq) continue;

		OutSequencePaths.Add(Asset.GetObjectPathString());


		// 获取原资产所在目录和名称
		const FString PackagePath = Asset.PackagePath.ToString();
		const FString AssetName   = Asset.AssetName.ToString();

		// 复制 _U，并将软引用路径写入 OutDuplicatedU
		const FString NameU = AssetName + TEXT("_U");
		UObject* DupU = AssetTools.DuplicateAsset(NameU, PackagePath, Seq);
		if (DupU)
		{
			DupU->MarkPackageDirty();
			OutDuplicatedU.Add(FSoftObjectPath(DupU));
			UE_LOG(LogTemp, Log, TEXT("Duplicated _U: %s/%s"), *PackagePath, *NameU);
		}
		else
		{
			// 复制失败时仍按索引占位，保持与 OutSequencePaths 一一对应
			const FString FallbackPathU = FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *NameU, *NameU);
			OutDuplicatedU.Add(FSoftObjectPath(FallbackPathU));
			UE_LOG(LogTemp, Warning, TEXT("DuplicateAsset _U failed: %s/%s"), *PackagePath, *NameU);
		}

		// 复制 _V，并将软引用路径写入 OutDuplicatedV
		const FString NameV = AssetName + TEXT("_V");
		UObject* DupV = AssetTools.DuplicateAsset(NameV, PackagePath, Seq);
		if (DupV)
		{
			DupV->MarkPackageDirty();
			OutDuplicatedV.Add(FSoftObjectPath(DupV));
			UE_LOG(LogTemp, Log, TEXT("Duplicated _V: %s/%s"), *PackagePath, *NameV);
		}
		else
		{
			const FString FallbackPathV = FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *NameV, *NameV);
			OutDuplicatedV.Add(FSoftObjectPath(FallbackPathV));
			UE_LOG(LogTemp, Warning, TEXT("DuplicateAsset _V failed: %s/%s"), *PackagePath, *NameV);
		}
	}
}

void UCameraReadLoad::AutoDuplicateLevelSequencesWithUV(const FString& SearchPath, bool bRecursive,float CircularOffset,float AngleOffset,
	TArray<FSoftObjectPath>& OutDuplicatedAssetsU,TArray<FSoftObjectPath>& OutDuplicatedAssetsV)
{
	OutDuplicatedAssetsU.Empty();
    OutDuplicatedAssetsV.Empty();

    // 1. 获取资产注册表
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // 2. 过滤 LevelSequence
    FARFilter Filter;
    Filter.ClassPaths.Add(ULevelSequence::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(*SearchPath);
    Filter.bRecursivePaths = bRecursive;
    Filter.bIncludeOnlyOnDiskAssets = true;

    TArray<FAssetData> AssetList;
    AssetRegistry.GetAssets(Filter, AssetList);

    // 3. 遍历资产，按尾缀 _U / _V 分类并直接处理
    for (const FAssetData& Asset : AssetList)
    {
        const FString AssetName = Asset.AssetName.ToString();

        // 只关心 _U 或 _V 结尾的资产
        const bool bIsU = AssetName.EndsWith(TEXT("_U"));
        const bool bIsV = AssetName.EndsWith(TEXT("_V"));
        if (!bIsU && !bIsV) continue;

        ULevelSequence* Seq = Cast<ULevelSequence>(Asset.GetAsset());
        if (!IsValid(Seq)) continue;

        FSoftObjectPath SoftPath(Seq);

        if (bIsV)
        {
            OutDuplicatedAssetsV.Add(SoftPath);
            TArray<FTransformTrackRef> TransformTracks = FindAllTransformTracks(Seq);
            for (const FTransformTrackRef& Ref : TransformTracks)
            {
                UMovieScene3DTransformTrack* Track = GetTransformTrackFromRef(Ref);
                if (Track)
                {
                    InspectTransformTrackKeys(Track, false, CircularOffset, AngleOffset);
                }
            }
        }
        else // bIsU
        {
            OutDuplicatedAssetsU.Add(SoftPath);
            TArray<FTransformTrackRef> TransformTracks = FindAllTransformTracks(Seq);
            for (const FTransformTrackRef& Ref : TransformTracks)
            {
                UMovieScene3DTransformTrack* Track = GetTransformTrackFromRef(Ref);
                if (Track)
                {
                    InspectTransformTrackKeys(Track, true, CircularOffset, AngleOffset);
                }
            }
        }
    }
}

void UCameraReadLoad::InspectTransformTrackKeys(UMovieScene3DTransformTrack* InTransformTrack,bool isU,float CircularOffset,float AngleOffset)
{
	if (!IsValid(InTransformTrack))
	{
		UE_LOG(LogTemp, Warning, TEXT("InspectTransformTrackKeys: 轨道无效"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("✅ 检查轨道：%s"), *InTransformTrack->GetName());

	for (UMovieSceneSection* Section : InTransformTrack->GetAllSections())
	{
		UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);
		if (!TransformSection) continue;

		UE_LOG(LogTemp, Log, TEXT("   → 片段：%s"), *TransformSection->GetName());

		// 直接按类型获取所有 Double 通道
		// 顺序固定：TX(0) TY(1) TZ(2) RX(3) RY(4) RZ(5) SX(6) SY(7) SZ(8)
		TArrayView<FMovieSceneDoubleChannel*> DoubleChannels =
			TransformSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();

		// 通道名称映射，方便日志阅读
		static const TArray<FString> ChannelNames = {
			TEXT("Translation.X"), TEXT("Translation.Y"), TEXT("Translation.Z"),
			TEXT("Rotation.X"),    TEXT("Rotation.Y"),    TEXT("Rotation.Z"),
			TEXT("Scale.X"),       TEXT("Scale.Y"),       TEXT("Scale.Z")
		};
		if (Section != InTransformTrack->GetAllSections()[0])
		{
			
		}
		for (int32 i = 0; i < DoubleChannels.Num(); ++i)
		{
			FMovieSceneDoubleChannel* Channel = DoubleChannels[i];
			if (!Channel) continue;

			// 获取所有关键帧的时间和值
			TArrayView<const FFrameNumber>         KeyTimes  = Channel->GetData().GetTimes();
			TArrayView<const FMovieSceneDoubleValue> KeyValues = Channel->GetData().GetValues();

			const FString& ChName = ChannelNames.IsValidIndex(i) ? ChannelNames[i] : FString::Printf(TEXT("Channel[%d]"), i);
			if (ChName==TEXT("Rotation.Z"))
			{
				TransformSection->Modify();  // ✅ 标记修改，确保保存生效
				TArrayView<FMovieSceneDoubleValue> Keys = Channel->GetData().GetValues();
				for (FMovieSceneDoubleValue& Key : Keys)
				{
					if (isU)
					{
						Key.Value -= AngleOffset;
					}
					else
					{
						Key.Value += AngleOffset;
					}
				}
				
			}
			
			if (ChName==TEXT("Translation.X"))
			{
				TransformSection->Modify();  // ✅ 标记修改，确保保存生效
				TArrayView<FMovieSceneDoubleValue> Keys = Channel->GetData().GetValues();
				for (FMovieSceneDoubleValue& Key : Keys)
				{
					if (isU)
					{
						Key.Value -= CircularOffset + FMath::RandRange(-20.0f, 20.0f);
					}
					else
					{
						Key.Value += CircularOffset + FMath::RandRange(-20.0f, 20.0f);
					}
				}
				
			}
			if (ChName==TEXT("Translation.Y") )
			{
				TransformSection->Modify();  // ✅ 标记修改，确保保存生效
				TArrayView<FMovieSceneDoubleValue> Keys = Channel->GetData().GetValues();
				for (FMovieSceneDoubleValue& Key : Keys)
				{
					if (isU)
					{
						Key.Value -= CircularOffset + FMath::RandRange(-20.0f, 20.0f);
					}
					else
					{
						Key.Value += CircularOffset + FMath::RandRange(-20.0f, 20.0f);
					}
				}
				
			}
			
			UE_LOG(LogTemp, Log, TEXT("      [%s] 关键帧数量：%d"), *ChName, KeyTimes.Num());

			for (int32 k = 0; k < KeyTimes.Num(); ++k)
			{
				UE_LOG(LogTemp, Log, TEXT("         帧 %d → 值：%.4f"),
					KeyTimes[k].Value,
					KeyValues[k].Value);
			}
		}
	}
}

TArray<FTransformTrackRef> UCameraReadLoad::FindAllTransformTracks(ULevelSequence* InSequence)
{
	TArray<FTransformTrackRef> OutTrackRefs;

	if (!IsValid(InSequence))
		return OutTrackRefs;

	UMovieScene* MovieScene = InSequence->GetMovieScene();
	if (!IsValid(MovieScene))
		return OutTrackRefs;

	// 遍历所有绑定（安全方式）
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		const TArray<UMovieSceneTrack*>& Tracks = Binding.GetTracks();
        
		for (int32 i = 0; i < Tracks.Num(); ++i)
		{
			UMovieSceneTrack* Track = Tracks[i];
			if (Track && Track->IsA<UMovieScene3DTransformTrack>())
			{
				// ✅ 构建编辑器安全引用
				FTransformTrackRef Ref;
				Ref.Sequence = InSequence;
				Ref.BindingGuid = Binding.GetObjectGuid();
				Ref.TrackIndex = i;

				OutTrackRefs.Add(Ref);
			}
		}
	}

	return OutTrackRefs;
}

void UCameraReadLoad::LogLevelSequenceAllTracks(FSoftObjectPath SequencePath)
{
	ULevelSequence* Seq = Cast<ULevelSequence>(SequencePath.TryLoad());
	UMovieScene* MovieScene = Seq->GetMovieScene();
	if (!MovieScene) return;;
	
	
	// ========== 1. 统计总轨道数（根轨道 + 所有绑定轨道） ==========
    int32 TotalTrackCount = 0;
    // 先加根轨道数
    TotalTrackCount += MovieScene->GetTracks().Num();
    // 再加所有绑定里的轨道数
    for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
    {
        TotalTrackCount += Binding.GetTracks().Num();
    }

    UE_LOG(LogTemp, Log, TEXT("=== Sequence: [%s]  总轨道数: %d ==="), *Seq->GetName(), TotalTrackCount);
    int32 TrackIndex = 0;

    // ========== 2. 遍历所有根轨道 ==========
    UE_LOG(LogTemp, Log, TEXT("--- 根轨道 (Master Tracks) ---"));
    const TArray<UMovieSceneTrack*>& MasterTracks = MovieScene->GetTracks();
    for (UMovieSceneTrack* Track : MasterTracks)
    {
        if (!IsValid(Track)) continue;

        const FString TrackName = Track->GetTrackName().ToString();
        const FString TrackType = Track->GetClass()->GetName();
        UE_LOG(LogTemp, Log, TEXT("  [%d] Type: %-50s  Name: %s"), TrackIndex++, *TrackType, *TrackName);
    }

    // ========== 3. 遍历所有对象绑定下的轨道（包含SpawnTrack） ==========
    UE_LOG(LogTemp, Log, TEXT("--- 对象绑定轨道 (Binding Tracks) ---"));
    const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();
    for (const FMovieSceneBinding& Binding : Bindings)
    {
        // 绑定对应的对象名（比如你的 ActorSpawner、CameraTarget_00）
        const FString BindingName = Binding.GetName();
        const TArray<UMovieSceneTrack*>& BindingTracks = Binding.GetTracks();

        if (BindingTracks.Num() <= 0) continue;

        UE_LOG(LogTemp, Log, TEXT("  绑定对象: [%s]  轨道数: %d"), *BindingName, BindingTracks.Num());
        for (UMovieSceneTrack* Track : BindingTracks)
        {
            if (!IsValid(Track)) continue;

            const FString TrackName = Track->GetTrackName().ToString();
            const FString TrackType = Track->GetClass()->GetName();
            UE_LOG(LogTemp, Log, TEXT("    [%d] Type: %-50s  Name: %s"), TrackIndex++, *TrackType, *TrackName);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("====================================="));
}

UMovieScene3DTransformTrack* UCameraReadLoad::GetTransformTrackFromRef(const FTransformTrackRef& TrackRef)
{
	if (!TrackRef.IsValid())
		return nullptr;

	// 同步安全加载
	ULevelSequence* Seq = TrackRef.Sequence.LoadSynchronous();
	if (!Seq)
		return nullptr;

	UMovieScene* MovieScene = Seq->GetMovieScene();
	if (!MovieScene)
		return nullptr;

	// 根据GUID找到绑定
	for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
	{
		if (Binding.GetObjectGuid() == TrackRef.BindingGuid)
		{
			const TArray<UMovieSceneTrack*>& Tracks = Binding.GetTracks();
			if (TrackRef.TrackIndex >= 0 && TrackRef.TrackIndex < Tracks.Num())
			{
				return Cast<UMovieScene3DTransformTrack>(Tracks[TrackRef.TrackIndex]);
			}
		}
	}

	return nullptr;
}

void UCameraReadLoad::ExportSequenceCameraTransformsToJson(const FString& SearchPath, bool bRecursive,
                                                           const FString& OutputJsonPath,
                                                           TArray<FString>& OutSequenceNames)
{
	OutSequenceNames.Empty();

	// 与 AutoDuplicateLevelSequencesWithUV 相同的资产查找流程
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(ULevelSequence::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(*SearchPath);
	Filter.bRecursivePaths = bRecursive; // 递归查找子文件夹资产
	Filter.bIncludeOnlyOnDiskAssets = true;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	// 创建根输出目录
	if (!PlatformFile.DirectoryExists(*OutputJsonPath))
	{
		PlatformFile.CreateDirectoryTree(*OutputJsonPath);
	}

	for (const FAssetData& Asset : AssetList)
		{
			ULevelSequence* Seq = Cast<ULevelSequence>(Asset.GetAsset());
			if (!IsValid(Seq)) continue;

			UMovieScene* MovieScene = Seq->GetMovieScene();
			if (!IsValid(MovieScene)) continue;

			const FString SeqName = Asset.AssetName.ToString();
			OutSequenceNames.Add(SeqName);

			const FFrameRate DisplayRate    = MovieScene->GetDisplayRate();
			const FFrameRate TickResolution = MovieScene->GetTickResolution();

			// --- 第1步：与 AutoDuplicateLevelSequencesWithUV 相同，先收集所有有效 Track ---
			struct FTrackEntry
			{
				FString                   BindingName;
				FMovieSceneDoubleChannel* Ch[9]; // TX TY TZ  RX RY RZ  SX SY SZ
			};
			TArray<FTrackEntry> Entries;

			TArray<FTransformTrackRef> TransformTracks = FindAllTransformTracks(Seq);
			for (const FTransformTrackRef& Ref : TransformTracks)
			{
				UMovieScene3DTransformTrack* Track = GetTransformTrackFromRef(Ref);
				if (!Track) continue;

				FString BindingName = Ref.BindingGuid.ToString();
				for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
				{
					if (Binding.GetObjectGuid() == Ref.BindingGuid)
					{
						BindingName = Binding.GetName();
						break;
					}
				}
				bool bIsCameraActor = false;
				FString CameraObjectName = BindingName;

				if (FMovieScenePossessable* Possessable = MovieScene->FindPossessable(Ref.BindingGuid))
				{
					const UClass* Class = Possessable->GetPossessedObjectClass();
					bIsCameraActor = Class && Class->GetName().Contains(TEXT("CameraActor"));
					CameraObjectName = BindingName; // Possessable 直接取 Binding 显示名
				}
				else if (FMovieSceneSpawnable* Spawnable = MovieScene->FindSpawnable(Ref.BindingGuid))
				{
					UObject* Template = Spawnable->GetObjectTemplate();
					bIsCameraActor = Template && Template->GetClass()->GetName().Contains(TEXT("CameraActor"));
					// Spawnable 的 UI 显示名来自模板 Actor 的 Label（即 Sequencer 中看到的 "01_24mm"）
					if (AActor* TemplateActor = Cast<AActor>(Template))
					{
						CameraObjectName = TemplateActor->GetActorLabel();
					}
				}
				if (!bIsCameraActor) continue;

				// 与 InspectTransformTrackKeys 相同：遍历 Section，读取 DoubleChannel
				for (UMovieSceneSection* Section : Track->GetAllSections())
				{
					UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);
					if (!TransformSection) continue;

					TArrayView<FMovieSceneDoubleChannel*> Channels =
						TransformSection->GetChannelProxy().GetChannels<FMovieSceneDoubleChannel>();
					if (Channels.Num() < 9) continue;

					FTrackEntry Entry;
					Entry.BindingName = CameraObjectName; // ✅ 这里用真正相机名
					for (int32 i = 0; i < 9; ++i)
						Entry.Ch[i] = Channels[i];
					Entries.Add(Entry);
					break;
				}
			}


		if (Entries.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("ExportCameraTransforms: %s 没有找到有效 TransformTrack"), *SeqName);
			continue;
		}

		// 帧范围：优先从 PlaybackRange 读取，否则从第一条 Track 的通道关键帧推算
		int32 StartFrame = 0, EndFrame = 0;
		const TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
		if (PlaybackRange.HasLowerBound() && PlaybackRange.HasUpperBound())
		{
			StartFrame = FFrameRate::TransformTime(FFrameTime(PlaybackRange.GetLowerBoundValue()), TickResolution, DisplayRate).GetFrame().Value;
			EndFrame   = FFrameRate::TransformTime(FFrameTime(PlaybackRange.GetUpperBoundValue()), TickResolution, DisplayRate).GetFrame().Value;
		}
		// 如果范围无效，从 TX 通道关键帧 Tick 推算
		if (StartFrame >= EndFrame && Entries[0].Ch[0])
		{
			TArrayView<const FFrameNumber> KeyTimes = Entries[0].Ch[0]->GetData().GetTimes();
			if (KeyTimes.Num() > 0)
			{
				StartFrame = FFrameRate::TransformTime(FFrameTime(KeyTimes[0]), TickResolution, DisplayRate).GetFrame().Value;
				EndFrame   = FFrameRate::TransformTime(FFrameTime(KeyTimes.Last()), TickResolution, DisplayRate).GetFrame().Value + 1;
			}
		}

		UE_LOG(LogTemp, Log, TEXT("ExportCameraTransforms: %s  Frames %d~%d  Tracks %d"),
			*SeqName, StartFrame, EndFrame, Entries.Num());

		// --- 第2步：逐帧 Evaluate，每帧建独立 FrameJson 后写入 RootJson ---
		TSharedRef<FJsonObject> RootJson = MakeShared<FJsonObject>();
		for (int32 FrameIdx = StartFrame; FrameIdx < EndFrame; ++FrameIdx)
		{
			const FFrameTime TickTime = FFrameRate::TransformTime(
				FFrameTime(FFrameNumber(FrameIdx)), DisplayRate, TickResolution);

			TSharedRef<FJsonObject> FrameJson = MakeShared<FJsonObject>();
			for (const FTrackEntry& Entry : Entries)
			{
				const double Def[9] = {0,0,0, 0,0,0, 1,1,1};
				double V[9];
				for (int32 i = 0; i < 9; ++i)
				{
					V[i] = Def[i];
					if (Entry.Ch[i])
						Entry.Ch[i]->Evaluate(TickTime, V[i]);
				}
				const FMatrix Matrix = FTransform(
					FRotator(V[4], V[5], V[3]),
					FVector(V[0], V[1], V[2]),
					FVector(V[6], V[7], V[8])).ToMatrixWithScale();

				FrameJson->SetStringField(Entry.BindingName, Matrix.ToString());
			}
			RootJson->SetObjectField(FString::FromInt(FrameIdx), FrameJson);
		}

		// 紧凑 JSON，与目标文件格式一致
		FString JsonString;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
		FJsonSerializer::Serialize(RootJson, Writer);

		// ====================== 核心修复：递归目录生成 ======================
		// 1. 统一处理搜索路径前缀（确保结尾带 /，正确截取相对路径）
		FString SearchPrefix = SearchPath;
		SearchPrefix /= TEXT(""); // 自动添加路径分隔符，最稳定

		// 2. 获取资产所在的原始路径
		FString AssetPackagePath = Asset.PackagePath.ToString();

		// 3. 截取相对于搜索路径的子目录（递归层级完全保留）
		FString RelativeDirectory;
		if (AssetPackagePath.StartsWith(SearchPrefix))
		{
			RelativeDirectory = AssetPackagePath.RightChop(SearchPrefix.Len());
		}

		// 4. 拼接最终输出目录 = 输出根目录 + 相对子目录（递归结构一致）
		FString FinalOutputDirectory = OutputJsonPath;
		if (!RelativeDirectory.IsEmpty())
		{
			FinalOutputDirectory /= RelativeDirectory;
		}

		// 5. 递归创建完整目录（自动生成所有嵌套子文件夹）
		if (!PlatformFile.DirectoryExists(*FinalOutputDirectory))
		{
			PlatformFile.CreateDirectoryTree(*FinalOutputDirectory);
		}

		// 6. 最终JSON文件路径
		FString JsonFilePath = FinalOutputDirectory / SeqName + TEXT(".json");
		FFileHelper::SaveStringToFile(JsonString, *JsonFilePath);

		UE_LOG(LogTemp, Log, TEXT("ExportCameraTransforms: %s → %s  (%d frames)"),
			*SeqName, *JsonFilePath, EndFrame - StartFrame);
	}
}

int32 UCameraReadLoad::CountCameraTracksInSequence(ULevelSequence* InSequence)
{
	if (!InSequence) return 0;

	UMovieScene* MovieScene = InSequence->GetMovieScene();
	if (!MovieScene) return 0;

	int32 CameraTrackCount = 0;

	// 遍历所有轨道，只筛选 CameraCutTrack（相机轨道）
	for (UMovieSceneTrack* Track : MovieScene->GetTracks())
	{
		if (Track && Track->IsA<UMovieSceneCameraCutTrack>())
		{
			CameraTrackCount++;
		}
	}

	return CameraTrackCount;
}
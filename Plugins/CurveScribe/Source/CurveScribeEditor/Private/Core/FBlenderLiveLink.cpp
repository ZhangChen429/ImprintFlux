
#include "Core/FBlenderLiveLink.h"

#include "MeshDescriptionBuilder.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"

FBlenderLiveLink::FBlenderLiveLink()
{
  
}

FBlenderLiveLink::~FBlenderLiveLink()
{
  Stop();
}

void FBlenderLiveLink::Start(const FString& Guid1, const FString& Guid2)
{
  
    if (Bridge.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("BlenderLiveLink: 已经在运行中"));
        return;
     }

  Bridge = MakeShared<FAlterMeshBridge>();
  if (Bridge->Initialize(Guid1, Guid2))
  {
   CurrentGuid = Guid2;
   UE_LOG(LogTemp, Log, TEXT("BlenderLiveLink: 开始监听 GUID=%s"), *Guid2);
  }
  else
  {
   UE_LOG(LogTemp, Error, TEXT("BlenderLiveLink: 初始化失败"));
   Bridge.Reset();
  }
}

void FBlenderLiveLink::Stop()
{
  if (Bridge.IsValid())
  {
    Bridge->Shutdown();
    Bridge.Reset();
  }
  LastDataHash = 0;
  CurrentGuid.Empty();
}

 bool FBlenderLiveLink::IsRunning() const
{
  return Bridge.IsValid() && Bridge->IsInitialized();
}

void FBlenderLiveLink::Tick(float DeltaTime)
{
	static int32 TickCount = 0;
	TickCount++;
	if (!IsRunning())
	{
		if (TickCount % 60 == 0) // 每秒输出一次
			UE_LOG(LogTemp, Warning, TEXT("LiveLink not running"));
		return;
	}

	// 非阻塞读取
	if (Bridge->ReadLock())
	{
		TArray<uint8> Data;
		if (Bridge->ReadBytes(Data) && Data.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("Tick %d: Read %d bytes"), TickCount, Data.Num());
			// 检查数据是否变化
			uint32 Hash = FCrc::MemCrc32(Data.GetData(), Data.Num());
			if (Data.Num() > 0 && Data.Num() != LastDataHash) // 简化检查
			{
				// 打印前8字节（Header）
				int32 VertexCount = *(int32*)Data.GetData();
				int32 IndexCount = *(int32*)(Data.GetData() + 4);
				UE_LOG(LogTemp, Log, TEXT("Header: %d verts, %d indices"), VertexCount, IndexCount);

				ProcessMeshData(Data);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ReadBytes failed"));
		}
		Bridge->ReadUnlock();
	}
	else
	{
		if (TickCount % 60 == 0)
			UE_LOG(LogTemp, Warning, TEXT("ReadLock failed - Blender may not be writing"));
	}
}

bool FBlenderLiveLink::IsTickable() const
{
	return IsRunning(); 
}

TStatId FBlenderLiveLink::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FBlenderLiveLink, STATGROUP_Tickables);
}


void FBlenderLiveLink::ProcessMeshData(const TArray<uint8>& Data)
{
	FMemoryReader Reader(Data);

	// 读取头部
	int32 VertexCount, IndexCount;
	Reader << VertexCount << IndexCount;

	if (VertexCount <= 0 || VertexCount > 10000000)
	{
		UE_LOG(LogTemp, Error, TEXT("BlenderLiveLink: 无效的顶点数 %d"), VertexCount);
		return;
	}

	// 读取顶点数据
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	Vertices.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);

	for (int32 i = 0; i < VertexCount; ++i)
	{
		float X, Y, Z;
		Reader << X << Y << Z;

		float NX, NY, NZ;
		Reader << NX << NY << NZ;

		float U, V;
		Reader << U << V;

		// Blender (Y-up) → UE (Z-up)
		Vertices.Add(FVector(Y, -X, Z));
		Normals.Add(FVector(NY, -NX, NZ));
		UVs.Add(FVector2D(U, V));
	}

	// 读取索引
	TArray<int32> Indices;
	Indices.Reserve(IndexCount);
	for (int32 i = 0; i < IndexCount; ++i)
	{
		int32 Index;
		Reader << Index;
		Indices.Add(Index);
	}

	UE_LOG(LogTemp, Log, TEXT("BlenderLiveLink: 收到网格 %d 顶点, %d 索引"),
		VertexCount, IndexCount);

	// 导入到 Editor
	ImportMeshToEditor(Vertices, Indices, Normals, UVs);
}

void FBlenderLiveLink::ImportMeshToEditor(const TArray<FVector>& Vertices, const TArray<int32>& Indices,
	const TArray<FVector>& Normals, const TArray<FVector2D>& UVs)
{
	// 生成唯一资产名
	FString AssetName = FString::Printf(TEXT("BlenderLive_%s"), *CurrentGuid.Left(8));
	FString PackagePath = FString::Printf(TEXT("/Game/BlenderLive/%s"), *AssetName);

	// 创建 Package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("无法创建 Package: %s"), *PackagePath);
		return;
	}

	// 创建 StaticMesh
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone);

	// 构建 MeshDescription
	FMeshDescription MeshDescription;
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register();

	FMeshDescriptionBuilder Builder;
	Builder.SetMeshDescription(&MeshDescription);

	// 添加顶点
	TArray<FVertexID> VertexIDs;
	VertexIDs.SetNum(Vertices.Num());

	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		VertexIDs[i] = Builder.AppendVertex(Vertices[i]);
	}
	
	FPolygonGroupID PolygonGroup = Builder.AppendPolygonGroup();
	// 添加三角形
	for (int32 i = 0; i < Indices.Num(); i += 3)
	{
		TArray<FVertexInstanceID> VertexInstanceIDs;

		for (int32 j = 0; j < 3; ++j)
		{
			int32 VertexIndex = Indices[i + j];

			FVertexInstanceID InstanceID = Builder.AppendInstance(VertexIDs[VertexIndex]);
			Builder.SetInstanceNormal(InstanceID, Normals[VertexIndex]);
			Builder.SetInstanceUV(InstanceID, UVs[VertexIndex], 0);

			VertexInstanceIDs.Add(InstanceID);
		}

		Builder.AppendTriangle(VertexInstanceIDs[0], VertexInstanceIDs[1], VertexInstanceIDs[2], PolygonGroup);
	}

	// 提交 MeshDescription
	StaticMesh->CommitMeshDescription(0);
	// 设置到 StaticMesh
	StaticMesh->Build();
	

	// 保存资产
	FAssetRegistryModule::AssetCreated(StaticMesh);
	Package->MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("BlenderLiveLink: 创建资产 %s"), *PackagePath);
}

//UStaticMesh* FBlenderLiveLink::CreateStaticMeshAsset(const FString& AssetName)
//{
//}
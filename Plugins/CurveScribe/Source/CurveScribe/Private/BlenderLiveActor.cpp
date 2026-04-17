// Fill out your copyright notice in the Description page of Project Settings.


#include "BlenderLiveActor.h"

#include "AlterMeshBridge.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralMeshComponent.h"


// Sets default values
ABlenderLiveActor::ABlenderLiveActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
	RootComponent = ProcMesh;
}

void ABlenderLiveActor::StartListening(const FString& Guid1, const FString& Guid2)
{
	if (!Bridge.IsValid())
	{
		Bridge = MakeShared<FAlterMeshBridge>();
	}

	// 这里才真正初始化共享内存
	if (Bridge->Initialize(Guid1, Guid2))
	{
		UE_LOG(LogTemp, Log, TEXT("BlenderLive: 开始监听 GUID=%s"), *Guid2);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BlenderLive: 初始化失败"));
	}
}

void ABlenderLiveActor::UpdateMeshFromData(const TArray<uint8>& Data)
{
	// 使用二进制读取（比 JSON 快 10 倍）
	FMemoryReader Reader(Data);

	// 读取头部信息
	int32 VertexCount, IndexCount;
	Reader << VertexCount;
	Reader << IndexCount;

	if (VertexCount <= 0 || VertexCount > 1000000) return; // 安全检查

	// 读取顶点数据（Blender: float3 position, float3 normal, float2 uv）
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;

	Vertices.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);

	for (int32 i = 0; i < VertexCount; ++i)
	{
		float X, Y, Z;
		Reader << X << Y << Z;  // 位置

		float NX, NY, NZ;
		Reader << NX << NY << NZ;  // 法线

		float U, V;
		Reader << U << V;  // UV

		// 坐标转换：Blender (Y-up) → UE (Z-up)
		// Blender: X=右, Y=上, Z=后
		// UE: X=前, Y=右, Z=上
		Vertices.Add(FVector(Y, -X, Z));  // 交换并翻转

		Normals.Add(FVector(NY, -NX, NZ));
		UVs.Add(FVector2D(U, V));
	}

	// 读取索引（三角形）
	TArray<int32> Indices;
	Indices.Reserve(IndexCount);

	for (int32 i = 0; i < IndexCount; ++i)
	{
		int32 Index;
		Reader << Index;
		Indices.Add(Index);
	}

	// 构建 ProceduralMesh 的 Section
	TArray<FProcMeshTangent> Tangents;
	TArray<FVector2D> UV0 = UVs;

	// 清空并创建新网格
	ProcMesh->ClearMeshSection(0);
	ProcMesh->CreateMeshSection(0, Vertices, Indices, Normals, UV0,
		VertexColors, Tangents, true);  // true = 碰撞

	UE_LOG(LogTemp, Log, TEXT("网格已更新: %d 顶点, %d 三角面"),
		VertexCount, IndexCount / 3);
}


// Called every frame
void ABlenderLiveActor::Tick(float DeltaTime)
{
	
	Super::Tick(DeltaTime);

	if (!Bridge.IsValid() || !Bridge->IsInitialized()) return;

	// 非阻塞检查新数据（5ms超时）
	if (Bridge->ReadLock())
	{
		TArray<uint8> Data;
		if (Bridge->ReadBytes(Data) && Data.Num() > 0)
		{
			// 计算数据哈希，避免重复处理
			uint32 Hash = FCrc::MemCrc32(Data.GetData(), Data.Num());
			if (Hash != LastDataHash)
			{
				LastDataHash = Hash;
				UpdateMeshFromData(Data);
			}
		}
		Bridge->ReadUnlock();
	}
}


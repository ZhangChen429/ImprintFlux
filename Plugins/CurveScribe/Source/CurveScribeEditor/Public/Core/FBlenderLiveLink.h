  #pragma once

#include "CoreMinimal.h"
#include "TickableEditorObject.h"
#include "AlterMeshBridge.h"

class FBlenderLiveLink : public FTickableObjectBase
{
public:
	FBlenderLiveLink();
	virtual ~FBlenderLiveLink();

	// 启动/停止监听
	void Start(const FString& Guid1, const FString& Guid2);
	void Stop();
	bool IsRunning() const;

	// FTickableEditorObject 接口
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;

	// 👇 就是缺了这个！必须加！
	virtual TStatId GetStatId() const override;

private:
	TSharedPtr<FAlterMeshBridge> Bridge;
	uint32 LastDataHash = 0;
	FString CurrentGuid;

	// 处理接收到的网格数据
	void ProcessMeshData(const TArray<uint8>& Data);

	// 在 Editor 中创建/更新 StaticMesh
	void ImportMeshToEditor(
		const TArray<FVector>& Vertices,
		const TArray<int32>& Indices,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UVs);

	// 创建 StaticMesh 资产
	//UStaticMesh* CreateStaticMeshAsset(const FString& AssetName);
	
};

  

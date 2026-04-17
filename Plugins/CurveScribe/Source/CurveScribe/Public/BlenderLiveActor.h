// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlenderLiveActor.generated.h"

class FAlterMeshBridge;
class UProceduralMeshComponent;

UCLASS()
class CURVESCRIBE_API ABlenderLiveActor : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UProceduralMeshComponent* ProcMesh;
	
	ABlenderLiveActor();

	void StartListening(const FString& Guid1, const FString& Guid2);
	
	virtual void Tick(float DeltaTime) override;

protected:
	TSharedPtr<FAlterMeshBridge> Bridge;

	// 缓存上一次的哈希，避免重复更新
	uint32 LastDataHash = 0;

	// 解析并更新网格
	void UpdateMeshFromData(const TArray<uint8>& Data);

	
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "CurveScribeDataAsset.h"

void UCurveScribeDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 逻辑：处理新增点位偏移
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
	{
		int32 NewIndex = ControlPoints.Num() - 1;
		if (NewIndex > 0) 
			ControlPoints[NewIndex] = ControlPoints[NewIndex - 1] + FVector(100.f, 0.f, 0.f);
	}

	// 广播：通知正在引用该 DataAsset 的 Actor 重新生成曲线
	OnDataChanged.Broadcast();
}

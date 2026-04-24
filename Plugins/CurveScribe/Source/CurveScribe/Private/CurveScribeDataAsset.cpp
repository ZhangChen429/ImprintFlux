// Fill out your copyright notice in the Description page of Project Settings.


#include "CurveScribeDataAsset.h"

UCurveScribeDataAsset::UCurveScribeDataAsset()
{
	// 初始化默认参数
	CurveResolution = 100;
	SplinePointType = ESplinePointType::Curve;
	FillSegmentCount = 4;
	MaxDeviationAngle = 15.0f;
	TargetStepDistance = 20.0f;
	CorridorRadius = 50.0f;
	RandomOffsetMinRadius = 10.0f;

	// 填充四个默认控制点 (例如沿 X 轴分布)
	if (ControlPoints.Num() == 0)
	{
		ControlPoints.Add(FVector(0.0f, 0.0f, 0.0f));      // 起点
		ControlPoints.Add(FVector(100.0f, 50.0f, 0.0f));   // 控制点 1
		ControlPoints.Add(FVector(200.0f, -50.0f, 0.0f));  // 控制点 2
		ControlPoints.Add(FVector(300.0f, 0.0f, 0.0f));    // 终点
	}

	// 默认管径缩放曲线：(0,1) → (1,1) 恒为 1
	if (TubeScaleCurve.GetNumKeys() == 0)
	{
		const FKeyHandle H0 = TubeScaleCurve.AddKey(0.f, 1.f);
		const FKeyHandle H1 = TubeScaleCurve.AddKey(1.f, 1.f);
		TubeScaleCurve.SetKeyInterpMode(H0, RCIM_Cubic);
		TubeScaleCurve.SetKeyInterpMode(H1, RCIM_Cubic);
	}
}

#if WITH_EDITOR
void UCurveScribeDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 逻辑：处理新增点位偏移
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
	{
		int32 NewIndex = ControlPoints.Num() - 1;
		if (NewIndex > 0)
			ControlPoints[NewIndex] = ControlPoints[NewIndex - 1] + FVector(100.f, 0.f, 0.f);
	}

	// 需要实时回调变更的属性列表
	static const TSet<FName> RealtimeProperties = {
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, TubeScaleCurve),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, bUseNoiseOffset),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, NoiseFrequency),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, NoiseSeedA),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, NoiseSeedB),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, CorridorRadius),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, RandomOffsetMinRadius),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, ControlPoints),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, CurveResolution),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, SplinePointType),
		GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, TargetStepDistance),
	};

	// TubeScaleCurve 的曲线编辑器修改时 Property 为 null，通过 MemberProperty 判断
	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// 如果变更的属性不在实时列表中，跳过
	if (!RealtimeProperties.Contains(PropertyName) &&
	    !RealtimeProperties.Contains(MemberPropertyName) &&
	    PropertyName != NAME_None)
	{
		return;
	}

	// 特殊处理 ControlPoints 的交互式拖拽：只在释放后才广播，避免拖拽过程中卡顿
	const bool bIsControlPointsProperty = (PropertyName == GET_MEMBER_NAME_CHECKED(UCurveScribeDataAsset, ControlPoints));
	if (bIsControlPointsProperty && PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		// 拖拽过程中跳过广播，等用户释放鼠标（ValueSet）时再更新
		return;
	}

	// 广播更新
	OnDataChanged.Broadcast();
}
#endif

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "AlterMeshBridge.h"

class FCurveScribeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FCurveScribeModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FCurveScribeModule>("CurveScribe");
	}

	// 获取桥接实例
	FAlterMeshBridge& GetBridge() { return Bridge; }

private:
	FAlterMeshBridge Bridge;
};

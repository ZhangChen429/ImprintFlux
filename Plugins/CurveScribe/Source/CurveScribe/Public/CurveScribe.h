// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FCurveScribeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FCurveScribeModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FCurveScribeModule>("CurveScribe");
	}

};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * PythonHot 主模块
 * 提供 Python 热重载和调试功能
 */
class FPythonHotModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FPythonHotModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FPythonHotModule>("PythonHot");
	}
};
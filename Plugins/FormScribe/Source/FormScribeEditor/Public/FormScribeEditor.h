#pragma once

#include "Modules/ModuleManager.h"

class FFormScribeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

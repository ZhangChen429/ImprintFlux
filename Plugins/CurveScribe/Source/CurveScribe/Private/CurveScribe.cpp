// Copyright Epic Games, Inc. All Rights Reserved.

#include "CurveScribe.h"


#define LOCTEXT_NAMESPACE "FUEBlenderModule"

void FCurveScribeModule::StartupModule()
{
}

void FCurveScribeModule::ShutdownModule()
{
	//Bridge.Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCurveScribeModule, CurveScribe)
// Copyright Epic Games, Inc. All Rights Reserved.

#include "PythonHot.h"

#define LOCTEXT_NAMESPACE "FPythonHotModule"

void FPythonHotModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("PythonHot: Module started"));
}

void FPythonHotModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("PythonHot: Module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPythonHotModule, PythonHot)
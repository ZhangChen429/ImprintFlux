#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CurveScribeSettings.generated.h"


UCLASS(config = UEBlender, meta = (DisplayName = "UEBlender"))
class CURVESCRIBE_API UCurveScribeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	// Select a path to blender.exe here
	UPROPERTY(EditAnywhere, config, Category = "CurveScribe", meta = (FilePathFilter = "json"))
	FFilePath ExecutablePath;

	// Blender 进程空闲超时时间（秒），超过后自动关闭
	UPROPERTY(EditAnywhere, config, Category = "CurveScribe")
	float MaxIdleTime = 30.f;
};

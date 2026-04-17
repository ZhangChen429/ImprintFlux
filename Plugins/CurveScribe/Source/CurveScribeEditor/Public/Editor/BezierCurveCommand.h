#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Style/BlenderPluginStyle.h"

/**
 * 贝塞尔曲线编辑器命令
 */
class FBezierCurveCommand : public TCommands<FBezierCurveCommand>
{
public:
	FBezierCurveCommand()
		: TCommands<FBezierCurveCommand>(
			FBlenderPluginStyle::GetStyleSetName(),
			NSLOCTEXT("BezierCurve", "BezierCurve", "Bezier Curve Commands"),
			NAME_None,
			FBlenderPluginStyle::GetStyleSetName()
		)
	{
	}

	virtual void RegisterCommands() override;

	// 打开贝塞尔曲线工具窗口命令
	TSharedPtr<FUICommandInfo> OpenBezierCurveWindow;
};

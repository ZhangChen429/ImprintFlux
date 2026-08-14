#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

/**
 * 贝塞尔曲线编辑器命令
 */
class FBezierCurveCommand : public TCommands<FBezierCurveCommand>
{
public:
	FBezierCurveCommand()
		: TCommands<FBezierCurveCommand>(
			TEXT("CurveScribeBezierCurve"),
			NSLOCTEXT("BezierCurve", "BezierCurve", "Bezier Curve Commands"),
			NAME_None,
			FAppStyle::GetAppStyleSetName()
		)
	{
	}

	virtual void RegisterCommands() override;

	// 打开贝塞尔曲线工具窗口命令
	TSharedPtr<FUICommandInfo> OpenBezierCurveWindow;
};

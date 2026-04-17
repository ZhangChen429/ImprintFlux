#include "Editor/BezierCurveCommand.h"

#define LOCTEXT_NAMESPACE "BezierCurveCommands"

void FBezierCurveCommand::RegisterCommands()
{
	UI_COMMAND(
		OpenBezierCurveWindow,
		"Bezier Curve",
		"Open Bezier Curve Generator Tool",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::C)
	);
}

#undef LOCTEXT_NAMESPACE

#include "GameInput.h"
#include "GameKeys.h"
#include "Joystick.h"
#include <assert.h>

// we use the game actions and 2 analog sticks (where 1 stick is 2 axes)
GameInput::GameInput() : GameControlState(ACTION_SIZE, 2+2)
{
	memset(&_emuActions[0], 0, sizeof(_emuActions));
}

Vector GameInput::getStick1() const
{
	return Vector(axes[0], axes[1]);
}

Vector GameInput::getStick2() const
{
	return Vector(axes[2], axes[3]);
}

void GameInput::update()
{
	Vector dir = getStick1();

	// Menu input is fired by sufficient analog input or normal dpad input
	emulateAction(ACTION_MENULEFT,  dir.x > JOY_AXIS_THRESHOLD || buttons[ACTION_SWIMLEFT]);
	emulateAction(ACTION_MENURIGHT, dir.x < -JOY_AXIS_THRESHOLD || buttons[ACTION_SWIMRIGHT]);
	emulateAction(ACTION_MENUDOWN,  dir.y > JOY_AXIS_THRESHOLD || buttons[ACTION_SWIMDOWN]);
	emulateAction(ACTION_MENUUP,    dir.y < -JOY_AXIS_THRESHOLD || buttons[ACTION_SWIMUP]);
}

void GameInput::emulateAction(unsigned ac, bool on)
{
	assert(ac < sizeof(_emuActions));
	if(_emuActions[ac] != on)
	{
		_emuActions[ac] = on;
		IInputMapper::ForwardAction(ac, on, -1, INP_DEV_NODEVICE);
	}
}

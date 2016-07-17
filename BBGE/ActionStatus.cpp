#include "ActionStatus.h"
#include "Core.h"
#include "ActionSet.h"

ActionButtonStatus::ActionButtonStatus()
: joystickID(-1)
{
	memset(status, 0, sizeof(status));
	memset(changed, 0, sizeof(changed));
}

void ActionButtonStatus::import(const ActionSet& as)
{
	joystickID = as.joystickID;

	unsigned char found[ACTION_BUTTON_ENUM_SIZE];
	memset(found, 0, sizeof(found));
	for(size_t i = 0; i < as.inputSet.size(); ++i)
	{
		const ActionInput& inp = as.inputSet[i];
		for(int j = 0; j < INP_COMBINED_SIZE; ++j)
			found[inp.all[j]] = 1;
	}
	
	toQuery.clear();
	for(int k = 1; k < sizeof(found); ++k) // ignore [0]
		if(found[k])
			toQuery.push_back(k);

	update();
}

void ActionButtonStatus::update()
{
	_queryAllStatus();
}

void ActionButtonStatus::_queryAllStatus()
{
	//memset(status, 0, sizeof(status));
	//memset(changed, 0, sizeof(changed));

	// k==0 is always 0
	//for(size_t i = 0; i < toQuery.size(); ++i)
	for(int k = 1; k < ACTION_BUTTON_ENUM_SIZE; ++k)
	{
		//const int k = toQuery[i];
		bool state = _queryStatus(k);
		changed[k] = !!status[k] != state;
		status[k] = state;
	}
}

bool ActionButtonStatus::_queryStatus(int k) const
{
	bool keyState = false;
	if (k > 0 && k < KEY_MAXARRAY)
	{
		keyState = core->getKeyState(k);
	}
	else if (k == MOUSE_BUTTON_LEFT)
	{
		keyState = core->mouse.buttons.left == DOWN;
	}
	else if (k == MOUSE_BUTTON_RIGHT)
	{
		keyState = core->mouse.buttons.right == DOWN;
	}
	else if (k == MOUSE_BUTTON_MIDDLE)
	{
		keyState = core->mouse.buttons.middle == DOWN;
	}
	else if (k >= MOUSE_BUTTON_EXTRA_START && k < MOUSE_BUTTON_EXTRA_END)
	{
		keyState  = core->mouse.buttons.extra[k - MOUSE_BUTTON_EXTRA_START];
	}
	else if (k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
	{
		Joystick *j = core->getJoystick(joystickID);
		if(j && j->isEnabled())
			keyState = j->getButton(k - JOY_BUTTON_0);
	}
	else if (k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		Joystick *j = core->getJoystick(joystickID);
		if(j && j->isEnabled())
		{
			float ax = j->getAxisUncalibrated(k - JOY_AXIS_0_POS);
			keyState = ax > JOY_AXIS_THRESHOLD;
		}
	}
	else if (k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		Joystick *j = core->getJoystick(joystickID);
		if(j && j->isEnabled())
		{
			float ax = j->getAxisUncalibrated(k - JOY_AXIS_0_NEG);
			keyState = ax < -JOY_AXIS_THRESHOLD;
		}
	}

	return keyState;
}

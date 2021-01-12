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
			if(unsigned(inp.data.all[j]) < ACTION_BUTTON_ENUM_SIZE)
				found[inp.data.all[j]] = 1;
	}

	// HACK: always query these for menu input emulation
	found[JOY_STICK_LEFT] = 1;
	found[JOY_STICK_RIGHT] = 1;
	found[JOY_STICK_UP] = 1;
	found[JOY_STICK_DOWN]= 1;
	
	toQuery.clear();
	for(int k = 1; k < sizeof(found); ++k) // ignore [0]
		if(found[k])
			toQuery.push_back(k);

	memset(status, 0, sizeof(status));
	memset(changed, 0, sizeof(changed));

	update();
}

void ActionButtonStatus::importQuery(const int *pKeys, size_t num)
{
	unsigned char found[ACTION_BUTTON_ENUM_SIZE];
	memset(found, 0, sizeof(found));
	for(size_t i = 0; i < num; ++i)
		if(unsigned(pKeys[i]) < ACTION_BUTTON_ENUM_SIZE)
			found[pKeys[i]] = 1;

	toQuery.clear();
	for(int k = 1; k < sizeof(found); ++k) // ignore [0]
		if(found[k])
			toQuery.push_back(k);

	memset(status, 0, sizeof(status));
	memset(changed, 0, sizeof(changed));

	update();
}

void ActionButtonStatus::update()
{
	_queryAllStatus();
}

void ActionButtonStatus::_queryAllStatus()
{
	// k==0 is always 0
	for(size_t i = 0; i < toQuery.size(); ++i)
	{
		const int k = toQuery[i];
		bool state = _queryStatus(k);
		changed[k] = !!status[k] != state;
		status[k] = state;
	}
}

bool ActionButtonStatus::_queryStatus(int k) const
{
	if (k > 0 && k < KEY_MAXARRAY)
		return core->getKeyState(k);

	if (k == MOUSE_BUTTON_LEFT)
		return core->mouse.buttons.left == DOWN;

	if (k == MOUSE_BUTTON_RIGHT)
		return core->mouse.buttons.right == DOWN;

	if (k == MOUSE_BUTTON_MIDDLE)
		return core->mouse.buttons.middle == DOWN;

	if (k >= MOUSE_BUTTON_EXTRA_START && k < MOUSE_BUTTON_EXTRA_END)
		return core->mouse.buttons.extra[k - MOUSE_BUTTON_EXTRA_START];

	// --- joystick from here ---

	Joystick *j = core->getJoystick(joystickID);
	if(!j || !j->isEnabled())
		return false;

	if (k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
		return j->getButton(k - JOY_BUTTON_0);

	if(k == JOY_STICK_LEFT)
		return j->position.x < -JOY_AXIS_THRESHOLD;
	if(k == JOY_STICK_RIGHT)
		return j->position.x > JOY_AXIS_THRESHOLD;
	if(k == JOY_STICK_UP)
		return j->position.y < -JOY_AXIS_THRESHOLD;
	if(k == JOY_STICK_DOWN)
		return j->position.y > JOY_AXIS_THRESHOLD;

	if (k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		float ax = j->getAxisUncalibrated(k - JOY_AXIS_0_POS);
		return ax > JOY_AXIS_THRESHOLD;
	}

	if (k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		float ax = j->getAxisUncalibrated(k - JOY_AXIS_0_NEG);
		return ax < -JOY_AXIS_THRESHOLD;
	}

	if(k >= JOY_HAT_BEGIN && k < JOY_HAT_END)
	{
		if (k >= JOY_HAT_0_LEFT && k < JOY_HAT_END_LEFT)
			return j->getHat(k - JOY_HAT_0_LEFT);

		if (k >= JOY_HAT_0_RIGHT && k < JOY_HAT_END_RIGHT)
			return j->getHat(k - JOY_HAT_0_RIGHT);

		if (k >= JOY_HAT_0_UP && k < JOY_HAT_END_UP)
			return j->getHat(k - JOY_HAT_0_UP);

		if (k >= JOY_HAT_0_DOWN && k < JOY_HAT_END_DOWN)
			return j->getHat(k - JOY_HAT_0_DOWN);
	}

	return false;
}

ActionButtonType joyHatToActionButton(unsigned hatID, JoyHatDirection dir)
{
	unsigned ret = 0;
	if(hatID < MAX_JOYSTICK_HATS)
	{
		if(dir & JOY_HAT_DIR_LEFT)
			ret = JOY_HAT_0_LEFT;
		else if(dir & JOY_HAT_DIR_RIGHT)
			ret = JOY_HAT_0_RIGHT;
		else if(dir & JOY_HAT_DIR_UP)
			ret = JOY_HAT_0_UP;
		else if(dir & JOY_HAT_DIR_DOWN)
			ret = JOY_HAT_0_DOWN;

		if(ret)
			ret += hatID;
	}

	return (ActionButtonType)ret;
}


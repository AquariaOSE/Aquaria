#ifndef VIRTUALMOUSE_H
#define VIRTUALMOUSE_H

#include "ActionMapper.h"

class VirtualMouse : public ActionMapper
{
	VirtualMouse();

	struct Buttons
	{
		bool left, middle, right;
	};

	// externally set to actions that are to be handled as a button press
	int actionToLeft, actionToRight;

	// const references to keep the "API" for the rest of the code
	// but make sure that nothing can be externally modified
	const Buttons &buttons, &pure_buttons;

	Vector position, lastPosition;
	Vector change;

	void update(float dt);

	// override
	virtual void action(int actionID, int state, int playerID, InputDeviceType device);

private:
	Buttons _buttons, _pure_buttons;
};

#endif

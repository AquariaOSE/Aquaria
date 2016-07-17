#ifndef ACTIONSTATUS_H
#define ACTIONSTATUS_H

#include "GameKeys.h"
#include "Joystick.h"

class ActionSet;

const unsigned mouseExtraButtons = 8;

enum ActionButtonType
{
	MOUSE_BUTTON_LEFT	=  KEY_MAXARRAY + 1,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_EXTRA_START,
	MOUSE_BUTTON_EXTRA_END = MOUSE_BUTTON_EXTRA_START + mouseExtraButtons,

	JOY_BUTTON_0		= MOUSE_BUTTON_EXTRA_END,
	JOY_BUTTON_END		= JOY_BUTTON_0 + MAX_JOYSTICK_BTN,

	JOY_AXIS_0_POS		= JOY_BUTTON_END,
	JOY_AXIS_END_POS	= JOY_AXIS_0_POS + MAX_JOYSTICK_AXIS,

	JOY_AXIS_0_NEG		= JOY_AXIS_END_POS,
	JOY_AXIS_END_NEG	= JOY_AXIS_0_NEG + MAX_JOYSTICK_AXIS,

	ACTION_BUTTON_ENUM_SIZE = JOY_AXIS_END_NEG
};


class ActionButtonStatus
{
public:
	ActionButtonStatus();
	void update();
	inline bool getKeyState(int k) const { return !!status[k]; }
	inline bool isKeyChanged(int k) { return !!changed[k]; }
	void import(const ActionSet& as);
	inline const std::vector<int>& getToQuery() const {return toQuery; }
	inline int getJoystickID() const { return joystickID; }
private:
	void _queryAllStatus();
	bool _queryStatus(int k) const;

	int joystickID;
	unsigned char status[ACTION_BUTTON_ENUM_SIZE];
	unsigned char changed[ACTION_BUTTON_ENUM_SIZE];
	std::vector<int> toQuery;
};


#endif

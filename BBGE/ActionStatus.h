#ifndef ACTIONSTATUS_H
#define ACTIONSTATUS_H

#include "GameKeys.h"
#include "Joystick.h"

class ActionSet;

const unsigned mouseExtraButtons = 8;

// *_END is non-inclusive!
enum ActionButtonType
{
	MOUSE_BUTTON_LEFT	=  KEY_MAXARRAY + 1,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_EXTRA_START,
	MOUSE_BUTTON_EXTRA_END = MOUSE_BUTTON_EXTRA_START + mouseExtraButtons,

	// maps to whatever is configured as the primary joystick x/y axes
	JOY_STICK_LEFT = MOUSE_BUTTON_EXTRA_END,
	JOY_STICK_RIGHT,
	JOY_STICK_UP,
	JOY_STICK_DOWN,

	INTERNALLY_USED_ACTION_BUTTONS_END, // Engine needs anything above this for handling inputs properly

	JOY_BUTTON_0		= INTERNALLY_USED_ACTION_BUTTONS_END,
	JOY_BUTTON_END		= JOY_BUTTON_0 + MAX_JOYSTICK_BTN,

	JOY_AXIS_0_POS		= JOY_BUTTON_END,
	JOY_AXIS_END_POS	= JOY_AXIS_0_POS + MAX_JOYSTICK_AXIS,

	JOY_AXIS_0_NEG		= JOY_AXIS_END_POS,
	JOY_AXIS_END_NEG	= JOY_AXIS_0_NEG + MAX_JOYSTICK_AXIS,

	JOY_HAT_BEGIN		= JOY_AXIS_END_NEG,
	JOY_HAT_0_LEFT		= JOY_HAT_BEGIN,
	JOY_HAT_END_LEFT	= JOY_HAT_0_LEFT + MAX_JOYSTICK_HATS,

	JOY_HAT_0_RIGHT		= JOY_HAT_END_LEFT,
	JOY_HAT_END_RIGHT	= JOY_HAT_0_RIGHT + MAX_JOYSTICK_HATS,

	JOY_HAT_0_UP		= JOY_HAT_END_RIGHT,
	JOY_HAT_END_UP		= JOY_HAT_0_UP + MAX_JOYSTICK_HATS,

	JOY_HAT_0_DOWN		= JOY_HAT_END_UP,
	JOY_HAT_END_DOWN	= JOY_HAT_0_DOWN + MAX_JOYSTICK_HATS,
	JOY_HAT_END			= JOY_HAT_END_DOWN,

	ACTION_BUTTON_ENUM_SIZE = JOY_HAT_END
};

ActionButtonType joyHatToActionButton(unsigned hatID, JoyHatDirection dir);

class ActionButtonStatus
{
public:
	ActionButtonStatus();
	void update();
	inline bool getKeyState(int k) const { return !!status[k]; }
	inline bool isKeyChanged(int k) { return !!changed[k]; }
	void import(const ActionSet& as);
	void importQuery(const int *pKeys, size_t num);
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

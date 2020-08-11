#ifndef BBGE_INPUTSYSTEM_H
#define BBGE_INPUTSYSTEM_H

enum InputDeviceType
{
	INP_DEV_NODEVICE,
	INP_DEV_KEYBOARD,
	INP_DEV_MOUSE,
	INP_DEV_JOYSTICK,
};

enum InputControlType
{
	INP_CTRL_BUTTON, // keyboard key or joystick button
	INP_CTRL_AXIS,   // joystick axis
	INP_CTRL_HAT,    // "digital axis"
	INP_CTRL_WHEEL,  // no absolute position, only knows relative changes
	INP_CTRL_POSITION, // only absolute position
};

struct InputSource
{
	InputDeviceType deviceType;
	unsigned deviceID;
	InputControlType ctrlType;
	unsigned ctrlID;
};

union InputValue
{
	float axis; // abs. axis value or wheel delta
	unsigned pressed; // used as bool
	// TODO constrollerfixup: add uchar .hat, SDL uses a bitmask!
	struct { int x, y; } ivec; // -1, 0, +1 as hat values, window coords, etc
};

struct RawInput
{
	InputSource src;
	InputValue u;
};

union SDL_Event;
class IInputMapper;

namespace InputSystem
{
	void handleSDLEvent(const SDL_Event *ev);
	void handleRawInput(const RawInput *inp);

	void addMapper(IInputMapper *mapper);
	void removeMapper(IInputMapper *mapper);
};

#endif

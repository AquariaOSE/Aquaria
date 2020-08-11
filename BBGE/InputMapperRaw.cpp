#include "InputMapperRaw.h"
#include <assert.h>

InputMapperRaw::InputMapperRaw()
{
	memset(&keyState[0], 0, sizeof(keyState));
}

InputMapperRaw::~InputMapperRaw()
{
}

void InputMapperRaw::input(const RawInput *inp)
{
	if(inp->src.ctrlType != INP_CTRL_BUTTON)
		return;

	unsigned idx = inp->src.ctrlID;
	switch(inp->src.deviceType)
	{
		case INP_DEV_MOUSE:
			if(idx > 2)
				return; // only want left, middle, right button
			idx += MOUSE_BUTTON_LEFT;
			// fall through
		case INP_DEV_KEYBOARD:
			break;	// all good
		default: // not interested in joystick etc buttons, those are only used via InputMapper
			return;
	}

	// record change as soon as a change event comes in,
	// so we don't lose button presses that are shorter than 1 frame.
	unsigned char ch = !!inp->u.pressed;
	keyChanged[idx] |= (keyState[idx] != ch) << 1; // bit 1 becomes bit 0 in next frame
	keyState[idx] |= ch;


}

void InputMapperRaw::update()
{
	// make sure that the change bit lives for at least 1 frame
	for(size_t i = 0; i < sizeof(keyChanged); ++i)
		keyChanged[i] >>= 1;
}

bool InputMapperRaw::isKeyPressed(unsigned k) const
{
	assert(k < sizeof(keyState));
	return k < sizeof(keyState) ? !!keyState[k] : false;
}

bool InputMapperRaw::isKeyChanged(unsigned k) const
{
	assert(k < sizeof(keyChanged));
	return k < sizeof(keyChanged) ? !!keyChanged[k] : false;
}



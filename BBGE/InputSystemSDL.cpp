#include "InputSystem.h"
#include <SDL.h>

#if SDL_VERSION_ATLEAST(2,0,0)
#define USESDL2
#endif

void InputSystem::handleSDLEvent(const SDL_Event *ev)
{
	RawInput raw;

	switch(ev->type)
	{
		case SDL_MOUSEMOTION:
		{
			raw.src.deviceType = INP_DEV_MOUSE;
			raw.src.deviceID = ev->motion.which;
			raw.src.ctrlType = INP_CTRL_POSITION;
			raw.src.ctrlID = ev->motion.which;
			raw.u.ivec.x = ev->motion.x;
			raw.u.ivec.y = ev->motion.y;
		}
		break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			raw.src.deviceType = INP_DEV_MOUSE;
			raw.src.deviceID = ev->button.which;

#ifndef USESDL2
			// SDL1 uses buttons 4 and 5 as wheel
			unsigned b = ev->button.button;
			if(b == 4 || b == 5)
			{
				raw.src.ctrlType = INP_CTRL_WHEEL;
				raw.src.ctrlID = 0;
				int dir = b == 4 ? 1 : -1;
				raw.u.axis = dir;
				break;
			}
#endif
			raw.src.ctrlType = INP_CTRL_BUTTON;
			raw.src.ctrlID = ev->button.which;
			raw.u.pressed = ev->button.state;
		}
		break;

#ifdef USESDL2
		case SDL_MOUSEWHEEL:
		{
			raw.src.deviceType = INP_DEV_MOUSE;
			raw.src.deviceID = ev->wheel.which;
			raw.src.ctrlType = INP_CTRL_WHEEL;
			raw.src.ctrlID = ev->wheel.which;
			raw.u.axis = ev->button.state;
		}
		break;
#endif

		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
#ifdef USESDL2
			unsigned kidx = ev->key.keysym.scancode;
#else
			unsigned kidx= event.key.keysym.sym;
#endif
			raw.src.deviceType = INP_DEV_KEYBOARD;
			raw.src.deviceID = 0; // SDL doesn't seem to support multiple keyboards
			raw.src.ctrlType = INP_CTRL_BUTTON;
			raw.src.ctrlID = kidx;
			raw.u.pressed = ev->key.state;
		}
		break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		{
			raw.src.deviceType = INP_DEV_KEYBOARD;
			raw.src.deviceID = ev->jbutton.which;
			raw.src.ctrlType = INP_CTRL_BUTTON;
			raw.src.ctrlID = ev->jbutton.button;
			raw.u.pressed = ev->jbutton.state;
		}

		case SDL_JOYAXISMOTION:
		{
			raw.src.deviceType = INP_DEV_JOYSTICK;
			raw.src.deviceID = ev->jaxis.which;
			raw.src.ctrlType = INP_CTRL_AXIS;
			raw.src.ctrlID = ev->jaxis.axis;
			raw.u.axis = ev->jaxis.value / 32768.0f;
		}
		break;

		case SDL_JOYHATMOTION:
		{
			raw.src.deviceType = INP_DEV_JOYSTICK;
			raw.src.deviceID = ev->jhat.which;
			raw.src.ctrlType = INP_CTRL_HAT;
			raw.src.ctrlID = ev->jhat.hat;
			int x, y;
			switch(ev->jhat.value)
			{
				case SDL_HAT_CENTERED:  x = 0; y = 0; break;
				case SDL_HAT_UP:        x = 0; y = -1; break;
				case SDL_HAT_DOWN:      x = 0; y = +1; break;
				case SDL_HAT_LEFT:      x = -1; y = 0; break;
				case SDL_HAT_RIGHT:     x = +1; y = 0; break;
				case SDL_HAT_LEFTUP:    x = -1; y = -1; break;
				case SDL_HAT_LEFTDOWN:  x = -1; y = +1; break;
				case SDL_HAT_RIGHTUP:   x = +1; y = -1; break;
				case SDL_HAT_RIGHTDOWN: x = +1; y = +1; break;
			}
			raw.u.ivec.x = (signed short)x;
			raw.u.ivec.y = (signed short)y;
		}
		break;

		case SDL_CONTROLLERAXISMOTION:
		{
			raw.src.deviceType = INP_DEV_JOYSTICK;
			raw.src.deviceID = ev->caxis.which;
			raw.src.ctrlType = INP_CTRL_AXIS;
			raw.src.ctrlID = ev->caxis.axis;
			raw.u.axis = ev->caxis.value / float(0x7fff);
		}
		break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
		{
			raw.src.deviceType = INP_DEV_JOYSTICK;
			raw.src.deviceID = ev->cbutton.which;
			raw.src.ctrlType = INP_CTRL_BUTTON;
			raw.src.ctrlID = ev->cbutton.button;
			raw.u.pressed = ev->cbutton.state;
		}
		break;

		/*case SDL_WINDOWEVENT:
		switch(ev->window.event)
		{
			case SDL_WINDOWEVENT_RESIZED:
				s_winSizeXInv = 1.0f / (float)ev->window.data1;
				s_winSizeYInv = 1.0f / (float)ev->window.data2;
			break;
		}*/
		// fall through
		default:
			return;
	}

	handleRawInput(&raw);
}

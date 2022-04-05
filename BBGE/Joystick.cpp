/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Joystick.h"
#include "Core.h"
#include "SDL.h"

unsigned Joystick::GetNumJoysticks()
{
	return SDL_NumJoysticks();
}

Joystick::Joystick()
{
	stickIndex = -1;
#if SDL_VERSION_ATLEAST(2,0,0)
	sdl_controller = NULL;
	sdl_haptic = NULL;
	if(!SDL_WasInit(SDL_INIT_HAPTIC))
		if(SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0)
			debugLog("Failed to init haptic subsystem");
#endif
	sdl_joy = NULL;
	buttonBitmask = 0;
	deadZone1 = 0.3f;
	deadZone2 = 0.3f;

	clearRumbleTime= 0;
	numJoyAxes = 0;
	numHats = 0;
	numButtons = 0;
	clearAxes();

	s1ax = 0;
	s1ay = 1;
	s2ax = 4;
	s2ay = 3;

	enabled = false;
}

Joystick::~Joystick()
{
	shutdown();

#if SDL_VERSION_ATLEAST(2,0,0)
	if(SDL_WasInit(SDL_INIT_HAPTIC))
		SDL_QuitSubSystem(SDL_INIT_HAPTIC);
#endif
}

const char *Joystick::getName() const
{
	return name.c_str();
}

const char *Joystick::getGUID() const
{
	return guid.c_str();
}

bool Joystick::init(int stick)
{
	stickIndex = stick;
	numJoyAxes = 0;

	#if SDL_VERSION_ATLEAST(2,0,0)
	if (SDL_IsGameController(stick))
	{
		sdl_controller = SDL_GameControllerOpen(stick);
		if (sdl_controller)
			sdl_joy = SDL_GameControllerGetJoystick(sdl_controller);
	}
	if (!sdl_joy)
		sdl_joy = SDL_JoystickOpen(stick);
	if (sdl_joy && SDL_JoystickIsHaptic(sdl_joy))
	{
		sdl_haptic = SDL_HapticOpenFromJoystick(sdl_joy);
		bool rumbleok = false;
		if (sdl_haptic && SDL_HapticRumbleSupported(sdl_haptic))
			rumbleok = (SDL_HapticRumbleInit(sdl_haptic) == 0);
		if (!rumbleok)
		{
			SDL_HapticClose(sdl_haptic);
			sdl_haptic = NULL;
		}
	}
	#endif

	if (!sdl_joy)
		sdl_joy = SDL_JoystickOpen(stick);

	if (sdl_joy)
	{
		#if SDL_VERSION_ATLEAST(2,0,0)
		const char *n = SDL_JoystickName(sdl_joy);
		name = n ? n : "<?>";
		SDL_JoystickGUID jg = SDL_JoystickGetGUID(sdl_joy);
		char guidbuf[40];
		SDL_JoystickGetGUIDString(jg, &guidbuf[0], sizeof(guidbuf));
		guid = &guidbuf[0];
		debugLog(std::string("Initialized Joystick [") + name + "], GUID [" + guid + "]");
		if (sdl_controller)
		{
			debugLog("Joystick is a Game Controller");
			numJoyAxes = SDL_CONTROLLER_AXIS_MAX;
		}
		if (sdl_haptic)
			debugLog("Joystick has force feedback support");
		instanceID = SDL_JoystickInstanceID(sdl_joy);
		#else
		const char *n = SDL_JoystickName(stick);
		name = n ? n : "<?>";
		debugLog(std::string("Initialized Joystick [") + name + "]");
		instanceID = SDL_JoystickIndex(sdl_joy);
		#endif

		if(!numJoyAxes)
			numJoyAxes = SDL_JoystickNumAxes(sdl_joy);
		if(numJoyAxes > MAX_JOYSTICK_AXIS)
			numJoyAxes = MAX_JOYSTICK_AXIS;

		numHats = SDL_JoystickNumHats(sdl_joy);
		numButtons = SDL_JoystickNumButtons(sdl_joy);
		

		return true;
	}
	
	std::ostringstream os;
	os << "Failed to init Joystick [" << stick << "]";
	debugLog(os.str());
	return false;
}

void Joystick::shutdown()
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if (sdl_haptic)
	{
		SDL_HapticClose(sdl_haptic);
		sdl_haptic = 0;
	}
	if (sdl_controller)
	{
		SDL_GameControllerClose(sdl_controller);
		sdl_controller = 0;
		sdl_joy = 0; // SDL_GameControllerClose() frees this
	}
#endif
	if (sdl_joy)
	{
		SDL_JoystickClose(sdl_joy);
		sdl_joy = 0;
	}

	numJoyAxes = 0;
	clearAxes();
}

void Joystick::clearAxes()
{
	memset(axisRaw, 0, sizeof(axisRaw));
}

void Joystick::rumble(float leftMotor, float rightMotor, float time)
{
	if (core->joystickEnabled)
	{
#if SDL_VERSION_ATLEAST(2,0,0)
		if (sdl_haptic)
		{
			const float power = (leftMotor + rightMotor) / 2.0f;
			if ((power > 0.0f) && (time > 0.0f))
			{
				clearRumbleTime = time;
				SDL_HapticRumblePlay(sdl_haptic, power, (Uint32) (time * 1000.0f));
			}
			else
			{
				clearRumbleTime = -1;
				SDL_HapticRumbleStop(sdl_haptic);
			}
		}
#endif
	}
}

void Joystick::calibrate(Vector &calvec, float deadZone)
{
	if (calvec.isLength2DIn(deadZone))
	{
		calvec = Vector(0,0,0);
	}
	else
	{
		if (!calvec.isZero())
		{
			Vector pos2 = calvec;
			pos2.setLength2D(deadZone);
			calvec -= pos2;
			float mult = 1.0f/float(1.0f-deadZone);
			calvec.x *= mult;
			calvec.y *= mult;
			if (calvec.x > 1)
				calvec.x = 1;
			else if (calvec.x < -1)
				calvec.x = -1;

			if (calvec.y > 1)
				calvec.y = 1;
			else if (calvec.y < -1)
				calvec.y = -1;
		}
	}
}

void Joystick::update(float dt)
{
	if (core->joystickEnabled && sdl_joy && stickIndex != -1)
	{
#if SDL_VERSION_ATLEAST(2,0,0)
		if (!SDL_JoystickGetAttached(sdl_joy))
		{
			debugLog("Lost Joystick");
			shutdown();
			return;
		}
		if (sdl_controller)
		{
			for(unsigned i = 0; i < numJoyAxes; ++i)
			{
				Sint16 ax = SDL_GameControllerGetAxis(sdl_controller, (SDL_GameControllerAxis)i);
				axisRaw[i] = float(ax)/32768.0f;
			}

			position.x = axisRaw[SDL_CONTROLLER_AXIS_LEFTX];
			position.y = axisRaw[SDL_CONTROLLER_AXIS_LEFTY];
			rightStick.x = axisRaw[SDL_CONTROLLER_AXIS_RIGHTX];
			rightStick.y = axisRaw[SDL_CONTROLLER_AXIS_RIGHTY];
		}
		else
#else
		if (!SDL_JoystickOpened(stickIndex))
		{
			debugLog("Lost Joystick");
			sdl_joy = NULL;
			return;
		}
#endif
		// Note: this connects with the else above when the SDL2 path is compiled!
		{
			for(unsigned i = 0; i < numJoyAxes; ++i)
			{
				Sint16 ax = SDL_JoystickGetAxis(sdl_joy, i);
				axisRaw[i] = float(ax)/32768.0f;
			}
			position.x = s1ax >= 0 ? axisRaw[s1ax] : 0.0f;
			position.y = s1ay >= 0 ? axisRaw[s1ay] : 0.0f;
			rightStick.x = s2ax >= 0 ? axisRaw[s2ax] : 0.0f;
			rightStick.y = s2ay >= 0 ? axisRaw[s2ay] : 0.0f;
		}

		calibrate(position, deadZone1);
		calibrate(rightStick, deadZone2);

		buttonBitmask = 0;

#if SDL_VERSION_ATLEAST(2,0,0)
		if (sdl_controller)
		{
			for (unsigned i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
				buttonBitmask |= !!SDL_GameControllerGetButton(sdl_controller, (SDL_GameControllerButton)i) << i;
		}
		else
#endif
		{
			for (unsigned i = 0; i < numButtons; i++)
				buttonBitmask |= !!SDL_JoystickGetButton(sdl_joy, i) << i;

			//for (unsigned i = 0; i < numHats; i++)
		}
	}

	if (clearRumbleTime >= 0)
	{
		clearRumbleTime -= dt;
		if (clearRumbleTime <= 0)
		{
			rumble(0,0,0);
		}
	}
}

bool Joystick::anyButton() const
{
	return !!buttonBitmask;;
}

unsigned Joystick::getNumAxes() const
{
#if SDL_VERSION_ATLEAST(2,0,0)
	return sdl_controller ? SDL_CONTROLLER_AXIS_MAX : numJoyAxes;
#else
	return numJoyAxes;
#endif
}

unsigned Joystick::getNumButtons() const
{
	return numButtons;
}

unsigned Joystick::getNumHats() const
{
	return numHats;
}

float Joystick::getAxisUncalibrated(unsigned id) const
{
	return id < MAX_JOYSTICK_AXIS ? axisRaw[id] : 0.0f;
}

JoyHatDirection Joystick::getHat(unsigned id) const
{
	unsigned dir = SDL_JoystickGetHat(sdl_joy, id);
	unsigned ret = JOY_HAT_DIR_CENTERED;
	switch(dir)
	{
		case SDL_HAT_UP: ret = JOY_HAT_DIR_UP; break;
		case SDL_HAT_DOWN: ret = JOY_HAT_DIR_DOWN; break;
		case SDL_HAT_LEFT: ret = JOY_HAT_DIR_LEFT; break;
		case SDL_HAT_RIGHT: ret = JOY_HAT_DIR_RIGHT; break;

		case SDL_HAT_LEFTUP: ret = JOY_HAT_DIR_UP | JOY_HAT_DIR_LEFT; break;
		case SDL_HAT_RIGHTUP: ret = JOY_HAT_DIR_UP | JOY_HAT_DIR_RIGHT; break;
		case SDL_HAT_LEFTDOWN: ret = JOY_HAT_DIR_DOWN | JOY_HAT_DIR_LEFT; break;
		case SDL_HAT_RIGHTDOWN: ret = JOY_HAT_DIR_DOWN | JOY_HAT_DIR_RIGHT; break;

		default: ;
	}
	return (JoyHatDirection)ret;
}

const char *Joystick::getAxisName(unsigned axis) const
{
	if(axis >= numJoyAxes)
		return NULL;
#if SDL_VERSION_ATLEAST(2,0,0)
	if(sdl_controller)
		return SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)axis);
#endif
	return NULL;
}

const char *Joystick::getButtonName(unsigned btn) const
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if(sdl_controller)
		return SDL_GameControllerGetStringForButton((SDL_GameControllerButton)btn);
#endif
	return NULL;
}

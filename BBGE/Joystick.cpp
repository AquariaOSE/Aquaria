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
#  ifdef BBGE_BUILD_SDL2
	sdl_controller = NULL;
	sdl_haptic = NULL;
	if(!SDL_WasInit(SDL_INIT_HAPTIC))
		if(SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0)
			debugLog("Failed to init haptic subsystem");
#  endif
	sdl_joy = NULL;
	clearRumbleTime= 0;
	enabled = false;
}

Joystick::~Joystick()
{
	shutdown();

#ifdef BBGE_BUILD_SDL2
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

	#ifdef BBGE_BUILD_SDL2
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
		#ifdef BBGE_BUILD_SDL2
		const char *n = SDL_JoystickName(sdl_joy);
		name = n ? n : "<?>";
		SDL_JoystickGUID jg = SDL_JoystickGetGUID(sdl_joy);
		char guidbuf[40];
		SDL_JoystickGetGUIDString(jg, &guidbuf[0], sizeof(guidbuf));
		guid = &guidbuf[0];
		debugLog(std::string("Initialized Joystick [") + name + "], GUID [" + guid + "]");
		if (sdl_controller)
			debugLog("Joystick is a Game Controller");
		if (sdl_haptic)
			debugLog("Joystick has force feedback support");
		instanceID = SDL_JoystickInstanceID(sdl_joy);
		#else
		const char *n = SDL_JoystickName(stick);
		name = n ? n : "<?>";
		debugLog(std::string("Initialized Joystick [") + name + "]");
		instanceID = SDL_JoystickIndex(sdl_joy);
		#endif

		std::ostringstream os;
		os << "Joystick has " << SDL_JoystickNumAxes(sdl_joy) << " axes, "
			<< SDL_JoystickNumButtons(sdl_joy) << " buttons, "
			<< SDL_JoystickNumHats(sdl_joy) << " hats.";
		debugLog(os.str());

		return true;
	}
	
	std::ostringstream os;
	os << "Failed to init Joystick [" << stick << "]";
	debugLog(os.str());
	return false;
}

void Joystick::shutdown()
{
#ifdef BBGE_BUILD_SDL2
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
}

void Joystick::rumble(float leftMotor, float rightMotor, float time)
{
	if (core->joystickEnabled)
	{
#ifdef BBGE_BUILD_SDL2
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

void Joystick::Calibrate(Vector &calvec, float deadZone)
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
#ifdef BBGE_BUILD_SDL2
		if (!SDL_JoystickGetAttached(sdl_joy))
		{
			debugLog("Lost Joystick");
			shutdown();
			return;
		}
#else
		if (!SDL_JoystickOpened(stickIndex))
		{
			debugLog("Lost Joystick");
			sdl_joy = NULL;
			return;
		}
#endif
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

const char *Joystick::getAxisName(unsigned axis) const
{
#ifdef BBGE_BUILD_SDL2
	if(sdl_controller)
		return SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)axis);
#endif
	return NULL;
}

const char *Joystick::getButtonName(unsigned btn) const
{
#ifdef BBGE_BUILD_SDL2
	if(sdl_controller)
		return SDL_GameControllerGetStringForButton((SDL_GameControllerButton)btn);
#endif
	return NULL;
}

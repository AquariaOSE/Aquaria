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

#ifndef BBGE_BUILD_SDL2
#error test this
#endif

#include "Joystick.h"
#include "Core.h"

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
#  endif
	sdl_joy = NULL;
	buttonBitmask = 0;
	deadZone1 = 0.3;
	deadZone2 = 0.3;

	clearRumbleTime= 0;

	s1ax = 0;
	s1ay = 1;
	s2ax = 4;
	s2ay = 3;

	enabled = true;
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
		debugLog(std::string("Initialized Joystick [") + SDL_JoystickName(sdl_joy) + "]");
		if (sdl_controller)
			debugLog("Joystick is a Game Controller");
		if (sdl_haptic)
			debugLog("Joystick has force feedback support");
		instanceID = SDL_JoystickInstanceID(sdl_joy);
		#else
		debugLog(std::string("Initialized Joystick [") + SDL_JoystickName(stick)) + std::string("]"));
		instanceID = SDL_JoystickIndex(sdl_joy);
		#endif

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
#ifdef BBGE_BUILD_SDL2
		if (!SDL_JoystickGetAttached(sdl_joy))
		{
			debugLog("Lost Joystick");
			shutdown();
			return;
		}

		if (sdl_controller)
		{
			Sint16 xaxis = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_LEFTX);
			Sint16 yaxis = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_LEFTY);
			position.x = float(xaxis)/32768.0f;
			position.y = float(yaxis)/32768.0f;

			Sint16 xaxis2 = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_RIGHTX);
			Sint16 yaxis2 = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_RIGHTY);
			rightStick.x = float(xaxis2)/32768.0f;
			rightStick.y = float(yaxis2)/32768.0f;
		}
		else
		{
			Sint16 xaxis = SDL_JoystickGetAxis(sdl_joy, s1ax);
			Sint16 yaxis = SDL_JoystickGetAxis(sdl_joy, s1ay);
			position.x = float(xaxis)/32768.0f;
			position.y = float(yaxis)/32768.0f;

			Sint16 xaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ax);
			Sint16 yaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ay);
			rightStick.x = float(xaxis2)/32768.0f;
			rightStick.y = float(yaxis2)/32768.0f;
		}
#else
		if (!SDL_JoystickOpened(stickIndex))
		{
			debugLog("Lost Joystick");
			sdl_joy = NULL;
			return;
		}

		Sint16 xaxis = SDL_JoystickGetAxis(sdl_joy, s1ax);
		Sint16 yaxis = SDL_JoystickGetAxis(sdl_joy, s1ay);
		position.x = xaxis/32768.0f;
		position.y = yaxis/32768.0f;

		Sint16 xaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ax);
		Sint16 yaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ay);
		rightStick.x = xaxis2/32768.0f;
		rightStick.y = yaxis2/32768.0f;
#endif

		calibrate(position, deadZone1);
		calibrate(rightStick, deadZone2);

		buttonBitmask = 0;

#ifdef BBGE_BUILD_SDL2
		if (sdl_controller)
		{
			for (unsigned i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
				buttonBitmask |= !!SDL_GameControllerGetButton(sdl_controller, (SDL_GameControllerButton)i) << i;
		}
		else
#endif
		{
			for (unsigned i = 0; i < MAX_JOYSTICK_BTN; i++)
				buttonBitmask |= !!SDL_JoystickGetButton(sdl_joy, i) << i;
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

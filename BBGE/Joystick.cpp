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

#include "Core.h"

#if defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_XINPUT) 
	#include "Xinput.h"

#if defined(BBGE_BUILD_DELAYXINPUT)
	#include <DelayImp.h>
#endif

/*
	HRESULT (WINAPI *XInputGetState)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) = 0;

if ( (winp.hInstDI = LoadLibrary( "dinput.dll" )) == 0 )


if (!pDirectInput8Create) {
	pDirectInput8Create = (HRESULT (__stdcall *)(HINSTANCE, DWORD ,REFIID, LPVOID *, LPUNKNOWN)) GetProcAddress(winp.hInstDI,"DirectInput8Create");

	if (!pDirectInput8Create) {
		error(L"Couldn't get DI proc addr\n");
	}
} 

	bool importXInput()
	{

	}
*/




bool tryXInput()
{
	__try
	{
		XINPUT_STATE xinp;
		XInputGetState(0, &xinp);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return true;
}

#endif

#ifdef __LINUX__
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>
#include <iostream>

#define BITS_TO_LONGS(x) \
	(((x) + 8 * sizeof (unsigned long) - 1) / (8 * sizeof (unsigned long)))
#define AQUARIA_BITS_PER_LONG (sizeof(long) * 8)
#define AQUARIA_OFF(x)  ((x)%AQUARIA_BITS_PER_LONG)
#define AQUARIA_BIT(x)  (1UL<<AQUARIA_OFF(x))
#define AQUARIA_LONG(x) ((x)/AQUARIA_BITS_PER_LONG)
#define test_bit(bit, array) ((array[AQUARIA_LONG(bit)] >> AQUARIA_OFF(bit)) & 1)
#endif

Joystick::Joystick()
{
	xinited = false;
	stickIndex = -1;
#ifdef BBGE_BUILD_SDL
#  ifdef BBGE_BUILD_SDL2
	sdl_controller = NULL;
	sdl_haptic = NULL;
#  endif
	sdl_joy = NULL;
#endif
#if defined(__LINUX__) && !defined(BBGE_BUILD_SDL2)
	eventfd = -1;
	effectid = -1;
#endif
	inited = false;
	for (int i = 0; i < maxJoyBtns; i++)
	{
		buttons[i] = UP;
	}
	deadZone1 = 0.3;
	deadZone2 = 0.3;

	clearRumbleTime= 0;
	leftThumb = rightThumb = false;
	leftTrigger = rightTrigger = 0;
	rightShoulder = leftShoulder = false;
	dpadRight = dpadLeft = dpadUp = dpadDown = false;
	btnStart = false;
	btnSelect = false;

	s1ax = 0;
	s1ay = 1;
	s2ax = 4;
	s2ay = 3;
}

void Joystick::init(int stick)
{
#if defined(BBGE_BUILD_SDL) || defined(__LINUX__)
	std::ostringstream os;
#endif

#ifdef BBGE_BUILD_SDL
	stickIndex = stick;
	const int numJoy = SDL_NumJoysticks();
	os << "Found [" << numJoy << "] joysticks";
	debugLog(os.str());

	if (numJoy > stick)
	{
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
			inited = true;
			#ifdef BBGE_BUILD_SDL2
			debugLog(std::string("Initialized Joystick [") + std::string(SDL_JoystickName(sdl_joy)) + std::string("]"));
			if (sdl_controller) debugLog(std::string("Joystick is a Game Controller"));
			if (sdl_haptic) debugLog(std::string("Joystick has force feedback support"));
			#else
			debugLog(std::string("Initialized Joystick [") + std::string(SDL_JoystickName(stick)) + std::string("]"));
			#endif
		}
		else
		{
			std::ostringstream os;
			os << "Failed to init Joystick [" << stick << "]";
			debugLog(os.str());
		}
	}
	else
	{
		debugLog("Not enough Joystick(s) found");
	}
#endif
	
#if defined(__LINUX__) && !defined(BBGE_BUILD_SDL2)
	os.seekp(0);
	os << "AQUARIA_EVENT_JOYSTICK" << stick;

	std::string envkey = os.str();
	const char* evdevice = getenv(envkey.c_str());

	if (evdevice != NULL) {
		eventfd = open(evdevice, O_RDWR, 0);
		if (eventfd < 0) {
			debugLog(std::string("Could not open rumble device [") + evdevice + "]: " + strerror(errno));
		}
		else {
			debugLog(std::string("Successfully opened rumble device [") + evdevice + "]");
			unsigned long features[BITS_TO_LONGS(FF_CNT)];

			if (ioctl(eventfd, EVIOCGBIT(EV_FF, sizeof(features)), features) == -1) {
				debugLog(std::string("Cannot query joystick/gamepad features: ") + strerror(errno));
				close(eventfd);
				eventfd = -1;
			}
			else if (!test_bit(FF_RUMBLE, features)) {
				debugLog("Rumble is not supported by your gamepad/joystick.");
				close(eventfd);
				eventfd = -1;
			}
		}
	}
	else {
		std::cout <<
			"Environment varialbe " << envkey << " is not set.\n"
			"Set this environment variable to the device file that shall be used for joystick number " << stick << " in order to enable rumble support.\n"
			"Example:\n"
			"\texport " << envkey << "=/dev/input/event6\n\n";
	}
#endif

#ifdef BBGE_BUILD_XINPUT
	debugLog("about to init xinput");

	xinited = tryXInput();

	if (!xinited)
		debugLog("XInput not found, not installed?");

	debugLog("after catch");

#if !defined(BBGE_BUILD_SDL)
	inited = xinited;
#endif
#endif
}

void Joystick::shutdown()
{
#if defined(__LINUX__) && !defined(BBGE_BUILD_SDL2)
	if (eventfd >= 0) {
		if (effectid != -1 && ioctl(eventfd, EVIOCRMFF, effectid) == -1) {
			debugLog(std::string("Remove rumble effect: ") + strerror(errno));
		}
		close(eventfd);
		eventfd = -1;
	}
#endif
#ifdef BBGE_BUILD_SDL
	if (sdl_joy)
	{
		SDL_JoystickClose(sdl_joy);
		sdl_joy = 0;
	}
#endif
}

void Joystick::rumble(float leftMotor, float rightMotor, float time)
{
	if (core->joystickEnabled && inited)
	{
#ifdef BBGE_BUILD_SDL2
		if (sdl_haptic)
		{
			const float power = (leftMotor + rightMotor) / 2.0f;
			if ((power > 0.0f) && (time > 0.0f))
				SDL_HapticRumblePlay(sdl_haptic, power, (Uint32) (time * 1000.0f));
			else
				SDL_HapticRumbleStop(sdl_haptic);
		}

#elif defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_XINPUT)
		XINPUT_VIBRATION vib;
		vib.wLeftMotorSpeed = WORD(leftMotor*65535);
		vib.wRightMotorSpeed = WORD(rightMotor*65535);
		
		clearRumbleTime = time;
		DWORD d = XInputSetState(0, &vib);
		if (d == ERROR_SUCCESS)
		{
			//debugLog("success");
		}
		else if (d == ERROR_DEVICE_NOT_CONNECTED)
		{
			//debugLog("joystick not connected");
		}
		else
		{
			//unknown error
		}
#elif defined(__LINUX__)
		if (eventfd >= 0) {
			struct ff_effect effect;
			struct input_event event;

			effect.type = FF_RUMBLE;
			effect.id = effectid;
			effect.direction = 0;
			effect.trigger.button = 0;
			effect.trigger.interval = 0;
			effect.replay.length = (uint16_t) (time * 1000);
			effect.replay.delay = 0;
			if (leftMotor > rightMotor) {
				effect.u.rumble.strong_magnitude = (uint16_t) (leftMotor * 0xffff);
				effect.u.rumble.weak_magnitude = (uint16_t) (rightMotor * 0xffff);
			}
			else {
				effect.u.rumble.strong_magnitude = (uint16_t) (rightMotor * 0xffff);
				effect.u.rumble.weak_magnitude = (uint16_t) (leftMotor * 0xffff);
			}
	
			if (ioctl(eventfd, EVIOCSFF, &effect) == -1) {
				debugLog(std::string("Upload rumble effect: ") + strerror(errno));
				return;
			}
	
			event.time.tv_sec = 0;
			event.time.tv_usec = 0;
			event.type = EV_FF;
			event.code = effectid = effect.id;
	
			if (leftMotor == 0 && rightMotor == 0) {
				event.value = 0;
			}
			else {
				event.value = 1;
			}
	
			if (write(eventfd, (const void*) &event, sizeof(event)) == -1) {
				debugLog(std::string("Play rumble effect: ") + strerror(errno));
			}
		}
#endif
	}
}

void Joystick::callibrate(Vector &calvec, float deadZone)
{
	//float len = position.getLength2D();
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
#ifdef BBGE_BUILD_SDL
	if (core->joystickEnabled && inited && sdl_joy && stickIndex != -1)
	{
#ifdef BBGE_BUILD_SDL2
		if (!SDL_JoystickGetAttached(sdl_joy))
		{
			debugLog("Lost Joystick");
			if (sdl_haptic) { SDL_HapticClose(sdl_haptic); sdl_haptic = NULL; }
			if (!sdl_controller)
				SDL_JoystickClose(sdl_joy);
			else
			{
				SDL_GameControllerClose(sdl_controller);
				sdl_controller = NULL;
			}
			sdl_joy = NULL;
			return;
		}

		if (sdl_controller)
		{
			Sint16 xaxis = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_LEFTX);
			Sint16 yaxis = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_LEFTY);
			position.x = double(xaxis)/32768.0;
			position.y = double(yaxis)/32768.0;

			Sint16 xaxis2 = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_RIGHTX);
			Sint16 yaxis2 = SDL_GameControllerGetAxis(sdl_controller, SDL_CONTROLLER_AXIS_RIGHTY);
			rightStick.x = double(xaxis2)/32768.0;
			rightStick.y = double(yaxis2)/32768.0;
		}
		else
		{
			Sint16 xaxis = SDL_JoystickGetAxis(sdl_joy, s1ax);
			Sint16 yaxis = SDL_JoystickGetAxis(sdl_joy, s1ay);
			position.x = double(xaxis)/32768.0;
			position.y = double(yaxis)/32768.0;

			Sint16 xaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ax);
			Sint16 yaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ay);
			rightStick.x = double(xaxis2)/32768.0;
			rightStick.y = double(yaxis2)/32768.0;
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

		/*
		std::ostringstream os;
		os << "joy(" << position.x << ", " << position.y << ")";
		debugLog(os.str());
		*/


		callibrate(position, deadZone1);

		callibrate(rightStick, deadZone2);
		

		/*
		std::ostringstream os2;
		os2 << "joy2(" << position.x << ", " << position.y << ")";
		debugLog(os2.str());
		*/
#ifdef BBGE_BUILD_SDL2
		if (sdl_controller)
		{
			for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
				buttons[i] = SDL_GameControllerGetButton(sdl_controller, (SDL_GameControllerButton)i)?DOWN:UP;
			for (int i = SDL_CONTROLLER_BUTTON_MAX; i < maxJoyBtns; i++)
				buttons[i] = UP;
		}
		else
		{
			for (int i = 0; i < maxJoyBtns; i++)
				buttons[i] = SDL_JoystickGetButton(sdl_joy, i)?DOWN:UP;
		}
#else
		for (int i = 0; i < maxJoyBtns; i++)
			buttons[i] = SDL_JoystickGetButton(sdl_joy, i)?DOWN:UP;
#endif
		/*
		unsigned char btns[maxJoyBtns];
		glfwGetJoystickButtons(GLFW_JOYSTICK_1, btns, maxJoyBtns);
		for (int i = 0; i < maxJoyBtns; i++)
		{
			if (btns[i] == GLFW_PRESS)
				buttons[i] = DOWN;
			else
				buttons[i] = UP;
		}
		*/


	}
#endif

	if (clearRumbleTime > 0)
	{
		clearRumbleTime -= dt;
		if (clearRumbleTime < 0)
		{
			rumble(0,0,0);
		}
	}

#if defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_XINPUT)
	if (inited && xinited)
	{
		XINPUT_STATE xinp;
		XInputGetState(0, &xinp);
		
		leftTrigger = float(xinp.Gamepad.bLeftTrigger)/255.0f;
		rightTrigger = float(xinp.Gamepad.bRightTrigger)/255.0f;

		leftShoulder = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
		rightShoulder = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

		leftThumb = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
		rightThumb = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
		
		dpadUp = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
		dpadDown = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
		dpadLeft = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
		dpadRight = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

		
		

#if !defined(BBGE_BUILD_SDL)

		buttons[0] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_A?DOWN:UP;
		buttons[1] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_B?DOWN:UP;
		buttons[2] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_X?DOWN:UP;
		buttons[3] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_Y?DOWN:UP;

		position = Vector(xinp.Gamepad.sThumbLX, xinp.Gamepad.sThumbLY)/32768.0f;
		position.y = -rightStick.y;

		rightStick = Vector(xinp.Gamepad.sThumbRX, xinp.Gamepad.sThumbRY)/32768.0f;
		rightStick.y = -rightStick.y;

		callibrate(position, deadZone1);

		callibrate(rightStick, deadZone2);

#endif

		btnStart = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_START;
		btnSelect = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
	}
#endif
		
		
		/*
		std::ostringstream os;
		os << "j-pos(" << position.x << ", " << position.y << " - b0[" << buttons[0] << "]) - len[" << len << "]";
		debugLog(os.str());
		*/
}

bool Joystick::anyButton()
{
	for (int i = 0; i < maxJoyBtns; i++)
	{
		if (buttons[i]) return true;
	}
	return false;
}

#ifndef BBGE_JOYSTICK_H
#define BBGE_JOYSTICK_H

#include <SDL_joystick.h>

#include "Vector.h"

#define MAX_JOYSTICK_BTN 32

class Joystick
{
public:
	Joystick();
	void init(int stick=0);
	void shutdown();
	//Ranges from 0 to 65535 (full speed).
	void rumble(float leftMotor, float rightMotor, float time);
	void update(float dt);
	Vector position, lastPosition;

	float deadZone1, deadZone2;
	float clearRumbleTime;

	void callibrate(Vector &vec, float dead);
	bool anyButton();
	bool getButton(unsigned id) const { return buttonBitmask & (1u << id); }

	Vector rightStick;

	int stickIndex;
	int s1ax, s1ay, s2ax, s2ay;

private:
	bool inited;
	unsigned buttonBitmask; // FIXME: this should go

#  ifdef BBGE_BUILD_SDL2
	SDL_GameController *sdl_controller;
	SDL_Haptic *sdl_haptic;
#  endif
	SDL_Joystick *sdl_joy;

#if defined(__LINUX__) && !defined(BBGE_BUILD_SDL2)
	int eventfd;
	short effectid;
#endif
};


#endif

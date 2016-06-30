#ifndef BBGE_JOYSTICK_H
#define BBGE_JOYSTICK_H

#include <SDL_joystick.h>

#ifdef BBGE_BUILD_SDL2
#include <SDL_gamecontroller.h>
#include <SDL_haptic.h>
#endif

#include "Vector.h"

#define MAX_JOYSTICK_BTN 32
#define MAX_JOYSTICK_AXIS 32

class Joystick
{
public:
	Joystick();
	void init(int stick=0);
	void shutdown();
	//Ranges from 0 to 1 (full speed).
	void rumble(float leftMotor, float rightMotor, float time);
	void update(float dt);
	Vector position;

	float deadZone1, deadZone2;
	float clearRumbleTime;

	void calibrate(Vector &vec, float dead);
	bool anyButton() const;
	bool getButton(unsigned id) const { return !!(buttonBitmask & (1u << id)); }
	int getIndex() const { return stickIndex; }
	int getInstanceID() const { return instanceID; }

	Vector rightStick;

	int s1ax, s1ay, s2ax, s2ay;

private:
	int stickIndex;
	int instanceID;
	unsigned buttonBitmask; // FIXME: this should go
	SDL_Joystick *sdl_joy;

#  ifdef BBGE_BUILD_SDL2
	SDL_GameController *sdl_controller;
	SDL_Haptic *sdl_haptic;
#  endif
};


#endif

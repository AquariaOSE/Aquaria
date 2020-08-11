#ifndef BBGE_JOYSTICK_H
#define BBGE_JOYSTICK_H

#include <string>
#include <SDL_joystick.h>

#ifdef BBGE_BUILD_SDL2
#include <SDL_gamecontroller.h>
#include <SDL_haptic.h>
#endif

#include "Vector.h"

const static float JOY_AXIS_THRESHOLD = 0.6f;

class Joystick
{
public:
	static unsigned GetNumJoysticks();

	Joystick();
	~Joystick();

	bool init(int stick=0);
	void shutdown();
	//Ranges from 0 to 1 (full speed).
	void rumble(float leftMotor, float rightMotor, float time);
	void update(float dt);

	static void Calibrate(Vector &vec, float dead);
	int getIndex() const { return stickIndex; }
	int getInstanceID() const { return instanceID; }
	inline bool isEnabled() const { return enabled; }
	inline void setEnabled(bool on) { enabled = on; }

	const char *getAxisName(unsigned axis) const;
	const char *getButtonName(unsigned btn) const;
	const char *getName() const;
	const char *getGUID() const;

private:
	float clearRumbleTime;
	bool enabled;
	int stickIndex;
	int instanceID;
	SDL_Joystick *sdl_joy;
	std::string name;
	std::string guid;

#  ifdef BBGE_BUILD_SDL2
	SDL_GameController *sdl_controller;
	SDL_Haptic *sdl_haptic;
#  endif
};


#endif

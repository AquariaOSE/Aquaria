#ifndef BBGE_JOYSTICK_H
#define BBGE_JOYSTICK_H

#include <string>
#include <SDL_joystick.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(2,0,0)
#include <SDL_gamecontroller.h>
#include <SDL_haptic.h>
#endif

#include "Vector.h"

#define MAX_JOYSTICK_BTN 32
#define MAX_JOYSTICK_AXIS 32
#define MAX_JOYSTICK_HATS 8

enum JoyHatDirection // bitmask
{
	JOY_HAT_DIR_CENTERED  = 0,
	JOY_HAT_DIR_UP        = 1,
	JOY_HAT_DIR_DOWN      = 2,
	JOY_HAT_DIR_LEFT      = 4,
	JOY_HAT_DIR_RIGHT     = 8
};


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
	Vector position;

	float deadZone1, deadZone2;
	float clearRumbleTime;

	void calibrate(Vector &vec, float dead);
	bool anyButton() const;
	bool getButton(size_t id) const { return !!(buttonBitmask & (1u << id)); }
	float getAxisUncalibrated(unsigned id) const;
	JoyHatDirection getHat(unsigned id) const;
	unsigned getNumAxes() const;
	unsigned getNumButtons() const;
	unsigned getNumHats() const;
	int getIndex() const { return stickIndex; }
	int getInstanceID() const { return instanceID; }
	inline bool isEnabled() const { return enabled; }
	inline void setEnabled(bool on) { enabled = on; }

	const char *getAxisName(unsigned axis) const;
	const char *getButtonName(unsigned btn) const;
	const char *getName() const;
	const char *getGUID() const;

	Vector rightStick;

	int s1ax, s1ay, s2ax, s2ay;

private:
	void clearAxes();
	bool enabled;
	int stickIndex;
	int instanceID;
	unsigned buttonBitmask;
	unsigned numJoyAxes, numHats, numButtons;
	SDL_Joystick *sdl_joy;
	float axisRaw[MAX_JOYSTICK_AXIS];
	std::string name;
	std::string guid;

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_GameController *sdl_controller;
	SDL_Haptic *sdl_haptic;
#endif
};


#endif

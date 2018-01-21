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
#ifndef ACTIONSET_H
#define ACTIONSET_H

#include <string>
#include <vector>
#include <sstream>

#include "ActionInput.h"

typedef std::vector<ActionInput> ActionInputSet;

class ActionMapper;
class Event;

const int ACTIONSET_REASSIGN_JOYSTICK = -2;

struct JoystickConfig
{
	JoystickConfig();
	unsigned int s1ax, s1ay, s2ax, s2ay;
	float s1dead, s2dead;
};

class ActionSet
{
public:
	ActionSet();

	// import this ActionSet into ActionMapper
	void importAction(ActionMapper *mapper, const std::string &name, Event *event, int state) const;
	void importAction(ActionMapper *mapper, const std::string &name, int actionID, int sourceID) const;
	void clearActions();
	int assignJoystickByName(bool force); // -1 if no such joystick found
	void assignJoystickIdx(int idx, bool updateValues);

	// note: this only ENABLES joysticks if they are needed, but never disables any
	void updateJoystick();

	ActionInput *addActionInput(const std::string &name);
	ActionInput *getActionInputByName(const std::string &name);

	InputDevice inputMode;
	size_t joystickID; // >= 0: use that, -1 = no joystick, or ACTIONSET_REASSIGN_JOYSTICK

	// --- Saved in config ---
	ActionInputSet inputSet;
	JoystickConfig joycfg;
	std::string joystickName;
	std::string joystickGUID;
	std::string name;
	bool enabled;
	// -----------------------

	//std::string insertInputIntoString(const std::string &string);
private:
	int _whichJoystickForName(); // -1 if no such joystick found
};

#endif

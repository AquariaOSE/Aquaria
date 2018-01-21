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
#ifndef ACTIONINPUT_H
#define ACTIONINPUT_H

#include <string>

enum ActionInputSize
{
	INP_MSESIZE = 1,
	INP_KEYSIZE = 2,
	INP_JOYSIZE = 1,
	INP_COMBINED_SIZE = INP_MSESIZE + INP_KEYSIZE + INP_JOYSIZE
};

std::string getInputCodeToString(int k);
std::string getInputCodeToUserString(unsigned int k, size_t joystickID);
int getStringToInputCode(const std::string& s);

class ActionInput
{
public:
	ActionInput();

	std::string name;

	union
	{
		struct
		{
			unsigned int mse[INP_MSESIZE];
			unsigned int key[INP_KEYSIZE];
			unsigned int joy[INP_JOYSIZE];
		} single;
		int all[INP_COMBINED_SIZE];
	} data;

	std::string toString() const;
	void fromString(const std::string &read);
};

enum InputSetType
{
	INPUTSET_NONE		= 0,
	INPUTSET_KEY		= 1,
	INPUTSET_JOY		= 2,
	INPUTSET_MOUSE		= 3,
	INPUTSET_OTHER		= 4
};

enum InputDevice
{
	INPUT_NODEVICE = 0,
	INPUT_MOUSE,
	INPUT_JOYSTICK,
	INPUT_KEYBOARD
};

#endif

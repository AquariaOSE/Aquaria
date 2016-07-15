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


#define INP_MSESIZE		1
#define INP_KEYSIZE		2
#define INP_JOYSIZE		1

std::string getInputCodeToString(int k);
std::string getInputCodeToUserString(int k, int joystickID);
int getStringToInputCode(const std::string& s);

class ActionInput
{
public:
	ActionInput();

	std::string name;

	int mse[INP_MSESIZE];
	int key[INP_KEYSIZE];
	int joy[INP_JOYSIZE];

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

#endif

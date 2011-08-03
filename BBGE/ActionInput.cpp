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
#include "ActionInput.h"
// crap
#include "Core.h"

ActionInput::ActionInput()
{
	for (int i = 0; i < INP_MSESIZE; i++)	mse[i] = 0;
	for (int i = 0; i < INP_KEYSIZE; i++)	key[i] = 0;
	for (int i = 0; i < INP_JOYSIZE; i++)	joy[i] = 0;
}

std::string ActionInput::toString()
{
	std::ostringstream os;

	for (int i = 0; i < INP_MSESIZE; i++)
	{
		os << getInputCodeToString(mse[i]) << " ";
	}
	for (int i = 0; i < INP_KEYSIZE; i++)
	{
		os << getInputCodeToString(key[i]) << " ";
	}
	for (int i = 0; i < INP_JOYSIZE; i++)
	{
		os << getInputCodeToString(joy[i]) << " ";
	}

	return os.str();
}

void ActionInput::fromString(const std::string &read)
{
	std::istringstream is(read);
	std::string str;
	for (int i = 0; i < INP_MSESIZE; i++)
	{
		is >> str;
		mse[i] = getStringToInputCode(str);
	}
	for (int i = 0; i < INP_KEYSIZE; i++)
	{
		is >> str;
		key[i] = getStringToInputCode(str);
	}
	for (int i = 0; i < INP_JOYSIZE; i++)
	{
		is >> str;
		joy[i] = getStringToInputCode(str);
	}
}


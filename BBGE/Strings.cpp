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

typedef std::map<std::string, int> InputCodeMap;

InputCodeMap inputCodeMap;

void initInputCodeMap()
{
	inputCodeMap["0"]		= 0;

	inputCodeMap["KEY_A"] = KEY_A;
	inputCodeMap["KEY_B"] = KEY_B;
	inputCodeMap["KEY_C"] = KEY_C;
	inputCodeMap["KEY_D"] = KEY_D;
	inputCodeMap["KEY_E"] = KEY_E;
	inputCodeMap["KEY_F"] = KEY_F;
	inputCodeMap["KEY_G"] = KEY_G;
	inputCodeMap["KEY_H"] = KEY_H;
	inputCodeMap["KEY_I"] = KEY_I;
	inputCodeMap["KEY_J"] = KEY_J;
	inputCodeMap["KEY_K"] = KEY_K;
	inputCodeMap["KEY_L"] = KEY_L;
	inputCodeMap["KEY_M"] = KEY_M;
	inputCodeMap["KEY_N"] = KEY_N;
	inputCodeMap["KEY_O"] = KEY_O;
	inputCodeMap["KEY_P"] = KEY_P;
	inputCodeMap["KEY_Q"] = KEY_Q;
	inputCodeMap["KEY_R"] = KEY_R;
	inputCodeMap["KEY_S"] = KEY_S;
	inputCodeMap["KEY_T"] = KEY_T;
	inputCodeMap["KEY_U"] = KEY_U;
	inputCodeMap["KEY_V"] = KEY_V;
	inputCodeMap["KEY_W"] = KEY_W;
	inputCodeMap["KEY_X"] = KEY_X;
	inputCodeMap["KEY_Y"] = KEY_Y;
	inputCodeMap["KEY_Z"] = KEY_Z;

	inputCodeMap["KEY_1"] = KEY_1;
	inputCodeMap["KEY_2"] = KEY_2;
	inputCodeMap["KEY_3"] = KEY_3;
	inputCodeMap["KEY_4"] = KEY_4;
	inputCodeMap["KEY_5"] = KEY_5;
	inputCodeMap["KEY_6"] = KEY_6;
	inputCodeMap["KEY_7"] = KEY_7;
	inputCodeMap["KEY_8"] = KEY_8;
	inputCodeMap["KEY_9"] = KEY_9;
	inputCodeMap["KEY_0"] = KEY_0;

	inputCodeMap["KEY_NUMPAD1"] = KEY_NUMPAD1;
	inputCodeMap["KEY_NUMPAD2"] = KEY_NUMPAD2;
	inputCodeMap["KEY_NUMPAD3"] = KEY_NUMPAD3;
	inputCodeMap["KEY_NUMPAD4"] = KEY_NUMPAD4;
	inputCodeMap["KEY_NUMPAD5"] = KEY_NUMPAD5;
	inputCodeMap["KEY_NUMPAD6"] = KEY_NUMPAD6;
	inputCodeMap["KEY_NUMPAD7"] = KEY_NUMPAD7;
	inputCodeMap["KEY_NUMPAD8"] = KEY_NUMPAD8;
	inputCodeMap["KEY_NUMPAD9"] = KEY_NUMPAD9;
	inputCodeMap["KEY_NUMPAD0"] = KEY_NUMPAD0;

	inputCodeMap["KEY_F1"] = KEY_F1;
	inputCodeMap["KEY_F2"] = KEY_F2;
	inputCodeMap["KEY_F3"] = KEY_F3;
	inputCodeMap["KEY_F4"] = KEY_F4;
	inputCodeMap["KEY_F5"] = KEY_F5;
	inputCodeMap["KEY_F6"] = KEY_F6;
	inputCodeMap["KEY_F7"] = KEY_F7;
	inputCodeMap["KEY_F8"] = KEY_F8;
	inputCodeMap["KEY_F9"] = KEY_F9;
	inputCodeMap["KEY_F10"] = KEY_F10;
	inputCodeMap["KEY_F11"] = KEY_F11;
	inputCodeMap["KEY_F12"] = KEY_F12;

	inputCodeMap["KEY_LEFT"] = KEY_LEFT;
	inputCodeMap["KEY_RIGHT"] = KEY_RIGHT;
	inputCodeMap["KEY_UP"] = KEY_UP;
	inputCodeMap["KEY_DOWN"] = KEY_DOWN;

	inputCodeMap["KEY_SPACE"] = KEY_SPACE;
	inputCodeMap["KEY_LCONTROL"] = KEY_LCONTROL;
	inputCodeMap["KEY_RCONTROL"] = KEY_RCONTROL;
	inputCodeMap["KEY_LSHIFT"] = KEY_LSHIFT;
	inputCodeMap["KEY_RSHIFT"] = KEY_RSHIFT;
	inputCodeMap["KEY_LMETA"] = KEY_LMETA;
	inputCodeMap["KEY_RMETA"] = KEY_RMETA;
	inputCodeMap["KEY_LALT"] = KEY_LALT;
	inputCodeMap["KEY_RALT"] = KEY_RALT;
	inputCodeMap["KEY_RETURN"] = KEY_RETURN;
	inputCodeMap["KEY_TAB"] = KEY_TAB;

	inputCodeMap["KEY_ESCAPE"] = KEY_ESCAPE;

	inputCodeMap["MOUSE_BUTTON_LEFT"] = ActionMapper::MOUSE_BUTTON_LEFT;
	inputCodeMap["MOUSE_BUTTON_RIGHT"] = ActionMapper::MOUSE_BUTTON_RIGHT;
	inputCodeMap["MOUSE_BUTTON_MIDDLE"] = ActionMapper::MOUSE_BUTTON_MIDDLE;

	for (int i = 0; i < 17; i++)
	{
		std::ostringstream os;
		os << "JOY_BUTTON_" << i;
		inputCodeMap[os.str()] = ActionMapper::JOY1_BUTTON_0+i;
	}
}

void clearInputCodeMap()
{
	inputCodeMap.clear();
}

std::string getInputCodeToString(int key)
{
	for (InputCodeMap::iterator i = inputCodeMap.begin(); i != inputCodeMap.end(); i++)
	{
		if ((*i).second == key)
		{
			return (*i).first;
		}
	}
	return "";
}

// FIXME: Move stringbank to BBGE and move these strings into it. -- fg

std::string getInputCodeToUserString(int key)
{
	for (InputCodeMap::iterator i = inputCodeMap.begin(); i != inputCodeMap.end(); i++)
	{
		if ((*i).second == key)
		{
			std::string use = (*i).first;
			int idx = 0;
			idx = use.find("KEY_");
			if (idx != std::string::npos)
			{
				use = use.substr(4, use.size());
			}
			if (use == "MOUSE_BUTTON_LEFT")
				use = "Left Mouse Button";
			if (use == "MOUSE_BUTTON_RIGHT")
				use = "Right Mouse Button";
			if (use == "MOUSE_BUTTON_MIDDLE")
				use = "Middle Mouse Button";

			return use;

		}
	}
	return "";
}

int getStringToInputCode(const std::string &string)
{
	return inputCodeMap[string];
}

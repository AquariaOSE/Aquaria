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
#include "ActionMapper.h"
#include "Core.h"
#include "SDL.h"
#include "GameKeyNames.h"


static std::string inputcode2string(int k)
{
	if(k <= 0)
		return std::string();
	if(k < KEY_MAXARRAY)
	{
		// Returns KEY_* or NULL
		const std::string& str = getKeyNameFromInputCode(k);
		if(str.length())
			return str;

		// fallback
		std::ostringstream os;
		os << "K:" << k;
		return os.str();
	}

	if(k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
	{
		std::ostringstream os;
		os << "JB:" << (k - JOY_BUTTON_0);
		return os.str();
	}

	if(k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		std::ostringstream os;
		os << "AX:+" << (k - JOY_AXIS_0_POS);
		return os.str();
	}

	if(k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		std::ostringstream os;
		os << "AX:-" << (k - JOY_AXIS_0_NEG);
		return os.str();
	}

	switch(k)
	{
	case MOUSE_BUTTON_LEFT:
		return "LMB";
	case MOUSE_BUTTON_RIGHT:
		return "RMB";
	case MOUSE_BUTTON_MIDDLE:
		return "MMB";
	default:
		if(k >= MOUSE_BUTTON_EXTRA_START && k < MOUSE_BUTTON_EXTRA_START+mouseExtraButtons)
		{
			std::ostringstream os;
			os << "MB:" << (k - MOUSE_BUTTON_LEFT);
			return os.str();
		}
	}

	return std::string();
}

static const char *jaxisname(int joystickID, int axis)
{
	Joystick *j = core->getJoystick(joystickID);
	return j ? j->getAxisName(axis) : NULL;
}

static const char *jbtnname(int joystickID, int btn)
{
	Joystick *j = core->getJoystick(joystickID);
	return j ? j->getButtonName(btn) : NULL;
}

std::string getInputCodeToString(int k)
{
	std::string s = inputcode2string(k);
	return s.empty() ? "0" : s;
}

std::string getInputCodeToUserString(unsigned int k, size_t joystickID)
{
	const char *pretty = NULL, *tail = NULL;

	// Special case keyboard input:
	// Return key name for current keyboard layout!
	// It's just confusing to see Y instead of Z with a german keyboard layout...
	if(k < KEY_MAXARRAY)
	{
#ifdef BBGE_BUILD_SDL2
		pretty = SDL_GetScancodeName((SDL_Scancode)k);
		const SDL_Keycode kcode = SDL_GetKeyFromScancode((SDL_Scancode)k);
		if(kcode != SDLK_UNKNOWN)
			pretty = SDL_GetKeyName(kcode);
#else
		pretty = SDL_GetKeyName((SDLKey)k);
#endif
	}
	if(k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		pretty = jaxisname(joystickID, k - JOY_AXIS_0_POS);
		tail = "(+)";
	}
	else if(k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		pretty = jaxisname(joystickID, k - JOY_AXIS_0_NEG);
		tail = "(-)";
	}
	else if(k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
		pretty = jbtnname(joystickID, k - JOY_BUTTON_0);

	if(pretty && *pretty)
	{
		std::string s = pretty;
		if(tail)
			s += tail;
		return s;
	}

	std::string s = inputcode2string(k);
	return s.empty() ? "-" : s;
}

int getStringToInputCode(const std::string& s)
{
	if(s == "LMB")
		return MOUSE_BUTTON_LEFT;
	if(s == "RMB")
		return MOUSE_BUTTON_RIGHT;
	if(s == "MMB")
		return MOUSE_BUTTON_MIDDLE;
	if(!strncmp(s.c_str(), "K:", 2))
		return atoi(s.c_str() + 2);
	if(!strncmp(s.c_str(), "JB:", 3))
		return JOY_BUTTON_0 + atoi(s.c_str() + 3);
	if(!strncmp(s.c_str(), "MB:", 3))
		return MOUSE_BUTTON_LEFT + atoi(s.c_str() + 3);
	if(s.length() > 4 && !strncmp(s.c_str(), "AX:", 3))
	{
		int n = atoi(s.c_str() + 4);
		switch(s[3])
		{
			case '+': return JOY_AXIS_0_POS + n;
			case '-': return JOY_AXIS_0_NEG + n;
			default: return 0;
		}
	}

	// Maybe we're upgrading from an old config?
	// This handles KEY_* and some old mouse/joystick names.
	if(int k = getInputCodeFromKeyName(s.c_str()))
		return k;

	return 0;
}



ActionInput::ActionInput()
{
	for (int i = 0; i < INP_COMBINED_SIZE; i++)
		data.all[i] = 0;
}

std::string ActionInput::toString() const
{
	std::ostringstream os;

	for (int i = 0; i < INP_MSESIZE; i++)
	{
		os << getInputCodeToString(data.single.mse[i]) << " ";
	}
	for (int i = 0; i < INP_KEYSIZE; i++)
	{
		os << getInputCodeToString(data.single.key[i]) << " ";
	}
	for (int i = 0; i < INP_JOYSIZE; i++)
	{
		os << getInputCodeToString(data.single.joy[i]) << " ";
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
		data.single.mse[i] = getStringToInputCode(str);
	}
	for (int i = 0; i < INP_KEYSIZE; i++)
	{
		is >> str;
		data.single.key[i] = getStringToInputCode(str);
	}
	for (int i = 0; i < INP_JOYSIZE; i++)
	{
		is >> str;
		data.single.joy[i] = getStringToInputCode(str);
	}
}


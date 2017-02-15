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
#include "LegacyKeycodes.h"


static std::string inputcode2string(int k)
{
	if(k <= 0)
		return std::string();
	if(k < KEY_MAXARRAY)
	{
		// See parseKey() below
		std::stringstream os;
		os << "K:";
#ifdef BBGE_BUILD_SDL2
		int keycode = SDL_GetKeyFromScancode((SDL_Scancode)k);
		os << keycode << "," << k;
#else
		os << k;
#endif
		return os.str();
	}

	if(k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
	{
		std::stringstream os;
		os << "JB:" << (k - JOY_BUTTON_0);
		return os.str();
	}

	if(k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		std::stringstream os;
		os << "AX:+" << (k - JOY_AXIS_0_POS);
		return os.str();
	}

	if(k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		std::stringstream os;
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
	if(s.empty())
		return "NONE";
	return spacesToUnderscores(s);
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

	return inputcode2string(k);
}

// two comma-separated ints
// first is the keycode, second the scancode
// (Keymap-independent) Scancodes are used when built with SDL2 support and specified
// (Keymap-dependent) Keycode is used otherwise
static int parseKey(const char *ks)
{
	int k = 0;

#ifdef BBGE_BUILD_SDL2
	if(const char *comma = strchr(ks, ','))
	{
		k = atoi(comma + 1);
		if(k && k < KEY_MAXARRAY)
			return k;
	}
#endif

	// Use the keycode
	k = atoi(ks);
	if(k < KEY_MAXARRAY)
	{
#ifdef BBGE_BUILD_SDL2
		// But when we're on SDL2, don't forget to turn they keycode back into a scancode, since we work with scancodes internally
		k = SDL_GetScancodeFromKey(k);
#endif
		return k;
	}

	return 0;
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
		return parseKey(s.c_str() + 2);
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
	if(s == "NONE")
		return 0;

	// Maybe we're upgrading from an old config?
	// Note that this returns 0 for "0", which was considered "no key"
	if(int k = getInputCodeFromLegacyName(s.c_str()))
		return k;

	return 0;
}



ActionInput::ActionInput()
{
	for (int i = 0; i < INP_MSESIZE; i++)	data.single.mse[i] = 0;
	for (int i = 0; i < INP_KEYSIZE; i++)	data.single.key[i] = 0;
	for (int i = 0; i < INP_JOYSIZE; i++)	data.single.joy[i] = 0;
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


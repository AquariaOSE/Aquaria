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
#include "StringBank.h"


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
	
	if(k >= JOY_HAT_BEGIN && k < JOY_HAT_END)
	{
		if(k >= JOY_HAT_0_LEFT && k < JOY_HAT_END_LEFT)
		{
			std::ostringstream os;
			os << "HL" << (k - JOY_HAT_0_LEFT);
			return os.str();
		}
		else if(k >= JOY_HAT_0_RIGHT && k < JOY_HAT_END_RIGHT)
		{
			std::ostringstream os;
			os << "HR" << (k - JOY_HAT_0_RIGHT);
			return os.str();
		}
		else if(k >= JOY_HAT_0_UP && k < JOY_HAT_END_UP)
		{
			std::ostringstream os;
			os << "HU" << (k - JOY_HAT_0_UP);
			return os.str();
		}
		else if(k >= JOY_HAT_0_DOWN && k < JOY_HAT_END_DOWN)
		{
			std::ostringstream os;
			os << "HD" << (k - JOY_HAT_0_DOWN);
			return os.str();
		}
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
	if(k && k < KEY_MAXARRAY)
	{
#if SDL_VERSION_ATLEAST(2,0,0)
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

	std::string s;
	if(pretty && *pretty)
	{
		s = pretty;
		if(tail)
			s += tail;
		return s;
	}

	if(k >= JOY_HAT_BEGIN && k < JOY_HAT_END)
	{
		if(k >= JOY_HAT_0_LEFT && k < JOY_HAT_END_LEFT)
		{
			std::ostringstream os;
			os << "H" << (k - JOY_HAT_0_LEFT) << "<";
			return os.str();
		}
		else if(k >= JOY_HAT_0_RIGHT && k < JOY_HAT_END_RIGHT)
		{
			std::ostringstream os;
			os << "H" << (k - JOY_HAT_0_RIGHT) << ">";
			return os.str();
		}
		else if(k >= JOY_HAT_0_UP && k < JOY_HAT_END_UP)
		{
			std::ostringstream os;
			os << "H" << (k - JOY_HAT_0_UP) << "^";
			return os.str();
		}
		else if(k >= JOY_HAT_0_DOWN && k < JOY_HAT_END_DOWN)
		{
			std::ostringstream os;
			os << "H" << (k - JOY_HAT_0_DOWN) << "v";
			return os.str();
		}
	}

	s = inputcode2string(k);
	return s.empty() ? stringbank.get(SB_BBGE_NO_KEY) : s;
}

static int checkInp(const char *s, int category, int limit, StringBankIndexBBGE errid)
{
	const int k = atoi(s);
	if(!k)
		return 0;
	if(k < limit)
		return k + category;

	std::ostringstream os;
	os << stringbank.get(errid) << k;
	errorLog(os.str());
	return 0;
}

int getStringToInputCode(const std::string& s)
{
	int k = 0;

	if(s == "LMB")
		k = MOUSE_BUTTON_LEFT;
	else if(s == "RMB")
		k = MOUSE_BUTTON_RIGHT;
	else if(s == "MMB")
		k = MOUSE_BUTTON_MIDDLE;
	else if(!strncmp(s.c_str(), "K:", 2))
		k = checkInp(s.c_str() + 2, 0, KEY_MAXARRAY, SB_BBGE_INVALID_KEY_ID);
	else if(!strncmp(s.c_str(), "JB:", 3))
		k = checkInp(s.c_str() + 3, JOY_BUTTON_0, JOY_BUTTON_END, SB_BBGE_INVALID_JOY_BTN);
	else if(!strncmp(s.c_str(), "MB:", 3))
		k = checkInp(s.c_str() + 3, MOUSE_BUTTON_LEFT, MOUSE_BUTTON_EXTRA_END, SB_BBGE_INVALID_MOUSE_BTN);
	else if(s.length() > 4 && !strncmp(s.c_str(), "AX:", 3))
	{
		switch(s[3])
		{
			case '+':
				k = checkInp(s.c_str() + 4, JOY_AXIS_0_POS, JOY_AXIS_END_POS, SB_BBGE_INVALID_JOY_AXIS_POS);
				break;
			case '-':
				k = checkInp(s.c_str() + 4, JOY_AXIS_0_NEG, JOY_AXIS_END_NEG, SB_BBGE_INVALID_JOY_AXIS_NEG);
				break;
			default:
				return 0;
		}
	}
	else if(s.length()  > 2 && s[0] == 'H')  // joystick hat
	{
		JoyHatDirection hd;
		switch(s[1])
		{
			case 'L': hd = JOY_HAT_DIR_LEFT; break;
			case 'R': hd = JOY_HAT_DIR_RIGHT; break;
			case 'U': hd = JOY_HAT_DIR_UP; break;
			case 'D': hd = JOY_HAT_DIR_DOWN; break;
			default: return 0;
		}
		unsigned hatID = atoi(s.c_str() + 2);
		return joyHatToActionButton(hatID, hd);
	}
	else
	{
		// Maybe we're upgrading from an old config?
		// This handles KEY_* and some old mouse/joystick names.
		k = getInputCodeFromKeyName(s.c_str());
	}

	// Non-configurable keys
	if(k == KEY_ESCAPE)
		return 0;

	if(k < ACTION_BUTTON_ENUM_SIZE)
		return k;

	std::ostringstream os;
	os << "ActionButton out of range: [" << s << "] = " << k;
	errorLog(os.str());
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


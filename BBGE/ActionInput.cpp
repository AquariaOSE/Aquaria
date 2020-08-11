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

static const char *jaxisname(int joystickID, unsigned axis)
{
	Joystick *j = core->getJoystick(joystickID);
	const char *s = j ? j->getAxisName(axis - 1) : NULL;
	return s && *s ? s : NULL;
}

static const char *jbtnname(int joystickID, unsigned btn)
{
	Joystick *j = core->getJoystick(joystickID);
	const char *s = j ? j->getButtonName(btn - 1) : NULL;
	return s && *s ? s : NULL;
}

struct JoyHatDir
{
	char c, pretty;
	int x, y;
};
static const JoyHatDir s_joyhat[] =
{
	{ 'u', '^', 0, -1 },
	{ 'd', 'v', 0, 1 },
	{ 'l', '<', -1, 0 },
	{ 'r', '>', 0, 1 },
	{ 0, 0, 0, 0 }, // terminator
};

static std::string num2str(unsigned x)
{
	std::ostringstream os;
	os << x;
	return os.str();
}

static std::string getMouseButtonToString(unsigned b)
{
	static const char *buf[] = { "LMB", "MMB", "RMB" };
	if(b && b < 4) // 1 2 3
		return buf[b-1];
	return num2str(b);
}

static const char *getRawSDLKeyName(unsigned k)
{
	const char *s = NULL;
#ifdef BBGE_BUILD_SDL2
	s = SDL_GetScancodeName((SDL_Scancode)k);
#else
	s = SDL_GetKeyName((SDLKey)k);
#endif
	return s && *s ? s : NULL;
}

static const char *getPrettySDLKeyName(unsigned k)
{
	const char *s;
#ifdef BBGE_BUILD_SDL2
	// Special case keyboard input:
	// Return key name for current keyboard layout!
	// It's just confusing to see Y instead of Z with a german keyboard layout...
	const SDL_Keycode kcode = SDL_GetKeyFromScancode((SDL_Scancode)k);
	if(kcode == SDLK_UNKNOWN)
		s = SDL_GetScancodeName((SDL_Scancode)k);
	else
		s = SDL_GetKeyName(kcode);
#else
	s = SDL_GetKeyName((SDLKey)k);
#endif
	return s && *s ? s : NULL;
}

static unsigned getKeyCodeFromSDLName(const char *s)
{
#ifdef BBGE_BUILD_SDL2
	return SDL_GetScancodeFromName(s);
#endif
	return SDL_GetKeyFromName(s);
}

static std::string getKeyToString(unsigned k)
{
	assert(k < KEY_MAXARRAY);
	const char *s = getLegacyKeyNameFromInputCode(k);
	if(s)
		return s;
	s = getRawSDLKeyName(k);
	if(s)
		return s;

	return num2str(k);
}

static std::string getJoystickDataToString(const ActionInput::JoyData& jd)
{
	std::ostringstream os;

	switch(jd.ctrlType)
	{
		case INP_CTRL_BUTTON:
			os << jd.ctrlID;
			break;
		case INP_CTRL_AXIS:
			os << 'A' << ((jd.x < 0 || jd.y < 0) ? '-' : '+') << jd.ctrlID;
			break;
		case INP_CTRL_HAT:
			os << 'H';
			for(const JoyHatDir *hd = &s_joyhat[0]; hd->c; ++hd)
				if(jd.x == hd->x && jd.y == hd->y)
				{
					os << hd->c << jd.ctrlID;
					break;
				}
			break;
	}
	return os.str();
}

static std::string getJoystickDataToPrettyString(const ActionInput::JoyData& jd, int joystickID)
{
	if(!jd.ctrlID)
		return std::string();

	std::ostringstream os;
	static const char *axislabels[] = { "(-)", "", "(+)" };

	switch(jd.ctrlType)
	{
		case INP_CTRL_BUTTON:
		{
			const char *s = jbtnname(joystickID, jd.ctrlID);
			if(s)
				os << s;
			else
				os << jd.ctrlID;
		}
		break;

		case INP_CTRL_AXIS:
		{
			const char *s = jaxisname(joystickID, jd.ctrlID);
			if(s)
				os << s;
			else
				os << "A:" << jd.ctrlID;
			os << axislabels[jd.x + 1] << axislabels[jd.y + 1];
		}
		break;

		case INP_CTRL_HAT:
		{
			os << "H:" << jd.ctrlID << ":";
			for(const JoyHatDir *hd = &s_joyhat[0]; hd->c; ++hd)
				if(jd.x == hd->x && jd.y == hd->y)
				{
					os << hd->pretty;
					break;
				}
		}
		break;
	}
	return os.str();
}

static unsigned getMouseButtonFromString_Legacy(const std::string& s)
{
	if(s == "LMB")
		return MOUSE_BUTTON_LEFT;
	else if(s == "RMB")
		return MOUSE_BUTTON_RIGHT;
	else if(s == "MMB")
		return MOUSE_BUTTON_MIDDLE;

	return getLegacyInputCodeFromKeyName(s.c_str());
}


static unsigned getMouseButtonFromString(const std::string& s)
{
	if(s.empty() || s == "0")
		return 0;

	unsigned k = getMouseButtonFromString_Legacy(s);
	switch(k)
	{
		case MOUSE_BUTTON_LEFT: return 1;
		case MOUSE_BUTTON_RIGHT: return 2;
		case MOUSE_BUTTON_MIDDLE: return 3;
		default: ;
	}

	return atoi(s.c_str());
}

static unsigned getKeyFromString(const std::string& s)
{
	if(s.empty() || s == "0")
		return 0;

	unsigned k = getLegacyInputCodeFromKeyName(s.c_str()); // fallback
	if(k)
		return k;
	k = getKeyCodeFromSDLName(s.c_str());
	if(k)
		return k;
	
	return atoi(s.c_str());
}

static ActionInput::JoyData getJostickDataFromString(const std::string& s)
{
	ActionInput::JoyData jd;

	if(s.length() && s != "0")
	{
		switch(s[0])
		{
		case 'A': // axis
		{
			jd.ctrlType = INP_CTRL_AXIS;
			int ax = atoi(s.c_str() + 1);
			jd.ctrlID = ax < 0 ? -ax : ax;
			jd.x = ax < 0 ? -1 : 1;
		}
		break;

		case 'H': // hat
			jd.ctrlType = INP_CTRL_HAT;
			if(s.length() > 1)
				for(const JoyHatDir *hd = &s_joyhat[0]; hd->c; ++hd)
					if(hd->c == s[1])
					{
						jd.x = hd->x;
						jd.y = hd->y;
						jd.ctrlID = atoi(s.c_str() + 2);
						break;
					}
		break;

		default: // button
			jd.ctrlType = INP_CTRL_BUTTON;
			jd.ctrlID = atoi(s.c_str() + 1);
		}
	}

	return jd;
}

ActionInput::JoyData::JoyData()
{
	memset(this, 0, sizeof(*this));
}

ActionInput::ActionInput(const std::string& name_)
: name(name_)
{
	for (int i = 0; i < INP_MSESIZE; i++)
		mse[i] = 0;
	for (int i = 0; i < INP_KEYSIZE; i++)
		key[i] = 0;
	// joy[] inited by its own ctor
}

std::string ActionInput::toString() const
{
	std::ostringstream os;

	for (int i = 0; i < INP_MSESIZE; i++)
	{
		os << getMouseButtonToString(mse[i]) << " ";
	}
	for (int i = 0; i < INP_KEYSIZE; i++)
	{
		os << getKeyToString(key[i]) << " ";
	}
	for (int i = 0; i < INP_JOYSIZE; i++)
	{
		os << getJoystickDataToString(joy[i]) << " ";
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
		mse[i] = getMouseButtonFromString(str);
	}
	for (int i = 0; i < INP_KEYSIZE; i++)
	{
		is >> str;
		key[i] = getKeyFromString(str);
	}
	for (int i = 0; i < INP_JOYSIZE; i++)
	{
		is >> str;
		joy[i] = getJostickDataFromString(str);
	}
}

struct TypeAndSlot
{
	InputDeviceType type;
	unsigned slot;
};
static TypeAndSlot getTypeAndSlot(unsigned field)
{
	TypeAndSlot ts;
	if(field < INP_MSESIZE)
	{
		ts.type = INP_DEV_MOUSE;
		ts.slot = field;
	}
	else if(field < INP_MSESIZE + INP_KEYSIZE)
	{
		ts.type = INP_DEV_KEYBOARD;
		ts.slot = field - INP_MSESIZE;
	}
	else if(field < INP_MSESIZE + INP_KEYSIZE + INP_JOYSIZE)
	{
		ts.type = INP_DEV_JOYSTICK;
		ts.slot = field - (INP_MSESIZE + INP_KEYSIZE);
	}
	else
	{
		assert(false);
		ts.type = InputDeviceType(-1);
		ts.slot = unsigned(-1);
	}
	return ts;
}

unsigned ActionInput::GetField(InputDeviceType dev, unsigned slot)
{
	unsigned base = 0;
	switch(dev)
	{
		case INP_DEV_MOUSE:
			base = 0;
			assert(slot < INP_MSESIZE);
			break;
		case INP_DEV_KEYBOARD:
			base = INP_MSESIZE;
			assert(slot < INP_KEYSIZE);
			break;
		case INP_DEV_JOYSTICK:
			base = INP_MSESIZE+INP_KEYSIZE;
			assert(slot < INP_JOYSIZE);
			break;
		default:
			assert(false);
	}
	return base + slot;
}

InputDeviceType ActionInput::GetDevice(unsigned field)
{
	return getTypeAndSlot(field).type;
}

std::string ActionInput::prettyPrintField(unsigned field, int joystickID /* = -1 */) const
{
	TypeAndSlot ts = getTypeAndSlot(field);
	switch(ts.type)
	{
		case INP_DEV_MOUSE:
			return getMouseButtonToString(mse[field]);
		case INP_DEV_KEYBOARD:
		{
			unsigned k = key[ts.slot];
			const char *s = getPrettySDLKeyName(k);
			if(s)
				return s;
			return getKeyToString(k);
		}
		case INP_DEV_JOYSTICK:
		{
			const JoyData& jd = joy[ts.slot];
			return getJoystickDataToPrettyString(jd, joystickID);
		}
	}
	assert(false);
	return std::string();
}

bool ActionInput::ImportField(const RawInput& inp, unsigned field)
{
	TypeAndSlot ts = getTypeAndSlot(field);
	return ts.type == inp.src.deviceType && Import(inp, ts.slot);
}

bool ActionInput::Import(const RawInput& inp, unsigned slot)
{
	switch(inp.src.deviceType)
	{
		case INP_DEV_MOUSE:
		{
			if(slot >= INP_MSESIZE)
				return false;
			if(inp.src.ctrlID == INP_CTRL_BUTTON)
			{
				mse[slot] = inp.src.ctrlID + 1;
				return true;
			}
		}
		break;

		case INP_DEV_KEYBOARD:
		{
			if(slot >= INP_KEYSIZE)
				return false;
			if(inp.src.ctrlID == INP_CTRL_BUTTON)
			{
				key[slot] = inp.src.ctrlID;
				return true;
			}
		}
		break;

		case INP_DEV_JOYSTICK:
		{
			if(slot >= INP_JOYSIZE)
				return false;
			JoyData jd;
			InputControlType t = inp.src.ctrlType;
			if(t == INP_CTRL_BUTTON || t == INP_CTRL_AXIS || t == INP_CTRL_HAT)
			{
				jd.ctrlType = t;
				jd.ctrlID = inp.src.ctrlID + 1;
				jd.x = inp.u.ivec.x;
				jd.y = inp.u.ivec.y;
				if(t == INP_CTRL_AXIS)
					jd.x = inp.u.axis < 0 ? -1 : 1;
				return true;
			}
		}
		break;
	}

	return false;
}

bool ActionInput::Export(RawInput& inp, unsigned field, unsigned deviceID) const
{
	TypeAndSlot ts = getTypeAndSlot(field);
	inp.src.deviceType = ts.type;
	inp.src.ctrlType = INP_CTRL_BUTTON;
	inp.src.deviceID = deviceID;
	inp.u.pressed = 1;

	switch(ts.type)
	{
		case INP_DEV_MOUSE:
		{
			unsigned m = mse[ts.slot];
			if(!m)
				return false;
			inp.src.ctrlID = m - 1;
			return true;
		}

		case INP_DEV_KEYBOARD:
		{
			unsigned k = key[ts.slot];
			if(!k)
				return false;
			inp.src.ctrlID = k;
			return true;
		}
		case INP_DEV_JOYSTICK:
		{
			const JoyData& jd = joy[field - (INP_MSESIZE + INP_KEYSIZE)];
			if(!jd.ctrlID)
				return false;
			inp.src.ctrlType = jd.ctrlType;
			inp.src.ctrlID = jd.ctrlID - 1;
			inp.u.ivec.x = jd.x;
			inp.u.ivec.y = jd.y;
			if(jd.ctrlType == INP_CTRL_AXIS)
				inp.u.axis = jd.x < 0 ? -1 : 1;
			return true;
		}
	}
	assert(false);
	return false;
}

bool ActionInput::hasEntry(unsigned field) const
{
	TypeAndSlot ts = getTypeAndSlot(field);

	switch(ts.type)
	{
		case INP_DEV_MOUSE:
			return mse[ts.slot];
		case INP_DEV_KEYBOARD:
			return key[ts.slot];
		case INP_DEV_JOYSTICK:
			return joy[ts.slot].ctrlID;
	}
	return false;
}

void ActionInput::clearEntry(unsigned field)
{
	TypeAndSlot ts = getTypeAndSlot(field);

	switch(ts.type)
	{
		case INP_DEV_MOUSE:
			mse[ts.slot] = 0;
			break;
		case INP_DEV_KEYBOARD:
			key[ts.slot] = 0;
			break;
		case INP_DEV_JOYSTICK:
			joy[ts.slot].ctrlID = 0;
			break;
	}
}

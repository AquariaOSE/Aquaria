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
#ifndef __core__
#define __core__

/*
valid BUILD_ flags
WIN32/BUILD_WIN32
BUILD_MACOSX
BUILD_X360
BUILD_LINUX
*/

#include "Base.h"
#include "RenderObject.h"
#include "SoundManager.h"
#include "ActionMapper.h"
#include "Event.h"
#include "StateManager.h"
#include "Light.h"
#include "Flags.h"
//#include "Scripting.h"
#include "Effects.h"
#include "Localization.h"

#include "DarkLayer.h"

/*
#ifdef BBGE_BUILD_WINDOWS
	#include "Joystick.h"
#endif
*/

#include "FrameBuffer.h"
#include "Shader.h"

class ParticleEffect;

class ParticleManager;

void initInputCodeMap();
void clearInputCodeMap();
std::string getInputCodeToString(int key);
std::string getInputCodeToUserString(int key);
int getStringToInputCode(const std::string &string);

enum TimeUpdateType
{
	TIMEUPDATE_DYNAMIC	= 0,
	TIMEUPDATE_FIXED	= 1
};

struct ScreenMode
{
	ScreenMode() { idx = x = y = 0; }
	ScreenMode(int i, int x, int y) : idx(i), x(x), y(y) {}

	int idx, x, y;
};

struct CoreSettings
{
	CoreSettings() { renderOn = true; updateOn = true; runInBackground = false; prebufferSounds = false; }
	bool renderOn;
	bool runInBackground;
	bool updateOn; // NOT IMPLEMENTED YET
	bool prebufferSounds;
};

enum CoreFlags
{
	CF_CLEARBUFFERS	= 0x00000001,
	CF_SORTENABLED	= 0x00000010
};

enum CoreLayers
{
	LR_NONE		= -1
};

const int NO_FOLLOW_CAMERA = -999;

class AfterEffectManager;

class Texture;

const int baseVirtualWidth		= 800;
const int baseVirtualHeight		= 600;

#if (defined(BBGE_BUILD_WINDOWS)) && !defined(BBGE_BUILD_SDL)
enum GameKeys
{
	KEY_BACKSPACE = DIK_BACKSPACE,
	KEY_CAPSLOCK = DIK_CAPSLOCK,
	KEY_CIRCUMFLEX = DIK_CIRCUMFLEX,
	KEY_DOWNARROW = DIK_DOWNARROW,
	KEY_LALT = DIK_LALT,
	KEY_LEFTARROW = DIK_LEFTARROW,
	KEY_NUMPADMINUS = DIK_NUMPADMINUS,
	KEY_NUMPADPERIOD = DIK_NUMPADPERIOD,
	KEY_NUMPADPLUS = DIK_NUMPADPLUS,
	KEY_NUMPADSLASH = DIK_NUMPADSLASH,
	KEY_NUMPADSTAR = DIK_NUMPADSTAR,
	KEY_PGDN = DIK_PGDN,
	KEY_PGUP = DIK_PGUP,
	KEY_RALT = DIK_RALT,
	KEY_RIGHTARROW = DIK_RIGHTARROW,
	KEY_UPARROW = DIK_UPARROW,
	KEY_TILDE = DIK_GRAVE,
    KEY_0 = DIK_0,
    KEY_1 = DIK_1,
    KEY_2 = DIK_2,
    KEY_3 = DIK_3,
    KEY_4 = DIK_4,
    KEY_5 = DIK_5,
    KEY_6 = DIK_6,
    KEY_7 = DIK_7,
    KEY_8 = DIK_8,
    KEY_9 = DIK_9,
    KEY_A = DIK_A,
    KEY_ABNT_C1 = DIK_ABNT_C1,
    KEY_ABNT_C2 = DIK_ABNT_C2,
    KEY_ADD = DIK_ADD,
    KEY_APOSTROPHE = DIK_APOSTROPHE,
    KEY_APPS = DIK_APPS,
    KEY_AT = DIK_AT,
    KEY_AX = DIK_AX,
    KEY_B = DIK_B,
    KEY_BACK = DIK_BACK,
    KEY_BACKSLASH = DIK_BACKSLASH,
    KEY_C = DIK_C,
    KEY_CALCULATOR = DIK_CALCULATOR,
    KEY_CAPITAL = DIK_CAPITAL,
    KEY_COLON = DIK_COLON,
    KEY_COMMA = DIK_COMMA,
    KEY_CONVERT = DIK_CONVERT,
    KEY_D = DIK_D,
    KEY_DECIMAL = DIK_DECIMAL,
    KEY_DELETE = DIK_DELETE,
    KEY_DIVIDE = DIK_DIVIDE,
    KEY_DOWN = DIK_DOWN,
    KEY_E = DIK_E,
    KEY_END = DIK_END,
    KEY_EQUALS = DIK_EQUALS,
    KEY_ESCAPE = DIK_ESCAPE,
    KEY_F = DIK_F,
    KEY_F1 = DIK_F1,
    KEY_F2 = DIK_F2,
    KEY_F3 = DIK_F3,
    KEY_F4 = DIK_F4,
    KEY_F5 = DIK_F5,
    KEY_F6 = DIK_F6,
    KEY_F7 = DIK_F7,
    KEY_F8 = DIK_F8,
    KEY_F9 = DIK_F9,
    KEY_F10 = DIK_F10,
    KEY_F11 = DIK_F11,
    KEY_F12 = DIK_F12,
    KEY_F13 = DIK_F13,
    KEY_F14 = DIK_F14,
    KEY_F15 = DIK_F15,
    KEY_G = DIK_G,
    KEY_GRAVE = DIK_GRAVE,
    KEY_H = DIK_H,
    KEY_HOME = DIK_HOME,
    KEY_I = DIK_I,
    KEY_INSERT = DIK_INSERT,
    KEY_J = DIK_J,
    KEY_K = DIK_K,
    KEY_KANA = DIK_KANA,
    KEY_KANJI = DIK_KANJI,
    KEY_L = DIK_L,
    KEY_LBRACKET = DIK_LBRACKET,
    KEY_LCONTROL = DIK_LCONTROL,
    KEY_LEFT = DIK_LEFT,
    KEY_LMENU = DIK_LMENU,
    KEY_LSHIFT = DIK_LSHIFT,
    KEY_LMETA = DIK_LWIN,
    KEY_M = DIK_M,
    KEY_MAIL = DIK_MAIL,
    KEY_MEDIASELECT = DIK_MEDIASELECT,
    KEY_MEDIASTOP = DIK_MEDIASTOP,
    KEY_MINUS = DIK_MINUS,
    KEY_MULTIPLY = DIK_MULTIPLY,
    KEY_MUTE = DIK_MUTE,
    KEY_MYCOMPUTER = DIK_MYCOMPUTER,
    KEY_N = DIK_N,
    KEY_NEXT = DIK_NEXT,
    KEY_NEXTTRACK = DIK_NEXTTRACK,
    KEY_NOCONVERT = DIK_NOCONVERT,
    KEY_NUMLOCK = DIK_NUMLOCK,
    KEY_NUMPAD0 = DIK_NUMPAD0,
    KEY_NUMPAD1 = DIK_NUMPAD1,
    KEY_NUMPAD2 = DIK_NUMPAD2,
    KEY_NUMPAD3 = DIK_NUMPAD3,
    KEY_NUMPAD4 = DIK_NUMPAD4,
    KEY_NUMPAD5 = DIK_NUMPAD5,
    KEY_NUMPAD6 = DIK_NUMPAD6,
    KEY_NUMPAD7 = DIK_NUMPAD7,
    KEY_NUMPAD8 = DIK_NUMPAD8,
    KEY_NUMPAD9 = DIK_NUMPAD9,
    KEY_NUMPADCOMMA = DIK_NUMPADCOMMA,
    KEY_NUMPADENTER = DIK_NUMPADENTER,
    KEY_NUMPADEQUALS = DIK_NUMPADEQUALS,
    KEY_O = DIK_O,
    KEY_OEM_102 = DIK_OEM_102,
    KEY_P = DIK_P,
    KEY_PAUSE = DIK_PAUSE,
    KEY_PERIOD = DIK_PERIOD,
    KEY_PLAYPAUSE = DIK_PLAYPAUSE,
    KEY_POWER = DIK_POWER,
    KEY_PREVTRACK = DIK_PREVTRACK,
    KEY_PRIOR = DIK_PRIOR,
    KEY_Q = DIK_Q,
    KEY_R = DIK_R,
    KEY_RBRACKET = DIK_RBRACKET,
    KEY_RCONTROL = DIK_RCONTROL,
    KEY_RETURN = DIK_RETURN,
    KEY_RIGHT = DIK_RIGHT,
    KEY_RMENU = DIK_RMENU,
    KEY_RSHIFT = DIK_RSHIFT,
    KEY_RMETA = DIK_RWIN,
    KEY_S = DIK_S,
    KEY_SCROLL = DIK_SCROLL,
    KEY_SEMICOLON = DIK_SEMICOLON,
    KEY_SLASH = DIK_SLASH,
    KEY_SLEEP = DIK_SLEEP,
    KEY_SPACE = DIK_SPACE,
    KEY_STOP = DIK_STOP,
    KEY_SUBTRACT = DIK_SUBTRACT,
    KEY_SYSRQ = DIK_SYSRQ,
    KEY_T = DIK_T,
    KEY_TAB = DIK_TAB,
    KEY_U = DIK_U,
    KEY_UNDERLINE = DIK_UNDERLINE,
    KEY_UNLABELED = DIK_UNLABELED,
    KEY_UP = DIK_UP,
    KEY_V = DIK_V,
    KEY_VOLUMEDOWN = DIK_VOLUMEDOWN,
    KEY_VOLUMEUP = DIK_VOLUMEUP,
    KEY_W = DIK_W,
    KEY_WAKE = DIK_WAKE,
    KEY_WEBBACK = DIK_WEBBACK,
    KEY_WEBFAVORITES = DIK_WEBFAVORITES,
    KEY_WEBFORWARD = DIK_WEBFORWARD,
    KEY_WEBHOME = DIK_WEBHOME,
    KEY_WEBREFRESH = DIK_WEBREFRESH,
    KEY_WEBSEARCH = DIK_WEBSEARCH,
    KEY_WEBSTOP = DIK_WEBSTOP,
    KEY_X = DIK_X,
    KEY_Y = DIK_Y,
    KEY_YEN = DIK_YEN,
    KEY_Z = DIK_Z,
	KEY_ANYKEY = 4059,
	KEY_MAXARRAY = 256
};
#elif defined(BBGE_BUILD_SDL)
enum GameKeys
{
	// replace with GLFW equivalent
	/*
	KEY_DOWNARROW = GLFW_KEY_DOWN,
	KEY_RIGHTARROW = GLFW_KEY_RIGHT,
	KEY_UPARROW = GLFW_KEY_UP,
	KEY_LEFTARROW = GLFW_KEY_LEFT,
	*/


	KEY_LSUPER = SDLK_LSUPER,
	KEY_RSUPER = SDLK_RSUPER,
	KEY_LMETA = SDLK_LMETA,
	KEY_RMETA = SDLK_RMETA,

	KEY_BACKSPACE = SDLK_BACKSPACE,
	KEY_PRINTSCREEN = SDLK_PRINT,

	//KEY_CAPSLOCK = DIK_CAPSLOCK,
	//KEY_CIRCUMFLEX = DIK_CIRCUMFLEX,
	KEY_LALT = SDLK_LALT,
	KEY_RALT = SDLK_RALT,
	KEY_LSHIFT = SDLK_LSHIFT,
	KEY_RSHIFT = SDLK_RSHIFT,
	KEY_LCONTROL = SDLK_LCTRL,
	KEY_RCONTROL = SDLK_RCTRL,
	KEY_NUMPADMINUS = SDLK_KP_MINUS,
	KEY_NUMPADPERIOD = SDLK_KP_PERIOD,
	KEY_NUMPADPLUS = SDLK_KP_PLUS,
	KEY_NUMPADSLASH = SDLK_KP_DIVIDE,
	KEY_NUMPADSTAR = SDLK_KP_MULTIPLY,
	KEY_PGDN = SDLK_PAGEDOWN,
	KEY_PGUP = SDLK_PAGEUP,
	KEY_APOSTROPHE = SDLK_QUOTE,
	KEY_EQUALS = SDLK_EQUALS,
	KEY_SEMICOLON = SDLK_SEMICOLON,
	KEY_LBRACKET = SDLK_LEFTBRACKET,
	KEY_RBRACKET = SDLK_RIGHTBRACKET,
	//KEY_RALT = GLFW_KEY_RALT,
	KEY_TILDE = SDLK_BACKQUOTE,
    KEY_0 = SDLK_0,
    KEY_1 = SDLK_1,
    KEY_2 = SDLK_2,
    KEY_3 = SDLK_3,
    KEY_4 = SDLK_4,
    KEY_5 = SDLK_5,
    KEY_6 = SDLK_6,
    KEY_7 = SDLK_7,
    KEY_8 = SDLK_8,
    KEY_9 = SDLK_9,
    KEY_A = SDLK_a,
	KEY_B = SDLK_b,
	KEY_C = SDLK_c,
	KEY_D = SDLK_d,
	KEY_E = SDLK_e,
	KEY_F = SDLK_f,
	KEY_G = SDLK_g,
	KEY_H = SDLK_h,
	KEY_I = SDLK_i,
	KEY_J = SDLK_j,
	KEY_K = SDLK_k,
	KEY_L = SDLK_l,
	KEY_M = SDLK_m,
	KEY_N = SDLK_n,
	KEY_O = SDLK_o,
	KEY_P = SDLK_p,
	KEY_Q = SDLK_q,
	KEY_R = SDLK_r,
	KEY_S = SDLK_s,
	KEY_T = SDLK_t,
	KEY_U = SDLK_u,
	KEY_V = SDLK_v,
	KEY_W = SDLK_w,
	KEY_X = SDLK_x,
	KEY_Y = SDLK_y,
	KEY_Z = SDLK_z,
	
	KEY_LEFT = SDLK_LEFT,
	KEY_RIGHT = SDLK_RIGHT,
	KEY_UP = SDLK_UP,
	KEY_DOWN = SDLK_DOWN,

	KEY_NUMPAD1 = SDLK_KP1,
	KEY_NUMPAD2 = SDLK_KP2,
	KEY_NUMPAD3 = SDLK_KP3,
	KEY_NUMPAD4 = SDLK_KP4,
	KEY_NUMPAD5 = SDLK_KP5,
	KEY_NUMPAD6 = SDLK_KP6,
	KEY_NUMPAD7 = SDLK_KP7,
	KEY_NUMPAD8 = SDLK_KP8,
	KEY_NUMPAD9 = SDLK_KP9,
	KEY_NUMPAD0 = SDLK_KP0,

	KEY_DELETE = SDLK_DELETE,
	KEY_SPACE = SDLK_SPACE,
	KEY_RETURN = SDLK_RETURN,
	KEY_PERIOD = SDLK_PERIOD,
	KEY_MINUS = SDLK_MINUS,
	KEY_CAPSLOCK = SDLK_CAPSLOCK,
	KEY_SYSRQ = SDLK_SYSREQ,
	KEY_TAB = SDLK_TAB,
	KEY_HOME = SDLK_HOME,
	KEY_END = SDLK_END,
	KEY_COMMA = SDLK_COMMA,
	KEY_SLASH = SDLK_SLASH,

    KEY_F1 = SDLK_F1,
    KEY_F2 = SDLK_F2,
    KEY_F3 = SDLK_F3,
    KEY_F4 = SDLK_F4,
    KEY_F5 = SDLK_F5,
    KEY_F6 = SDLK_F6,
    KEY_F7 = SDLK_F7,
    KEY_F8 = SDLK_F8,
    KEY_F9 = SDLK_F9,
    KEY_F10 = SDLK_F10,
    KEY_F11 = SDLK_F11,
    KEY_F12 = SDLK_F12,
    KEY_F13 = SDLK_F13,
    KEY_F14 = SDLK_F14,
    KEY_F15 = SDLK_F15,

	KEY_ESCAPE = SDLK_ESCAPE,
	KEY_ANYKEY = 4059,
	KEY_MAXARRAY = SDLK_LAST+1
};
#elif defined(BBGE_BUILD_XINPUT)
enum GameKeys
{
	KEY_LSUPER = 0,
	KEY_RSUPER = 0,
	KEY_LMETA = 0,
	KEY_RMETA = 0,
	KEY_BACKSPACE = 0,
	KEY_PRINTSCREEN = 0,
	KEY_LALT = 0,
	KEY_RALT = 0,
	KEY_LSHIFT = 0,
	KEY_RSHIFT = 0,
	KEY_LCONTROL = 0,
	KEY_RCONTROL = 0,
	KEY_NUMPADMINUS = 0,
	KEY_NUMPADPERIOD = 0,
	KEY_NUMPADPLUS = 0,
	KEY_NUMPADSLASH = 0,
	KEY_NUMPADSTAR = 0,
	KEY_PGDN = 0,
	KEY_PGUP = 0,
	KEY_TILDE = 0,
    KEY_0 = 0,
    KEY_1 = 0,
    KEY_2 = 0,
    KEY_3 = 0,
    KEY_4 = 0,
    KEY_5 = 0,
    KEY_6 = 0,
    KEY_7 = 0,
    KEY_8 = 0,
    KEY_9 = 0,
    KEY_A = 0,
	KEY_B = 0,
	KEY_C = 0,
	KEY_D = 0,
	KEY_E = 0,
	KEY_F = 0,
	KEY_G = 0,
	KEY_H = 0,
	KEY_I = 0,
	KEY_J = 0,
	KEY_K = 0,
	KEY_L = 0,
	KEY_M = 0,
	KEY_N = 0,
	KEY_O = 0,
	KEY_P = 0,
	KEY_Q = 0,
	KEY_R = 0,
	KEY_S = 0,
	KEY_T = 0,
	KEY_U = 0,
	KEY_V = 0,
	KEY_W = 0,
	KEY_X = 0,
	KEY_Y = 0,
	KEY_Z = 0,
	
	KEY_LEFT = 0,
	KEY_RIGHT = 0,
	KEY_UP = 0,
	KEY_DOWN = 0,
	
	KEY_NUMPAD1 = 0,
	KEY_NUMPAD2 = 0,
	KEY_NUMPAD3 = 0,
	KEY_NUMPAD4 = 0,
	KEY_NUMPAD5 = 0,
	KEY_NUMPAD6 = 0,
	KEY_NUMPAD7 = 0,
	KEY_NUMPAD8 = 0,
	KEY_NUMPAD9 = 0,
	KEY_NUMPAD0 = 0,
	
	KEY_DELETE = 0,
	KEY_SPACE = 0,
	KEY_RETURN = 0,
	KEY_PERIOD = 0,
	KEY_MINUS = 0,
	KEY_CAPSLOCK = 0,
	KEY_SYSRQ = 0,
	KEY_TAB = 0,
	KEY_HOME = 0,
	KEY_END = 0,
	KEY_COMMA = 0,
	KEY_SLASH = 0,

    KEY_F1 = 0,
    KEY_F2 = 0,
    KEY_F3 = 0,
    KEY_F4 = 0,
    KEY_F5 = 0,
    KEY_F6 = 0,
    KEY_F7 = 0,
    KEY_F8 = 0,
    KEY_F9 = 0,
    KEY_F10 = 0,
    KEY_F11 = 0,
    KEY_F12 = 0,
    KEY_F13 = 0,
    KEY_F14 = 0,
    KEY_F15 = 0,

	KEY_ESCAPE = 0,
	KEY_ANYKEY = 4059,
	KEY_MAXARRAY = 256
};
#elif defined(BBGE_BUILD_GLFW)
enum GameKeys
{
	// replace with GLFW equivalent
	/*
	KEY_DOWNARROW = GLFW_KEY_DOWN,
	KEY_RIGHTARROW = GLFW_KEY_RIGHT,
	KEY_UPARROW = GLFW_KEY_UP,
	KEY_LEFTARROW = GLFW_KEY_LEFT,
	*/


	KEY_BACKSPACE = GLFW_KEY_BACKSPACE,
	//KEY_CAPSLOCK = DIK_CAPSLOCK,
	//KEY_CIRCUMFLEX = DIK_CIRCUMFLEX,
	KEY_LALT = GLFW_KEY_LALT,
	KEY_RALT = GLFW_KEY_RALT,
	KEY_LSHIFT = GLFW_KEY_LSHIFT,
	KEY_RSHIFT = GLFW_KEY_RSHIFT,
	KEY_LCONTROL = GLFW_KEY_LCTRL,
	KEY_RCONTROL = GLFW_KEY_RCTRL,
	KEY_NUMPADMINUS = GLFW_KEY_KP_SUBTRACT,
	KEY_NUMPADPERIOD = GLFW_KEY_KP_DECIMAL,
	KEY_NUMPADPLUS = GLFW_KEY_KP_ADD,
	KEY_NUMPADSLASH = GLFW_KEY_KP_DIVIDE,
	KEY_NUMPADSTAR = GLFW_KEY_KP_MULTIPLY,
	KEY_PGDN = GLFW_KEY_PAGEDOWN,
	KEY_PGUP = GLFW_KEY_PAGEUP,
	//KEY_RALT = GLFW_KEY_RALT,
	KEY_TILDE = '`',
    KEY_0 = '0',
    KEY_1 = '1',
    KEY_2 = '2',
    KEY_3 = '3',
    KEY_4 = '4',
    KEY_5 = '5',
    KEY_6 = '6',
    KEY_7 = '7',
    KEY_8 = '8',
    KEY_9 = '9',
    KEY_A = 'A',
	KEY_B = 'B',
	KEY_C = 'C',
	KEY_D = 'D',
	KEY_E = 'E',
	KEY_F = 'F',
	KEY_G = 'G',
	KEY_H = 'H',
	KEY_I = 'I',
	KEY_J = 'J',
	KEY_K = 'K',
	KEY_L = 'L',
	KEY_M = 'M',
	KEY_N = 'N',
	KEY_O = 'O',
	KEY_P = 'P',
	KEY_Q = 'Q',
	KEY_R = 'R',
	KEY_S = 'S',
	KEY_T = 'T',
	KEY_U = 'U',
	KEY_V = 'V',
	KEY_W = 'W',
	KEY_X = 'X',
	KEY_Y = 'Y',
	KEY_Z = 'Z',
	
	KEY_LEFT = GLFW_KEY_LEFT,
	KEY_RIGHT = GLFW_KEY_RIGHT,
	KEY_UP = GLFW_KEY_UP,
	KEY_DOWN = GLFW_KEY_DOWN,
	
	KEY_NUMPAD1 = GLFW_KEY_KP_1,
	KEY_NUMPAD2 = GLFW_KEY_KP_2,
	KEY_NUMPAD3 = GLFW_KEY_KP_3,
	KEY_NUMPAD4 = GLFW_KEY_KP_4,
	KEY_NUMPAD5 = GLFW_KEY_KP_5,
	KEY_NUMPAD6 = GLFW_KEY_KP_6,
	KEY_NUMPAD7 = GLFW_KEY_KP_7,
	KEY_NUMPAD8 = GLFW_KEY_KP_8,
	KEY_NUMPAD9 = GLFW_KEY_KP_9,
	KEY_NUMPAD0 = GLFW_KEY_KP_0,
	
	KEY_DELETE = GLFW_KEY_DEL,
	KEY_SPACE = GLFW_KEY_SPACE,
	// mac os x
	KEY_RETURN = 13,
	KEY_PERIOD = '.',
	KEY_MINUS = '-',
	KEY_CAPSLOCK = -1,
	KEY_SYSRQ = '`',
	KEY_TAB = GLFW_KEY_TAB,
	KEY_HOME = GLFW_KEY_HOME,
	KEY_END = GLFW_KEY_END,
	KEY_COMMA = ',',
	KEY_SLASH = '/',
    //KEY_ABNT_C1 = DIK_ABNT_C1,
    //KEY_ABNT_C2 = DIK_ABNT_C2,
	/*
    KEY_ADD = DIK_ADD,
    KEY_APOSTROPHE = DIK_APOSTROPHE,
    KEY_APPS = DIK_APPS,
    KEY_AT = DIK_AT,
    KEY_AX = DIK_AX,
	*/
	/*
    KEY_B = 'B',
    KEY_BACK = DIK_BACK,
    KEY_BACKSLASH = DIK_BACKSLASH,
    KEY_C = DIK_C,
    KEY_CALCULATOR = DIK_CALCULATOR,
    KEY_CAPITAL = DIK_CAPITAL,
    KEY_COLON = DIK_COLON,
    KEY_COMMA = DIK_COMMA,
    KEY_CONVERT = DIK_CONVERT,
    KEY_D = DIK_D,
    KEY_DECIMAL = DIK_DECIMAL,
    KEY_DELETE = DIK_DELETE,
    KEY_DIVIDE = DIK_DIVIDE,
    KEY_DOWN = DIK_DOWN,
    KEY_E = DIK_E,
    KEY_END = DIK_END,
    KEY_EQUALS = DIK_EQUALS,
    KEY_ESCAPE = DIK_ESCAPE,
    KEY_F = DIK_F,
	*/
    KEY_F1 = GLFW_KEY_F1,
    KEY_F2 = GLFW_KEY_F2,
    KEY_F3 = GLFW_KEY_F3,
    KEY_F4 = GLFW_KEY_F4,
    KEY_F5 = GLFW_KEY_F5,
    KEY_F6 = GLFW_KEY_F6,
    KEY_F7 = GLFW_KEY_F7,
    KEY_F8 = GLFW_KEY_F8,
    KEY_F9 = GLFW_KEY_F9,
    KEY_F10 = GLFW_KEY_F10,
    KEY_F11 = GLFW_KEY_F11,
    KEY_F12 = GLFW_KEY_F12,
    KEY_F13 = GLFW_KEY_F13,
    KEY_F14 = GLFW_KEY_F14,
    KEY_F15 = GLFW_KEY_F15,
	/*
    KEY_G = DIK_G,
    KEY_GRAVE = DIK_GRAVE,
    KEY_H = DIK_H,
    KEY_HOME = DIK_HOME,
    KEY_I = DIK_I,
    KEY_INSERT = DIK_INSERT,
    KEY_J = DIK_J,
    KEY_K = DIK_K,
    KEY_KANA = DIK_KANA,
    KEY_KANJI = DIK_KANJI,
    KEY_L = DIK_L,
    KEY_LBRACKET = DIK_LBRACKET,
    KEY_LCONTROL = DIK_LCONTROL,
    KEY_LEFT = DIK_LEFT,
    KEY_LMENU = DIK_LMENU,
    KEY_LWIN = DIK_LWIN,
    KEY_M = DIK_M,
    KEY_MAIL = DIK_MAIL,
    KEY_MEDIASELECT = DIK_MEDIASELECT,
    KEY_MEDIASTOP = DIK_MEDIASTOP,
    KEY_MINUS = DIK_MINUS,
    KEY_MULTIPLY = DIK_MULTIPLY,
    KEY_MUTE = DIK_MUTE,
    KEY_MYCOMPUTER = DIK_MYCOMPUTER,
    KEY_N = DIK_N,
    KEY_NEXT = DIK_NEXT,
    KEY_NEXTTRACK = DIK_NEXTTRACK,
    KEY_NOCONVERT = DIK_NOCONVERT,
    KEY_NUMLOCK = DIK_NUMLOCK,
    KEY_NUMPAD0 = DIK_NUMPAD0,
    KEY_NUMPAD1 = DIK_NUMPAD1,
    KEY_NUMPAD2 = DIK_NUMPAD2,
    KEY_NUMPAD3 = DIK_NUMPAD3,
    KEY_NUMPAD4 = DIK_NUMPAD4,
    KEY_NUMPAD5 = DIK_NUMPAD5,
    KEY_NUMPAD6 = DIK_NUMPAD6,
    KEY_NUMPAD7 = DIK_NUMPAD7,
    KEY_NUMPAD8 = DIK_NUMPAD8,
    KEY_NUMPAD9 = DIK_NUMPAD9,
    KEY_NUMPADCOMMA = DIK_NUMPADCOMMA,
    KEY_NUMPADENTER = DIK_NUMPADENTER,
    KEY_NUMPADEQUALS = DIK_NUMPADEQUALS,
    KEY_O = DIK_O,
    KEY_OEM_102 = DIK_OEM_102,
    KEY_P = DIK_P,
    KEY_PAUSE = DIK_PAUSE,
    KEY_PERIOD = DIK_PERIOD,
    KEY_PLAYPAUSE = DIK_PLAYPAUSE,
    KEY_POWER = DIK_POWER,
    KEY_PREVTRACK = DIK_PREVTRACK,
    KEY_PRIOR = DIK_PRIOR,
    KEY_Q = DIK_Q,
    KEY_R = DIK_R,
    KEY_RBRACKET = DIK_RBRACKET,
    KEY_RCONTROL = DIK_RCONTROL,
    KEY_RETURN = DIK_RETURN,
    KEY_RIGHT = DIK_RIGHT,
    KEY_RMENU = DIK_RMENU,
    
    KEY_RWIN = DIK_RWIN,
    KEY_S = DIK_S,
    KEY_SCROLL = DIK_SCROLL,
    KEY_SEMICOLON = DIK_SEMICOLON,
    KEY_SLASH = DIK_SLASH,
    KEY_SLEEP = DIK_SLEEP,
    KEY_SPACE = DIK_SPACE,
    KEY_STOP = DIK_STOP,
    KEY_SUBTRACT = DIK_SUBTRACT,
    KEY_SYSRQ = DIK_SYSRQ,
    KEY_T = DIK_T,
    KEY_TAB = DIK_TAB,
    KEY_U = DIK_U,
    KEY_UNDERLINE = DIK_UNDERLINE,
    KEY_UNLABELED = DIK_UNLABELED,
    KEY_UP = DIK_UP,
    KEY_V = DIK_V,
    KEY_VOLUMEDOWN = DIK_VOLUMEDOWN,
    KEY_VOLUMEUP = DIK_VOLUMEUP,
    KEY_W = DIK_W,
    KEY_WAKE = DIK_WAKE,
    KEY_WEBBACK = DIK_WEBBACK,
    KEY_WEBFAVORITES = DIK_WEBFAVORITES,
    KEY_WEBFORWARD = DIK_WEBFORWARD,
    KEY_WEBHOME = DIK_WEBHOME,
    KEY_WEBREFRESH = DIK_WEBREFRESH,
    KEY_WEBSEARCH = DIK_WEBSEARCH,
    KEY_WEBSTOP = DIK_WEBSTOP,
    KEY_X = DIK_X,
    KEY_Y = DIK_Y,
    KEY_YEN = DIK_YEN,
    KEY_Z = DIK_Z,
	*/
	KEY_ESCAPE = GLFW_KEY_ESC,
	KEY_ANYKEY = 4059
};
#endif


enum ButtonState { UP = 0, DOWN };

struct MouseButtons
{
	MouseButtons ()
	{
		left = UP;
		right = UP;
		middle = UP;
	}
	
	ButtonState left, right, middle;
};

struct Mouse
{
	Mouse()
	{
		scrollWheel = scrollWheelChange = lastScrollWheel = 0;
		buttonsEnabled = true;
		movementEnabled = true;
	}
	Vector position, lastPosition;
	MouseButtons buttons;
	MouseButtons pure_buttons;
	Vector change;


	// movementEnabled is not implemented yet
	bool buttonsEnabled, movementEnabled;

	int scrollWheel, scrollWheelChange, lastScrollWheel;
};

const int maxJoyBtns = 64;

class Joystick
{
public:
	Joystick();
	void init(int stick=0);
	void shutdown();
	//Ranges from 0 to 65535 (full speed).
	void rumble(float leftMotor, float rightMotor, float time);
	void update(float dt);
	Vector position, lastPosition;
	ButtonState buttons[maxJoyBtns];
	float deadZone1, deadZone2;
	float clearRumbleTime;

	void callibrate(Vector &vec, float dead);

	float leftTrigger, rightTrigger;
	bool leftThumb, rightThumb, leftShoulder, rightShoulder, dpadLeft, dpadRight, dpadUp, dpadDown;
	bool btnStart, btnSelect;
	Vector rightStick;
	bool inited, xinited;
	bool anyButton();
#ifdef BBGE_BUILD_SDL
	SDL_Joystick *sdl_joy;
#endif
#ifdef __LINUX__
	int eventfd;
	int16_t effectid;
#endif
	int stickIndex;

	int s1ax, s1ay, s2ax, s2ay;
};

enum FollowCameraLock
{
	FCL_NONE		= 0,
	FCL_HORZ		= 1,
	FCL_VERT		= 2
};

//RenderObject Layer Type (enable only one)
//#define RLT_DYNAMIC		// Dynamic list
#define RLT_FIXED		// Static array
//#define RLT_MAP		// Mapping

typedef std::vector <RenderObject*> RenderObjects;
typedef std::list <RenderObject*> RenderObjectList;
typedef std::map <intptr_t, RenderObject*> RenderObjectMap;

class RenderObjectLayer
{
public:
	RenderObjectLayer();
	~RenderObjectLayer();
	void add(RenderObject* r);
	void remove(RenderObject* r);
	void moveToFront(RenderObject *r);
	void moveToBack(RenderObject *r);
	void setCull(bool cull);
	void setOptimizeStatic(bool opt);
	void sort();
	void renderPass(int pass);
	void reloadDevice();

	inline bool empty()
	{
	#ifdef RLT_FIXED
		return objectCount == 0;
	#endif
	#ifdef RLT_DYNAMIC
		return renderObjectList.empty();
	#endif
		return false;
	}

	inline RenderObject *getFirst()
	{
	#ifdef RLT_DYNAMIC
		if (renderObjectList.empty()) return 0;
		iter = renderObjectList.begin();
		return *iter;
	#endif
	#ifdef RLT_MAP
		if (renderObjectMap.empty()) return 0;
		iter = renderObjectMap.begin();
		return (*iter).second;
	#endif
	#ifdef RLT_FIXED
		iter = 0;
		return getNext();
	#endif
	}

	RenderObject *getNext()
	{
	#ifdef RLT_DYNAMIC
		if (iter == renderObjectList.end()) return 0;
		iter++;
		if (iter == renderObjectList.end()) return 0;
		return *iter;
	#endif
	#ifdef RLT_MAP
		if (iter == renderObjectMap.end()) return 0;
		iter++;
		if (iter == renderObjectMap.end()) return 0;
		return (*iter).second;
	#endif
	#ifdef RLT_FIXED
		const int size = renderObjects.size();
		int i;
		for (i = iter; i < size; i++)
		{
			if (renderObjects[i] != 0)
				break;
		}
		if (i < size)
		{
			iter = i+1;
			return renderObjects[i];
		}
		else
		{
			iter = i;
			return 0;
		}
	#endif
		return 0;
	}

	//inclusive
	int startPass, endPass;
	bool visible;
	float followCamera;

	int followCameraLock;
	bool cull;
	bool update;

	int mode;

	Vector color;

protected:

	void clearDisplayList();
	void generateDisplayList();
	inline void renderOneObject(RenderObject *robj);

	bool optimizeStatic;
	bool displayListValid;
	int displayListGeneration;
	struct DisplayListElement {
		DisplayListElement() {isList = false; u.robj = 0;}
		bool isList;  // True if this is a GL display list
		union {
			RenderObject *robj;
			GLuint listID;
		} u;
	};
	std::vector<DisplayListElement> displayList;

#ifdef RLT_DYNAMIC
	RenderObjectList renderObjectList;
	RenderObjectList::iterator iter;
#endif
#ifdef RLT_MAP
	RenderObjectMap renderObjectMap;
	RenderObjectMap::iterator iter;
#endif
#ifdef RLT_FIXED
	RenderObjects renderObjects;
	int objectCount;
	int firstFreeIdx;
	int iter;
#endif
};

class Core : public ActionMapper, public StateManager
{
public:

	enum RemoveResource
	{
		DESTROY = 0,
		NO_DESTROY
	};
	// init
	Core(const std::string &filesystem, int numRenderLayers, const std::string &appName="BBGE", int particleSize=1024, std::string userDataSubFolder="");
	void initPlatform(const std::string &filesystem);
	~Core();

	virtual void init();

	void initRenderObjectLayers(int num);

	void applyState(const std::string &state);
	//bool createGlWindow(char* title, int width, int height, int bits, bool fullscreenflag);
	bool createWindow(int width, int height, int bits, bool fullscreen, std::string windowTitle="");
	//void setWindowTitle(const std::string &title); // func not yet written
	void clearBuffers();	
	void render(int startLayer=-1, int endLayer=-1, bool useFrameBufferIfAvail=true);
	void showBuffer();
	void quit();
	bool isShuttingDown();
	bool isWindowFocus();

	void instantQuit();

	void cacheRender();

	void setSDLGLAttributes();

	void reloadResources();
	void unloadResources();
	
	std::string getPreferencesFolder();
	std::string getUserDataFolder();

	std::string adjustFilenameCase(const char *buf);
	std::string adjustFilenameCase(const std::string &str) { return adjustFilenameCase(str.c_str()); };

	void resetCamera();

	virtual void shutdown();

	void main(float runTime = -1); // can use main 

	void adjustWindowPosition(int x, int y);

	// state functions

	std::string getTextureLoadName(const std::string &texture);

	void setMousePosition(const Vector &p);

	void toggleScreenMode(int t=0);

	void enable2D(int pixelScaleX=0, int pixelScaleY=0, bool forcePixelScale=false);
	void addRenderObject(RenderObject *o, int layer=0);
	void switchRenderObjectLayer(RenderObject *o, int toLayer);
	void addResource(Resource *r);
	Resource *findResource(const std::string &name);
	Texture *findTexture(const std::string &name);
	void removeResource(std::string name, RemoveResource removeFlag);

	Texture *addTexture(const std::string &texture);
	void removeTexture(std::string texture);

	PostProcessingFX postProcessingFx;

	enum RemoveRenderObjectFlag { DESTROY_RENDER_OBJECT=0, DO_NOT_DESTROY_RENDER_OBJECT };
	void removeRenderObject(RenderObject *r, RemoveRenderObjectFlag flag = DESTROY_RENDER_OBJECT);

	void setMouseConstraint(bool on);
	void setMouseConstraintCircle(int mouseCircle);
	
	void setReentryInputGrab(int on);
	
	void action(int id, int state){}

	bool exists(const std::string &file);

	void enqueueRenderObjectDeletion(RenderObject *object);
	void clearGarbage();
	void clearResources();


	bool isNested() { return nestedMains > 1; }
	int getNestedMains() { return nestedMains; }
	void quitNestedMain();

	int getWindowWidth() { return width; }
	int getWindowHeight() { return height; }

	void updateCursorFromJoystick(float dt, int spd);

	uint32 getTicks();

	float stopWatch(int d);


	float stopWatchStartTime;


	void resetGraphics(int w, int h, int fullscreen=-1, int vsync=-1, int bpp=-1);

/*
#ifdef BBGE_BUILD_OPENGL
	void getWindowHeight(int *height)
	{glfwGetWindowSize(0, height);}

	void getWindowWidth(int *width)
	{glfwGetWindowSize(width, 0);}
#endif
*/

	void setDockIcon(const std::string &ident);

	Vector getGameCursorPosition();
	Vector getGamePosition(const Vector &v);

	Vector joystickPosition;

	bool coreVerboseDebug;


	Vector screenCenter;
	void sort();
	void sortLayer(int layer);

	void print(int x, int y, const char *str, float sz=1);

	// windows variables
	#ifdef BBGE_BUILD_WINDOWS
		HDC			hDC;		// Private GDI Device Context
		HGLRC		hRC;		// Permanent Rendering Context
		HWND		hWnd;		// Holds Our Window Handle
		HINSTANCE	hInstance;		// Holds The Instance Of The Application
	#endif

	std::vector<Resource*>resources;
	std::vector<std::string> resourceNames;

	RenderObjectLayer *getRenderObjectLayer(int i);
	std::vector <int> renderObjectLayerOrder;
	//typedef std::list<RenderObject*> RenderObjects;
	typedef std::vector<RenderObjectLayer> RenderObjectLayers;
	RenderObjectLayers renderObjectLayers;

	RenderObjectList garbage;

	SoundManager *sound;

	float aspect;

	int width, height;

	enum Modes { MODE_NONE=-1, MODE_3D=0, MODE_2D };

	InterpolatedVector globalScale;
	Vector globalResolutionScale;
	Vector screenCapScale;

	virtual void onResetScene(){}

	virtual void onPlayedVoice(const std::string &name){}

	InterpolatedVector cameraPos;

	int fps;
	bool loopDone;

	Mouse mouse;

	AfterEffectManager *afterEffectManager;

	ParticleManager *particleManager;

	//Scripting::Script script;


	void setBaseTextureDirectory(const std::string &baseTextureDirectory)
	{ this->baseTextureDirectory = baseTextureDirectory; }
	std::string getBaseTextureDirectory()
	{
		return baseTextureDirectory;
	}


	virtual bool canChangeState();
	void resetTimer();

	inline int getVirtualWidth()
	{
		return virtualWidth;
	}

	inline int getVirtualHeight()
	{
		return virtualHeight;
	}

	unsigned char *grabScreenshot(int x, int y, int w, int h);
	unsigned char *grabCenteredScreenshot(int w, int h);
	int saveScreenshotTGA(const std::string &filename);
	void save64x64ScreenshotTGA(const std::string &filename);
	void saveSizedScreenshotTGA(const std::string &filename, int sz, int crop34);
	void saveCenteredScreenshotTGA(const std::string &filename, int sz);

	virtual void msg(const std::string &message);

	bool minimized;
	std::string getEnqueuedJumpState();
	int cullRadius;
	float cullRadiusSqr;
	Vector cullCenter;
	int screenCullX1, screenCullY1, screenCullX2, screenCullY2;
	unsigned int renderObjectCount, processedRenderObjectCount, totalRenderObjectCount;
	float invGlobalScale, invGlobalScaleSqr;

	void screenshot();

	void clearRenderObjects();

	void applyMatrixStackToWorld();
	void translateMatrixStack(float x, float y, float z=0);
	//void translateMatrixStackRelative(float x, float y, float z=0);
	void rotateMatrixStack(float x, float y, float z);
	void scaleMatrixStack(float x, float y, float z=1);
	void rotateMatrixStack(float z);
	void setColor(float r, float g, float b, float a);

	void bindTexture(int stage, unsigned int handle);
	
#ifdef BBGE_BUILD_DIRECTX
	
	void blitD3DVerts(IDirect3DTexture9 *texture, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y, float v4x, float v4y);
	void blitD3D (IDirect3DTexture9 *texture, int width, int height);
	void blitD3DPreTrans (IDirect3DTexture9 *texture, float x, float y, int absWidth, int absHeight);
	void blitD3DEx (IDirect3DTexture9 *texture, int width, int height, float u1=0, float v1=0, float u2=1, float v2=1);
	void blitD3DGradient(D3DCOLOR ulc0, D3DCOLOR ulc1, D3DCOLOR ulc2, D3DCOLOR ulc3);
	LPDIRECT3DDEVICE9 getD3DDevice();
	LPD3DXSPRITE getD3DSprite();
	LPD3DXMATRIXSTACK getD3DMatrixStack();
#endif

	bool getKeyState(int k);
	bool getMouseButtonState(int m);
	
	int currentLayerPass;
	int keys[KEY_MAXARRAY];
	Flags flags;
	virtual void debugLog(const std::string &s);
	virtual void errorLog(const std::string &s);
	void messageBox(const std::string &title, const std::string &msg);
	bool getShiftState();
	bool getAltState();
	bool getCtrlState();
	bool getMetaState();

	virtual void generateCollisionMask(RenderObject *r){}

	DarkLayer darkLayer;

	int redBits, greenBits, blueBits, alphaBits;

	void setupRenderPositionAndScale();
	void setupGlobalResolutionScale();

	
	int particlesPaused;

	//JoystickData joystickData[4];
	bool joystickEnabled;
	bool joystickOverrideMouse;
	/*
	int numJoysticks;
	DIJOYSTATE2 joystate;
	Joystick* joysticks[4];
	*/

	bool debugLogTextures;
	

	Joystick joystick;
	std::string getInternalTextureName(const std::string &name);
	void setClearColor(const Vector &c);
	Vector getClearColor();
	int flipMouseButtons;
	void initFrameBuffer();
	FrameBuffer frameBuffer;
	void updateRenderObjects(float dt);
	bool joystickAsMouse;
	virtual void prepScreen(bool t){}

	bool updateMouse;
	bool frameOutputMode;

	int overrideStartLayer, overrideEndLayer;
	
	void setWindowCaption(const std::string &caption, const std::string &icon);

	ParticleEffect* createParticleEffect(const std::string &name, const Vector &position, int layer, float rotz=0);

	std::string secondaryTexturePath;

	bool hasFocus();

	EventQueue eventQueue;

	float aspectX, aspectY;

	float get_old_dt() { return old_dt; }

	bool debugLogActive;

	void setInputGrab(bool on);

	bool isFullscreen();

	int viewOffX, viewOffY;

	int getVirtualOffX();
	int getVirtualOffY();

	void centerMouse();

	int vw2, vh2;
	Vector center;

	void enable2DWide(int rx, int ry);
	
	void enumerateScreenModes();

	std::vector<ScreenMode> screenModes;

	void pollEvents();

	CoreSettings settings;

	int tgaSave(const char *filename, short int width, short int height, unsigned char	pixelDepth, unsigned char	*imageData);
	int zgaSave(const char *filename, short int width, short int height, unsigned char	pixelDepth, unsigned char	*imageData);

	volatile int dbg_numThreadDecoders;

protected:

	std::string fpsDebugString;

	TimeUpdateType timeUpdateType;
	int fixedFPS;

	void updateCullData();

	std::string userDataFolder;

	int grabInputOnReentry;

	int virtualOffX, virtualOffY;

	void initIcon();

	float old_dt;
	
	std::string debugLogPath;

	virtual void onReloadResources();

	Texture* doTextureAdd(const std::string &texture, const std::string &name, std::string internalTextureName);
	
	void deleteRenderObjectMemory(RenderObject *r);
	bool _hasFocus;
	bool lib_graphics, lib_sound, lib_input;
	Vector clearColor;
	bool updateCursorFromMouse;
	virtual void unloadDevice();
	virtual void reloadDevice();

	std::string appName;
	bool mouseConstraint;
	int mouseCircle;
	
	bool doMouseConstraint();
	
	virtual void onMouseInput(){}
	bool doScreenshot;
	int baseCullRadius;
	bool initSoundLibrary(const std::string &defaultDevice);
	bool initInputLibrary();
	bool initJoystickLibrary(int numSticks=1);
	bool initGraphicsLibrary(int w, int h, bool fullscreen, int vsync, int bpp, bool recreate=true);
	void shutdownInputLibrary();
	void shutdownJoystickLibrary();
	void shutdownGraphicsLibrary(bool kill=true);
	void shutdownSoundLibrary();

	int afterEffectManagerLayer;
	bool sortEnabled;
	Vector cameraOffset;
	std::vector<float> avgFPS;
	float sortTimer;
	bool sortFlag;
	virtual void modifyDt(float &dt){}
	void setPixelScale(int pixelScaleX, int pixelScaleY);
	

	int virtualHeight, virtualWidth;
	
	bool shuttingDown;
	bool quitNestedMainFlag;
	bool clearedGarbageFlag;
	int nestedMains;
	std::string baseTextureDirectory;
#ifdef BBGE_BUILD_WINDOWS
	__int64 lastTime, curTime, freq;
#endif

	std::ofstream _logOut;

#ifdef BBGE_BUILD_SDL
	int nowTicks, thenTicks;
#endif
	
	int _vsync, _bpp;
	bool _fullscreen;

	int numSavedScreenshots;

	//unsigned int windowWidth, windowHeight;

	
	int tgaSaveSeries(char	*filename,  short int width, short int height, unsigned char pixelDepth, unsigned char *imageData);
	virtual void onUpdate(float dt);
	virtual void onRender(){}

	void setupFileAccess();
};

extern Core *core;

#include "RenderObject_inline.h"

#endif

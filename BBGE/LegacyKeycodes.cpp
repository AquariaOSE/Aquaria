#include "LegacyKeycodes.h"
#include "ActionStatus.h"
#include <map>
#include <sstream>

typedef std::map<std::string, int> InputCodeMap;

InputCodeMap inputCodeMap;

static void initInputCodeMap()
{
#define K(k)inputCodeMap[#k] = k;
	K(KEY_A)
	K(KEY_B)
	K(KEY_C)
	K(KEY_D)
	K(KEY_E)
	K(KEY_F)
	K(KEY_G)
	K(KEY_H)
	K(KEY_I)
	K(KEY_J)
	K(KEY_K)
	K(KEY_L)
	K(KEY_M)
	K(KEY_N)
	K(KEY_O)
	K(KEY_P)
	K(KEY_Q)
	K(KEY_R)
	K(KEY_S)
	K(KEY_T)
	K(KEY_U)
	K(KEY_V)
	K(KEY_W)
	K(KEY_X)
	K(KEY_Y)
	K(KEY_Z)
	K(KEY_1)
	K(KEY_2)
	K(KEY_3)
	K(KEY_4)
	K(KEY_5)
	K(KEY_6)
	K(KEY_7)
	K(KEY_8)
	K(KEY_9)
	K(KEY_0)
	K(KEY_NUMPAD1)
	K(KEY_NUMPAD2)
	K(KEY_NUMPAD3)
	K(KEY_NUMPAD4)
	K(KEY_NUMPAD5)
	K(KEY_NUMPAD6)
	K(KEY_NUMPAD7)
	K(KEY_NUMPAD8)
	K(KEY_NUMPAD9)
	K(KEY_NUMPAD0)
	K(KEY_F1)
	K(KEY_F2)
	K(KEY_F3)
	K(KEY_F4)
	K(KEY_F5)
	K(KEY_F6)
	K(KEY_F7)
	K(KEY_F8)
	K(KEY_F9)
	K(KEY_F10)
	K(KEY_F11)
	K(KEY_F12)
	K(KEY_LEFT)
	K(KEY_RIGHT)
	K(KEY_UP)
	K(KEY_DOWN)
	K(KEY_SPACE)
	K(KEY_LCONTROL)
	K(KEY_RCONTROL)
	K(KEY_LSHIFT)
	K(KEY_RSHIFT)
	K(KEY_LMETA)
	K(KEY_RMETA)
	K(KEY_LALT)
	K(KEY_RALT)
	K(KEY_RETURN)
	K(KEY_TAB)
	K(KEY_ESCAPE)

	K(MOUSE_BUTTON_LEFT)
	K(MOUSE_BUTTON_RIGHT)
	K(MOUSE_BUTTON_MIDDLE)
#undef K

	for (int jb = JOY_BUTTON_0; jb < JOY_BUTTON_END; jb++)
	{
		std::ostringstream os;
		os << "JOY_BUTTON_" << jb - JOY_BUTTON_0;
		inputCodeMap[os.str()] = jb;
	}
}

int getInputCodeFromLegacyName(const char *name)
{
	return inputCodeMap[name];
}


struct LegacyKeymapInitializer
{
	LegacyKeymapInitializer() { initInputCodeMap(); }
};
static LegacyKeymapInitializer s_kinit;

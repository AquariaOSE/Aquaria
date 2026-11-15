/* SDL2 config for Windows builds via zig build (MinGW-w64 headers).
   Upstream config, minus Windows.Gaming.Input: MinGW's winsdkver.h claims a Win10 SDK but lacks windows.gaming.input.h. */

#ifndef SDL_config_h_
#define SDL_config_h_

#include "SDL_platform.h"
#include "SDL_config_windows.h"

#undef HAVE_WINDOWS_GAMING_INPUT_H
#undef SDL_JOYSTICK_WGI

#endif /* SDL_config_h_ */

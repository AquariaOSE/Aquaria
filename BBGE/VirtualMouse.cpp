#include "VirtualMouse.h"
#include <SDL.h>

VirtualMouse::VirtualMouse()
: buttons(_buttons), pure_buttons(_pure_buttons)
, actionToLeft(-1), actionToRight(-1)
{
}

VirtualMouse::update(float dt)
{
	int x, int y;
	Uint32 mousestate = SDL_GetMouseState(&x, &y);
	pure_buttons.left   = mousestate & SDL_BUTTON_LMASK;
	pure_buttons.right  = mousestate & SDL_BUTTON_MMASK;
	pure_buttons.middle = mousestate & SDL_BUTTON_RMASK;

	buttons = pure_buttons;

	ActionMapper::onUpdate(dt);

	if(actionToLeft >= 0 && isActing(actionToLeft))
		buttons.left = true;
	if(actionToRight> 0 && isActing(actionToRight))
		buttons.right = true;

	// TODO: controller to mouse?
}


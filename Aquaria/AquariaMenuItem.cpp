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
#include "AquariaMenuItem.h"
#include "DSQ.h"
#include "Game.h"
#include "ActionInput.h"
#include "InGameMenu.h"

#include <tinyxml2.h>
using namespace tinyxml2;

const float moveDelay = 0.2f;

float AquariaGuiElement::guiMoveTimer = 0;
AquariaGuiElement::GuiElements AquariaGuiElement::guiElements;
bool AquariaGuiElement::canDirMoveGlobal = true;

int AquariaGuiElement::currentGuiInputLevel = 0;
AquariaGuiElement *AquariaGuiElement::currentFocus = 0;

AquariaGuiElement::AquariaGuiElement()
{
	for (int i = 0; i < DIR_MAX; i++)
	{
		dirMove[i] = 0;
	}

	guiMoveTimer = 0;

	guiElements.push_back(this);

	canDirMove = true;

	guiInputLevel = 0;
}

bool AquariaGuiElement::hasInput()
{
	if (guiInputLevel >= AquariaGuiElement::currentGuiInputLevel)
	{
		return true;
	}
	return false;
}

void AquariaGuiElement::clean()
{
	guiElements.remove(this);
}

void AquariaGuiElement::setDirMove(int dir, AquariaGuiElement *item)
{
	if (dir >= 0 && dir < DIR_MAX)
	{
		dirMove[dir] = item;
	}
}

void AquariaGuiElement::setCanDirMove(bool on)
{
	canDirMove = on;
}

bool AquariaGuiElement::hasFocus() const
{
	return this == currentFocus;
}

void AquariaGuiElement::setFocus(bool v)
{
	if (v)
	{
		currentFocus = this;
		if (dsq->getInputMode() == INPUT_JOYSTICK || dsq->getInputMode() == INPUT_KEYBOARD)
			core->setMousePosition(getGuiPosition());

		AquariaGuiElement *gui=0, *guiThis = (AquariaGuiElement*)this;
		for (GuiElements::iterator i = guiElements.begin(); i != guiElements.end(); i++)
		{
			gui = (*i);
			if (gui && gui != guiThis)
			{
				gui->setFocus(false);
			}
		}
	}
	else if(this == currentFocus)
		currentFocus = 0;
}

void AquariaGuiElement::updateMovement(float dt)
{
	if (hasFocus() && isGuiVisible() && canDirMove && canDirMoveGlobal && hasInput())
	{
		if (guiMoveTimer==0)
		{
			Direction dir = GetDirection();

			if (dir == DIR_NONE) return;

			AquariaGuiElement *gui = 0;
			if (dir > DIR_NONE && dir < DIR_MAX)
			{
				gui = dirMove[dir];
				if (!gui)
					gui = FindClosestTo(this, getGuiPosition(), dir);
				if (gui)
				{
					gui->setFocus(true);
					guiMoveTimer = moveDelay;
				}
			}
		}
	}
}

AquariaGuiElement *AquariaGuiElement::FocusClosestToMouse(Direction dir)
{
	if (dir > DIR_NONE && dir < DIR_MAX)
	{
		AquariaGuiElement *gui = FindClosestTo(NULL, core->mouse.position, dir);
		if (gui)
		{
			gui->setFocus(true);
			guiMoveTimer = moveDelay;
			return gui;
		}
	}
	return NULL;
}

void AquariaGuiElement::UpdateGlobalFocus(float dt)
{
	if (guiMoveTimer > 0)
	{
		guiMoveTimer -= dt;
		if (guiMoveTimer < 0) guiMoveTimer = 0;
	}

	if(!currentFocus && guiMoveTimer == 0)
		FocusClosestToMouse(GetDirection());
}

Direction AquariaGuiElement::GetDirection()
{
	Direction dir = DIR_NONE;
	
	StateObject *obj = dsq->getTopStateObject(); // usually Game...
	if (obj)
	{
		if (obj->isActing(ACTION_MENULEFT, -1))			dir = DIR_LEFT;
		else if (obj->isActing(ACTION_MENURIGHT, -1))	dir = DIR_RIGHT;
		else if (obj->isActing(ACTION_MENUUP, -1))		dir = DIR_UP;
		else if (obj->isActing(ACTION_MENUDOWN, -1))	dir = DIR_DOWN;
	}

	return dir;
}

AquariaGuiElement *AquariaGuiElement::FindClosestTo(AquariaGuiElement *cur, Vector pos, Direction dir)
{
	float dist = 0, smallDist = -1;

	AquariaGuiElement *gui = 0, *closest = 0;
	const float ch = 64;
	for (GuiElements::iterator i = guiElements.begin(); i != guiElements.end(); i++)
	{
		gui = (*i);
		if (gui != cur && gui->isGuiVisible() && gui->canDirMove && gui->hasInput())
		{
			int go = 0;
			Vector p1 = pos;
			Vector p2 = gui->getGuiPosition();

			if (dir == DIR_DOWN)
			{
				if (fabsf(p1.x - p2.x) < ch)
				{
					if (p2.y > p1.y) go = 1;
					p1.x = p2.x = 0;
				}
			}
			else if (dir == DIR_UP)
			{
				if (fabsf(p1.x - p2.x) < ch)
				{
					if (p2.y < p1.y) go = 1;
					p1.x = p2.x = 0;
				}
			}
			else if (dir == DIR_RIGHT)
			{
				if (fabsf(p1.y - p2.y) < ch)
				{
					if (p2.x > p1.x) go = 1;
					p1.y = p2.y = 0;
				}
			}
			else if (dir == DIR_LEFT)
			{
				if (fabsf(p1.y - p2.y) < ch)
				{
					if (p2.x < p1.x) go = 1;
					p1.y = p2.y = 0;
				}
			}
			else
			{
				continue;
			}

			if (go)
			{
				dist = (p1 - p2).getSquaredLength2D();

				if (smallDist == -1 || dist < smallDist)
				{
					closest = gui;
					smallDist = dist;
				}
			}
			else
			{
				continue;
			}
		}
	}

	return closest;
}

AquariaGuiElement *AquariaGuiElement::getClosestGuiElement(const Vector& pos)
{
	AquariaGuiElement *gui = 0, *closest = 0;
	float minlen = 0;
	for (GuiElements::iterator i = guiElements.begin(); i != guiElements.end(); i++)
	{
		gui = (*i);
		if (gui->isGuiVisible() && gui->hasInput())
		{
			Vector dist = gui->getGuiPosition() - pos;
			float len = dist.getSquaredLength2D();
			if(!closest || len < minlen)
			{
				closest = gui;
				minlen = len;
			}
		}
	}
	return closest;
}


AquariaGuiQuad::AquariaGuiQuad() : Quad(), AquariaGuiElement()
{
}

void AquariaGuiQuad::destroy()
{
	Quad::destroy();
	AquariaGuiElement::clean();
}

Vector AquariaGuiQuad::getGuiPosition()
{
	return getWorldPosition();
}

bool AquariaGuiQuad::isGuiVisible()
{
	return !isHidden() && alpha.x > 0 && alphaMod > 0 && renderQuad;
}

void AquariaGuiQuad::update(float dt)
{
	// super hacky
	if (hasInput())
	{
		Quad::update(dt);
	}
	else
	{
		updateMovement(dt);
		Quad::onUpdate(dt);
	}
}

void AquariaGuiQuad::onUpdate(float dt)
{
	updateMovement(dt);
	Quad::onUpdate(dt);
}

// Joystick input threshold at which we start sliding (0.0-1.0); must be
// less than updateMovement() threshold.
const float SLIDER_JOY_THRESHOLD = 0.39f;
// Initial delay before repeating for slider input (seconds).
const float SLIDER_REPEAT_DELAY = 0.4f;
// Scale factor for delay as repeats continue.
const float SLIDER_REPEAT_ACCEL = 0.8f;

AquariaSlider::AquariaSlider()
: Slider(90, 12, "gui/slider-bg", "gui/slider-fg"), AquariaGuiElement()
// len, grab radius
{
	inputTimer = inputDelay = 0;
	_hadInput = false;
}

void AquariaSlider::onUpdate(float dt)
{
	if (!hasInput())
	{
		inputTimer = inputDelay = 0;
		AquariaGuiElement::updateMovement(dt);
		RenderObject::onUpdate(dt);
	}
	else
	{
		if (!doSliderInput(dt))
			AquariaGuiElement::updateMovement(dt);
		Slider::onUpdate(dt);
	}
}

bool AquariaSlider::doSliderInput(float dt)
{
	if (!(core->mouse.position - this->position).isLength2DIn(5))
		return false;

	float inputAmount = 0;  // How much to adjust by?

	StateObject *obj = dsq->getTopStateObject();
	if (obj && obj->isActing(ACTION_MENULEFT, -1))
		inputAmount = -0.1f;
	else if (obj && obj->isActing(ACTION_MENURIGHT, -1))
		inputAmount = +0.1f;

	if (inputAmount != 0)
	{
		inputTimer += dt;
		if (inputTimer >= inputDelay)
		{
			float oldValue = value;
			setValue(value + inputAmount);
			if (value != oldValue)
				_hadInput = true;

			inputTimer = 0;
			if (inputDelay == 0)
				inputDelay = SLIDER_REPEAT_DELAY;
			else
				inputDelay *= SLIDER_REPEAT_ACCEL;
		}
		return true;
	}
	else
	{
		inputTimer = inputDelay = 0;
		return false;
	}
}

void AquariaSlider::destroy()
{
	Slider::destroy();
	AquariaGuiElement::clean();
}

Vector AquariaSlider::getGuiPosition()
{
	return getWorldPosition();
}

bool AquariaSlider::isGuiVisible()
{
	return !isHidden() && alpha.x > 0 && alphaMod > 0;
}

AquariaCheckBox::AquariaCheckBox()
: CheckBox(12, "gui/check-bg", "gui/check-fg", "Click"), AquariaGuiElement()
{
}

void AquariaCheckBox::onUpdate(float dt)
{
	AquariaGuiElement::updateMovement(dt);
	if (!hasInput())
	{
		RenderObject::onUpdate(dt);
	}
	else
	{
		CheckBox::onUpdate(dt);
	}
}

void AquariaCheckBox::destroy()
{
	CheckBox::destroy();
	AquariaGuiElement::clean();
}

Vector AquariaCheckBox::getGuiPosition()
{
	return getWorldPosition();
}

bool AquariaCheckBox::isGuiVisible()
{
	return !isHidden() && alpha.x > 0 && alphaMod > 0;
}


AquariaKeyConfig *AquariaKeyConfig::waitingForInput = 0;


AquariaKeyConfig::AquariaKeyConfig(const std::string &actionInputName, InputSetType inputSetType, int inputIdx)
: AquariaGuiElement(), RenderObject()
, actionInputName(actionInputName)
, inputSetType(inputSetType)
, inputIdx(inputIdx)
, actionSetIndex(0)
, rejectJoyAxis(false)
{
	bg = new Quad();
	bg2 = new Quad();
	if (inputSetType == INPUTSET_OTHER)
	{
		bg->setWidthHeight(40, 20);
		bg2->setWidthHeight(40, 20);
	}
	else
	{
		bg->setWidthHeight(90, 20);
		bg2->setWidthHeight(90, 20);
	}

	bg->color = Vector(0.5f, 0.5f, 0.5f);
	bg->alphaMod = 0;
	bg->position = Vector(0, -3);
	bg2->color = Vector(0.4f, 0.4f, 0.6f);
	bg2->alphaMod = 0;
	bg2->position = Vector(0, -3);
	addChild(bg, PM_POINTER);
	addChild(bg2, PM_POINTER);



	keyConfigFont = new TTFText(&dsq->fontArialSmallest);

	keyConfigFont->setAlign(ALIGN_CENTER);

	addChild(keyConfigFont, PM_POINTER);

	keyDown = false;

	toggleEnterKey(false);
}

void AquariaKeyConfig::destroy()
{
	AquariaGuiElement::clean();
	RenderObject::destroy();

	if (waitingForInput == this)
		waitingForInput = 0;
}

Vector AquariaKeyConfig::getGuiPosition()
{
	return getWorldPosition();
}

bool AquariaKeyConfig::isGuiVisible()
{
	return !isHidden() && alpha.x > 0 && alphaMod > 0;
}

void AquariaKeyConfig::toggleEnterKey(int on)
{
	if (on==1)
	{
		bg->color = Vector(0.75f, 0.75f, 0.75f);
		bg->alphaMod = 0.25f;
	}
	else if (on == 0)
	{
		bg->alphaMod = 0;
		bg->color = Vector(0.1f, 0.1f, 0.1f);
	}
	else
	{
		bg->alphaMod = 0.5f;
		bg->color = Vector(0.5f, 0.5f, 0.5f);
	}

}

void AquariaKeyConfig::onUpdate(float dt)
{
	static bool inLoop = false;

	if (inLoop) return;



	AquariaGuiElement::updateMovement(dt);

	RenderObject::onUpdate(dt);


	if (!hasInput() || alpha.x <= 0) return;

	if(actionSetIndex >= dsq->user.control.actionSets.size())
		return;

	ActionSet& as = dsq->user.control.actionSets[actionSetIndex];

	inLoop = true;

	unsigned int *k = 0;

	ActionInput *ai = 0;

	bool used = false;

	if (inputSetType != INPUTSET_OTHER)
	{
		ai = as.getActionInputByName(actionInputName);

		if (!ai)
		{
			exit_error("Could not find actionInput: " + actionInputName);
		}
		switch(inputSetType)
		{
		case INPUTSET_KEY:
			k = &ai->data.single.key[inputIdx];
		break;
		case INPUTSET_MOUSE:
			k = &ai->data.single.mse[inputIdx];
		break;
		case INPUTSET_JOY:
			k = &ai->data.single.joy[inputIdx];
		break;
		case INPUTSET_NONE:
		case INPUTSET_OTHER:
			k = 0;
		break;
		}
		used = k && *k;
	}


	if (inputSetType == INPUTSET_OTHER)
	{
		if (actionInputName == "s1ax")
			k = &as.joycfg.s1ax;
		else if (actionInputName == "s1ay")
			k = &as.joycfg.s1ay;
		else if (actionInputName == "s2ax")
			k = &as.joycfg.s2ax;
		else if (actionInputName == "s2ay")
			k = &as.joycfg.s2ay;
		used = k && int(*k) >= 0;
	}

	if(used)
		bg2->alphaMod = 0.3f;
	else
		bg2->alphaMod = 0;

	if (waitingForInput == this)
	{
		std::string s;
		s.reserve(6);
		s = "_";
		for (int i = 0; i < int(game->getTimer(5)); i++)
		{
			s += "_";
		}
		keyConfigFont->setText(s);
	}
	else
	{
		if (inputSetType != INPUTSET_OTHER)
		{
			keyConfigFont->setText(getInputCodeToUserString(*k, as.joystickID));
		}
		else
		{
			if(*k >= 0)
			{
				std::ostringstream os;
				os << (*k);
				keyConfigFont->setText(os.str());
			}
			else
			{
				keyConfigFont->setText(getInputCodeToUserString(0, as.joystickID));
			}
		}
	}

	if (waitingForInput == this)
	{
		switch(inputSetType)
		{
		case INPUTSET_OTHER:
		{
			if (k)
			{
				int ac = -1;
				bool clear = false;
				bool abort = false;
				for (int i = 0; i < KEY_MAXARRAY; i++)
				{
					if (core->getKeyState(i))
					{
						if(i == KEY_BACKSPACE || i == KEY_DELETE)
							clear = true;
						else if (i == KEY_ESCAPE)
							abort = true;
						else
						{
							switch(i)
							{
							#define K(k) case k: ac = i-k; break
								K(KEY_0);
								K(KEY_1);
								K(KEY_2);
								K(KEY_3);
								K(KEY_4);
								K(KEY_5);
								K(KEY_6);
								K(KEY_7);
								K(KEY_8);
								K(KEY_9);
							#undef K
							}
						}

						while (game->getKeyState(i))
						{
							dsq->run(0.1f, true);
						}
					}
				}

				if(ac < 0)
				{
					Joystick *j = core->getJoystick(as.joystickID);
					if(j)
						for(int i = 0; i < MAX_JOYSTICK_AXIS; ++i)
						{
							float ax = j->getAxisUncalibrated(i);
							if(fabsf(ax) > JOY_AXIS_THRESHOLD)
							{
								ac = i;
								while (fabsf(j->getAxisUncalibrated(i)) > JOY_AXIS_THRESHOLD)
									dsq->run(0.1f, true);
								break;
							}
						}
				}

				if(ac >= 0 || abort || clear)
				{
					toggleEnterKey(0);
					waitingForInput = 0;
					AquariaGuiElement::canDirMoveGlobal = true;
					if(!abort)
					{
						if(clear || ac == *k)
							*k = -1;
						else
							*k = ac;
					}
				}
			}
		}
		break;
		case INPUTSET_KEY:
		{
			for (int i = 0; i < KEY_MAXARRAY; i++)
			{
				if (core->getKeyState(i))
				{
					if(*k == i) // clear key if pressed again
						*k = 0;
					else if(i != KEY_ESCAPE)
					{
						if (i == KEY_DELETE || i == KEY_BACKSPACE)
							*k = 0;
						else
							*k = i;
					}

					while (game->getKeyState(i))
					{
						dsq->run(0.1f, true);
					}

					toggleEnterKey(0);
					waitingForInput = 0;
					AquariaGuiElement::canDirMoveGlobal = true;
					break;
				}
			}
		}
		break;
		case INPUTSET_MOUSE:
		{
			bool clear = false;
			bool abort = false;
			unsigned int ac = 0;
			if (core->getKeyState(KEY_DELETE) || core->getKeyState(KEY_BACKSPACE))
			{
				while(core->getKeyState(KEY_DELETE) || core->getKeyState(KEY_BACKSPACE))
					dsq->run(0.1f, true);
				clear = true;
			}
			else if(core->getKeyState(KEY_ESCAPE))
			{
				while(core->getKeyState(KEY_ESCAPE))
					dsq->run(0.1f, true);
				abort = true;
			}
			else if(dsq->mouse.rawButtonMask)
			{
				MouseButtons btns = dsq->mouse.buttons;

				while(dsq->mouse.rawButtonMask)
					dsq->run(0.1f, true);

				if(btns.left)
					ac = MOUSE_BUTTON_LEFT;
				else if(btns.right)
					ac = MOUSE_BUTTON_RIGHT;
				else if(btns.middle)
					ac = MOUSE_BUTTON_MIDDLE;
				else
				{
					for(unsigned i = 0; i < mouseExtraButtons; ++i)
						if(btns.extra[i])
						{
							ac = MOUSE_BUTTON_EXTRA_START+i;
							break;
						}
				}
			}

			if(ac || clear || abort)
			{
				toggleEnterKey(0);
				waitingForInput = 0;
				AquariaGuiElement::canDirMoveGlobal = true;

				if(!abort)
				{
					if(clear || *k == ac) // clear key if pressed again
						*k = 0;
					else
						*k = ac;
				}
			}
		}
		break;
		case INPUTSET_JOY:
		{
			size_t ac = 0;
			bool clear = false;
			bool abort = false;
			if (core->getKeyState(KEY_DELETE) || core->getKeyState(KEY_BACKSPACE))
			{
				clear = true;
			}
			else if(core->getKeyState(KEY_ESCAPE))
			{
				abort = true;
				while(core->getKeyState(KEY_ESCAPE))
					dsq->run(0.1f, true);
			}
			else
			{
				Joystick *j = core->getJoystick(as.joystickID);
				if(j)
				{
					for (size_t i = 0; i < MAX_JOYSTICK_BTN; i++)
						if (j->getButton(i))
						{
							ac = JOY_BUTTON_0 + i;
							while (j->getButton(i))
								dsq->run(0.1f, true); // skip recursion check; we're already in the menu so this would always warn
							break;
						}

					if(!ac)
						for(size_t i = 0; i < MAX_JOYSTICK_AXIS; ++i)
						{
							float ax = j->getAxisUncalibrated(i);
							if(fabsf(ax) > JOY_AXIS_THRESHOLD)
							{
								ac = (ax < 0.0f ? JOY_AXIS_0_NEG : JOY_AXIS_0_POS) + i;
								while (fabsf(j->getAxisUncalibrated(i)) > JOY_AXIS_THRESHOLD)
									dsq->run(0.1f, true);
								break;
							}
						}

					if(!ac)
						for(size_t i = 0; i < MAX_JOYSTICK_HATS; ++i)
						{
							JoyHatDirection hd = j->getHat(i);
							if(hd != JOY_HAT_DIR_CENTERED)
							{
								ac = joyHatToActionButton(i, hd);
								while (j->getHat(i) != JOY_HAT_DIR_CENTERED)
									dsq->run(0.1f, true);
								break;
							}
						}
				}
				else
					clear = true;
			}

			if(abort || ac || clear)
			{
				toggleEnterKey(0);
				waitingForInput = 0;
				AquariaGuiElement::canDirMoveGlobal = true;

				if(rejectJoyAxis && (
					(ac >= JOY_AXIS_0_POS && ac < JOY_AXIS_END_POS)
				 || (ac >= JOY_AXIS_0_NEG && ac < JOY_AXIS_END_NEG)
				))
				{
					dsq->sound->playSfx("denied");
					abort = true;
				}

				if(!abort)
				{
					if(clear || *k == ac) // clear key if pressed again
						*k = 0;
					else
						*k = ac;
				}
			}
		}
		break;
		case INPUTSET_NONE:
		break;
		}
	}

	Vector p = bg->getWorldPosition();

	if (waitingForInput == this || (!waitingForInput &&
		(core->mouse.position.x > (p.x - bg->getWidth()*0.5f) && core->mouse.position.x < (p.x + bg->getWidth()*0.5f)
		 && core->mouse.position.y > (p.y - bg->getHeight()*0.5f) && core->mouse.position.y < (p.y + bg->getHeight()*0.5f)
		 )))
	{
		if (waitingForInput != this)
		{
			toggleEnterKey(-1);
		}


		if (!keyDown && (core->mouse.buttons.left || core->mouse.buttons.right))
		{
			keyDown = true;
		}
		else if (keyDown && (!core->mouse.buttons.left && !core->mouse.buttons.right ))
		{
			keyDown = false;

			if (waitingForInput == this)
			{
				waitingForInput = 0;
				toggleEnterKey(0);
				AquariaGuiElement::canDirMoveGlobal = true;
			}
			else
			{
				waitingForInput = this;
				toggleEnterKey(1);
				AquariaGuiElement::canDirMoveGlobal = false;
			}
		}
	}
	else
	{
		toggleEnterKey(0);
		keyDown = false;
	}

	inLoop = false;
}

void AquariaKeyConfig::setActionSetIndex(size_t idx)
{
	actionSetIndex = idx;
}

void AquariaKeyConfig::setRejectJoyAxis(bool b)
{
	rejectJoyAxis = b;
}

AquariaMenuItem::AquariaMenuItem() : Quad(), ActionMapper(), AquariaGuiElement()
{
	quad = glow = 0;
	int sz = 20;

	shareAlpha = 0;

	font = 0;

	font = new BitmapText(dsq->font);
	font->setFontSize(sz);
	font->position = Vector(0, -sz/2, 0);
	addChild(font, PM_POINTER, RBP_ON);

	glowFont = new BitmapText(dsq->font);
	glowFont->setFontSize(sz);
	glowFont->position = Vector(0, -sz/2, 0);
	glowFont->setBlendType(BLEND_ADD);
	glowFont->alpha = 0;

	addChild(glowFont, PM_POINTER, RBP_OFF);



	width = 0;
	height = 0;
	highlighted = false;

	cull = false;
	followCamera = 1;
	addAction(MakeFunctionEvent(AquariaMenuItem, onClick), MOUSE_BUTTON_LEFT, 0);
	addAction(MakeFunctionEvent(AquariaMenuItem, onClick), MOUSE_BUTTON_RIGHT, 0);

	renderQuad = false;
}

void AquariaMenuItem::destroy()
{
	setFocus(false);
	Quad::destroy();
	AquariaGuiElement::clean();
}

Vector AquariaMenuItem::getGuiPosition()
{
	return getWorldPosition();
}

bool AquariaMenuItem::isGuiVisible()
{
	return !isHidden() && alpha.x > 0 && alphaMod > 0;
}

void AquariaMenuItem::useSound(const std::string &tex)
{
	useSfx = tex;
}

bool AquariaMenuItem::useQuad(const std::string &tex)
{
	if (quad)
	{
		debugLog("trying to call useQuad twice on the same object");
		return true;
	}
	quad = new Quad;
	bool good = quad->setTexture(tex);
	addChild(quad, PM_POINTER);
	return good;
}

void AquariaMenuItem::useGlow(const std::string &tex, int w, int h)
{
	if (glow)
	{
		debugLog("trying to call useGlow twice on the same object");
		return;
	}
	glow = new Quad;
	glow->setTexture(tex);
	glow->setWidthHeight(w, h);
	glow->setBlendType(BLEND_ADD);
	glow->alpha = 0;
	addChild(glow, PM_POINTER);
}

void AquariaMenuItem::onClick()
{
	if (hasInput() && highlighted && game->getInGameMenu()->menuSelectDelay == 0)
	{
		game->getInGameMenu()->menuSelectDelay = MENUSELECTDELAY;

		if (!useSfx.empty())
			dsq->sound->playSfx(useSfx);
		else
			dsq->playMenuSelectSfx();

		event.call();
	}
}

void AquariaMenuItem::setLabel(const std::string &label)
{
	font->setText(label);
	glowFont->setText(label);
}

void AquariaMenuItem::toggleHighlight(bool state)
{
	highlighted = state;
	if (highlighted)
	{
		if (glow)
		{
			glow->alpha.interpolateTo(0.3f, 0.2f);
		}
		else
		{
			glowFont->alpha.interpolateTo(0.3f, 0.2f);
		}

	}
	else
	{
		if (glow)
			glow->alpha.interpolateTo(0, 0.2f);
		else
			glowFont->alpha.interpolateTo(0, 0.2f);
	}

	onToggleHighlight(highlighted);
}

void AquariaMenuItem::onUpdate(float dt)
{
	AquariaGuiElement::updateMovement(dt);

	if (font)
	{
		font->alpha = this->alpha;
	}
	Quad::onUpdate(dt);

	if (shareAlpha)
	{
		if (quad) quad->alphaMod = alpha.x;
		if (glow) glow->alphaMod = alpha.x;
	}

	if (this->alpha.x < 1.0) return;

	if (hasInput())
		ActionMapper::onUpdate(dt);

	if (quad)
	{
		quad->alpha.x = alpha.x;
	}


	if (hasInput())
	{
		if (alpha.x == 1)
		{
			bool on = true;


			if (isCursorInMenuItem())
			{
				if (!highlighted)
					toggleHighlight(true);
			}
			else
				on = false;

			if (!on && highlighted)
				toggleHighlight(false);
		}
		else
		{
			if (highlighted)
				toggleHighlight(false);
		}
	}
}

bool AquariaMenuItem::isCursorInMenuItem()
{
	Vector v = dsq->mouse.position;
	int hw = font->getWidthOnScreen()/2;
	int hh = 20;
	if (hw < 64)
		hw = 64;
	if (glow)
	{
		hw = glow->getWidth()/2.0f;
		hh = glow->getHeight()/2.0f;
	}
	if (rotation.z == 90)
	{
		std::swap(hw, hh);
	}
	Vector pos = getWorldPosition();
	if (v.y > pos.y - hh && v.y < pos.y + hh)
	{
		if (v.x > pos.x - hw && v.x < pos.x + hw)
		{
			return true;
		}
	}
	return false;
}


AquariaButton::AquariaButton(const std::string texbase, TTFFont *font)
: activeColor(1,1,1), activeAlpha(0.5f)
, inactiveColor(0,0,0), inactiveAlpha(0.5f)
, buttonlabel(new TTFText(font))
, _texbase(texbase), pressed(0), lastpressed(0)
{
	useQuad(texbase + "-button-up");
	addChild(buttonlabel, PM_POINTER);
	buttonlabel->setAlign(ALIGN_CENTER);
	buttonlabel->position = Vector(0, 3);
}

void AquariaButton::goUp()
{
	quad->setTexture(_texbase + "-button-up");
	buttonlabel->position = Vector(0, 3);
}

void AquariaButton::goDown()
{
	quad->setTexture(_texbase + "-button-down");
	buttonlabel->position = Vector(0, 7);
}

void AquariaButton::action(int actionID, int state, int source, InputDevice device)
{
	if(actionID == ACTION_PRIMARY)
	{
		if(state)
			pressed |= 1;
		else
			pressed &= ~1;
	}
	else if(actionID == ACTION_SECONDARY)
	{
		if(state)
			pressed |= 2;
		else
			pressed &= ~2;
	}
}

void AquariaButton::onUpdate(float dt)
{
	AquariaMenuItem::onUpdate(dt);

	/*if(pressed != lastpressed)
	{
		if(pressed)
			goDown();
		else
			goUp();
		lastpressed = pressed;
	}*/
}

void AquariaButton::onToggleHighlight(bool on)
{
	if(on)
	{
		quad->color.interpolateTo(activeColor, 0.1f);
		quad->alpha.interpolateTo(activeAlpha, 0.1f);
	}
	else
	{
		quad->color.interpolateTo(inactiveColor, 0.1f);
		quad->alpha.interpolateTo(inactiveAlpha, 0.1f);
	}
}

void AquariaButton::setButtonLabel(const std::string& s)
{
	buttonlabel->setText(s);
}

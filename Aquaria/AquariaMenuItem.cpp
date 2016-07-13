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
	hasFocus = false;

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

void AquariaGuiElement::setFocus(bool v)
{
	hasFocus = v;

	if (v)
	{
		currentFocus = this;
		if (dsq->inputMode == INPUT_JOYSTICK)
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

	if (hasFocus && isGuiVisible() && canDirMove && canDirMoveGlobal && hasInput())
	{



		if (guiMoveTimer > 0)
		{
			guiMoveTimer -= dt;
			if (guiMoveTimer < 0) guiMoveTimer = 0;
		}

		if (guiMoveTimer==0)
		{
			Direction dir = DIR_NONE;
			Vector p;
			for(size_t i = 0; i < core->joysticks.size(); ++i)
				if(Joystick *j = core->joysticks[i])
					if(j->isEnabled())
					{
						p = core->joysticks[i]->position;
						if(!p.isLength2DIn(0.4f))
							break;
					}
			if (!p.isLength2DIn(0.4f))
			{
				if (fabsf(p.x) > fabsf(p.y))
				{
					if (p.x > 0)
						dir = DIR_RIGHT;
					else
						dir = DIR_LEFT;
				}
				else
				{
					if (p.y > 0)
						dir = DIR_DOWN;
					else
						dir = DIR_UP;
				}
			}
			else
			{
				StateObject *obj = dsq->getTopStateObject();
				if (obj)
				{
					if (obj->isActing(ACTION_MENULEFT))			dir = DIR_LEFT;
					else if (obj->isActing(ACTION_MENURIGHT))	dir = DIR_RIGHT;
					else if (obj->isActing(ACTION_MENUUP))		dir = DIR_UP;
					else if (obj->isActing(ACTION_MENUDOWN))	dir = DIR_DOWN;
				}
			}

			if (dir == DIR_NONE) return;

			const float moveDelay = 0.2;

			AquariaGuiElement *gui = 0;
			if (dir > DIR_NONE && dir < DIR_MAX)
			{
				gui = dirMove[dir];
				if (gui)
				{
					gui->setFocus(true);



					guiMoveTimer = moveDelay;
				}
			}

			if (!gui)
			{
				debugLog("updating closest");
				int smallDist = -1, dist = 0;

				AquariaGuiElement *gui = 0, *closest = 0;
				int ch = 64;
				for (GuiElements::iterator i = guiElements.begin(); i != guiElements.end(); i++)
				{
					gui = (*i);
					if (gui != this && gui->isGuiVisible() && gui->canDirMove)
					{
						int go = 0;
						Vector p1 = getGuiPosition();
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

				if (closest)
				{
					closest->setFocus(true);

					guiMoveTimer = moveDelay;
				}
			}
		}
	}
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
const float SLIDER_JOY_THRESHOLD = 0.39;
// Initial delay before repeating for slider input (seconds).
const float SLIDER_REPEAT_DELAY = 0.4;
// Scale factor for delay as repeats continue.
const float SLIDER_REPEAT_ACCEL = 0.8;

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

	float inputAmount;  // How much to adjust by?

	Vector jpos;
	for(size_t i = 0; i < core->joysticks.size(); ++i)
		if(Joystick *j = core->joysticks[i])
			if(j->isEnabled())
			{
				jpos = core->joysticks[i]->position;
				if(fabsf(jpos.x) > SLIDER_JOY_THRESHOLD)
					break;
			}

	StateObject *obj = dsq->getTopStateObject();
	if (jpos.x <= -SLIDER_JOY_THRESHOLD)
		inputAmount = -0.1f;
	else if (jpos.x >= SLIDER_JOY_THRESHOLD)
		inputAmount = +0.1f;
	else if (obj && obj->isActing(ACTION_MENULEFT))
		inputAmount = -0.1f;
	else if (obj && obj->isActing(ACTION_MENURIGHT))
		inputAmount = +0.1f;
	else
		inputAmount = 0;

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
: AquariaGuiElement(), RenderObject(), actionInputName(actionInputName), inputSetType(inputSetType), inputIdx(inputIdx)
{

	bg = new Quad();
	if (inputSetType == INPUTSET_OTHER)
		bg->setWidthHeight(40, 20);
	else
		bg->setWidthHeight(90, 20);

	bg->color = Vector(0.5, 0.5, 0.5);
	bg->alphaMod = 0;
	addChild(bg, PM_POINTER);



	keyConfigFont = new TTFText(&dsq->fontArialSmallest);

	keyConfigFont->setAlign(ALIGN_CENTER);

	addChild(keyConfigFont, PM_POINTER);


	keyDown = false;
	acceptEsc = false;
	joystickID = 0;

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
		bg->color = Vector(0.75, 0.75, 0.75);
		bg->alphaMod = 0.25;
	}
	else if (on == 0)
	{
		bg->alphaMod = 0;
		bg->color = Vector(0.1, 0.1, 0.1);
	}
	else
	{
		bg->alphaMod = 0.5;
		bg->color = Vector(0.5, 0.5, 0.5);
	}

}

void AquariaKeyConfig::onUpdate(float dt)
{
	static bool inLoop = false;

	if (inLoop) return;



	AquariaGuiElement::updateMovement(dt);

	RenderObject::onUpdate(dt);


	if (!hasInput() || alpha.x <= 0) return;

	inLoop = true;

	int *k = 0;

	ActionInput *ai = 0;

	if (inputSetType != INPUTSET_OTHER)
	{
		ai = dsq->user.control.actionSet.getActionInputByName(actionInputName);

		if (!ai)
		{
			exit_error("Could not find actionInput: " + actionInputName);
		}
		switch(inputSetType)
		{
		case INPUTSET_KEY:
			k = &ai->key[inputIdx];
		break;
		case INPUTSET_MOUSE:
			k = &ai->mse[inputIdx];
		break;
		case INPUTSET_JOY:
			k = &ai->joy[inputIdx];
		break;
		default:
			k = 0;
		break;
		}
	}

	int *value = 0;

	if (inputSetType == INPUTSET_OTHER)
	{
		if (actionInputName == "s1ax")
			value = &dsq->user.control.s1ax;
		else if (actionInputName == "s1ay")
			value = &dsq->user.control.s1ay;
		else if (actionInputName == "s2ax")
			value = &dsq->user.control.s2ax;
		else if (actionInputName == "s2ay")
			value = &dsq->user.control.s2ay;
	}

	if (waitingForInput == this)
	{
		std::string s;
		s = "_";
		for (int i = 0; i < int(dsq->game->getTimer(5)); i++)
		{
			s += "_";
		}
		keyConfigFont->setText(s);
	}
	else
	{
		if (k)
		{
			keyConfigFont->setText(getInputCodeToUserString(*k, joystickID));
		}
		else if (value)
		{
			std::ostringstream os;
			os << (*value);
			keyConfigFont->setText(os.str());
		}
	}

	if (waitingForInput == this)
	{
		switch(inputSetType)
		{
		case INPUTSET_OTHER:
		{
			if (value)
			{
				for (int i = 0; i < KEY_MAXARRAY; i++)
				{
					if (core->getKeyState(i))
					{
						if (i != KEY_ESCAPE)
						{
							if (i >= KEY_0 && i <= KEY_9)
							{
								*value = i-KEY_0;
							}
						}

						while (dsq->game->getKeyState(i))
						{
							dsq->run(0.1);
						}

						toggleEnterKey(0);
						waitingForInput = 0;
						AquariaGuiElement::canDirMoveGlobal = true;
						break;
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
					else if(acceptEsc || i != KEY_ESCAPE)
					{
						if (i == KEY_DELETE || i == KEY_BACKSPACE)
							*k = 0;
						else
							*k = i;
					}

					while (dsq->game->getKeyState(i))
					{
						dsq->run(0.1f);
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
			int ac = 0;
			if (core->getKeyState(KEY_DELETE) || core->getKeyState(KEY_BACKSPACE))
			{
				clear = true;
			}
			else if(core->getKeyState(KEY_ESCAPE))
			{
				// do nothing
			}
			else if(dsq->mouse.rawButtonMask)
			{
				MouseButtons btns = dsq->mouse.buttons;

				while(dsq->mouse.rawButtonMask)
					dsq->run(0.1f);

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

			if(ac || clear)
			{
				toggleEnterKey(0);
				waitingForInput = 0;
				AquariaGuiElement::canDirMoveGlobal = true;

				if(clear || *k == ac) // clear key if pressed again
					*k = 0;
				else
					*k = ac;
			}
		}
		break;
		case INPUTSET_JOY:
		{
			int ac = 0;
			bool clear = false;
			if (core->getKeyState(KEY_DELETE) || core->getKeyState(KEY_BACKSPACE))
			{
				clear = true;
			}
			else if(core->getKeyState(KEY_ESCAPE))
			{
				// do nothing
			}
			else
			{
				Joystick *j = core->joysticks[joystickID];
				if(j)
				{
					for (int i = 0; i < MAX_JOYSTICK_BTN; i++)
						if (j->getButton(i))
						{
							ac = JOY_BUTTON_0 + i;
							while (j->getButton(i))
								dsq->run(0.1f);
							break;
						}

					if(!ac)
						for(int i = 0; i < MAX_JOYSTICK_AXIS; ++i)
						{
							float ax = j->getAxisUncalibrated(i);
							if(fabsf(ax) > JOY_AXIS_THRESHOLD)
							{
								ac = (ax < 0.0f ? JOY_AXIS_0_NEG : JOY_AXIS_0_POS) + i;
								while (fabsf(j->getAxisUncalibrated(i)) > JOY_AXIS_THRESHOLD)
									dsq->run(0.1f);
								break;
							}
						}
				}
				else
					clear = true;
			}

			if(ac || clear)
			{
				toggleEnterKey(0);
				waitingForInput = 0;
				AquariaGuiElement::canDirMoveGlobal = true;

				if(clear || *k == ac) // clear key if pressed again
					*k = 0;
				else
					*k = ac;
			}
		}
		break;
		}
	}

	Vector p = getWorldPosition();

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

void AquariaKeyConfig::setAcceptEsc(bool a)
{
	acceptEsc = a;
}

AquariaMenuItem::AquariaMenuItem() : Quad(), ActionMapper(), AquariaGuiElement()
{
	quad = glow = 0;
	choice = -1;
	ability = 0;
	xmlItem = 0;
	int sz = 20;

	shareAlpha = 0;

	font = 0;

	font = new BitmapText(&dsq->font);
	font->setFontSize(sz);
	font->position = Vector(0, -sz/2, 0);
	addChild(font, PM_POINTER, RBP_ON);

	glowFont = new BitmapText(&dsq->font);
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
			glow->alpha.interpolateTo(0.3, 0.2);
		}
		else
		{
			glowFont->alpha.interpolateTo(0.3, 0.2);
		}

	}
	else
	{
		if (glow)
			glow->alpha.interpolateTo(0, 0.2);
		else
			glowFont->alpha.interpolateTo(0, 0.2);
	}

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

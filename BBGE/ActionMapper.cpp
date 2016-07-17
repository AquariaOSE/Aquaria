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
*/

#include "ActionMapper.h"
#include "Core.h"


ActionMapper::ActionMapper()
{
	cleared = false;
	inputEnabled = true;
	inUpdate = false;
}

ActionMapper::~ActionMapper()
{
	clearCreatedEvents();
}

ActionData *ActionMapper::getActionDataByIDAndSource(int actionID, int source)
{
	for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); ++i)
	{
		if (i->id == actionID && i->source == source)
			return &(*i);
	}
	return 0;
}

bool ActionMapper::isActing(int actionID, int source)
{
	if(source < 0)
	{
		for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); ++i)
		{
			ActionData& ad = *i;
			if(ad.id == actionID)
				for (ButtonList::iterator ii = ad.buttonList.begin(); ii != ad.buttonList.end(); ++ii)
					if (keyDownMap[*ii])
						return true;
		}
		return false;
	}

	ActionData *ad = getActionDataByIDAndSource(actionID, source);
	if (ad)
	{
		ButtonList::iterator i = ad->buttonList.begin();
		for (; i != ad->buttonList.end(); i++)
		{
			if (keyDownMap[(*i)])
				return true;
		}
	}
	return false;
}

void ActionMapper::addAction(int actionID, int k, int source)
{
	ActionData *ad = getActionDataByIDAndSource(actionID, source);

	if (!ad)
	{
		ActionData data;
		data.id = actionID;
		data.source = source;
		actionData.push_back(data);
		ad = &actionData.back();
	}

	if (ad)
	{
		if(std::find(ad->buttonList.begin(), ad->buttonList.end(), k) == ad->buttonList.end())
			ad->buttonList.push_back(k);
		keyDownMap[k] = core->getKeyState(k);
	}
}

void ActionMapper::addAction(Event *event, int k, int state)
{
	ActionData data;
	data.event = event;
	data.state = state;
	data.buttonList.push_back(k);
	actionData.push_back(data);

	keyDownMap[k] = core->getKeyState(k);
}

Event* ActionMapper::addCreatedEvent(Event *event)
{
	for (int i = 0; i < createdEvents.size(); i++)
	{
		if (createdEvents[i] == event)
			return event;
	}
	createdEvents.push_back(event);
	return event;
}

void ActionMapper::clearCreatedEvents()
{
	for (int i = 0; i < createdEvents.size(); i++)
	{
		delete createdEvents[i];
	}
	createdEvents.clear();
}

void ActionMapper::enableInput()
{
	inputEnabled = true;
}

void ActionMapper::disableInput()
{
	inputEnabled = false;
}

bool ActionMapper::pollAction(int actionID, int source)
{
	if(source < 0)
	{
		for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); i++)
			if(i->id == actionID && _pollActionData(*i))
				return true;
		return false;
	}

	ActionData *ad = getActionDataByIDAndSource(actionID, source);
	return ad && _pollActionData(*ad);
}

bool ActionMapper::_pollActionData(const ActionData& ad)
{
	const ButtonList& blist = ad.buttonList;
	for (ButtonList::const_iterator j = blist.begin(); j != blist.end(); j++)
		if (getKeyState((*j)))
			return true;
	return false;
}


bool ActionMapper::getKeyState(int k)
{
	if(!k)
		return false;

	bool keyState = false;
	if (k >= 0 && k < KEY_MAXARRAY)
	{
		keyState = (core->getKeyState(k));
	}
	else if (k == MOUSE_BUTTON_LEFT)
	{
		keyState = (core->mouse.buttons.left == DOWN);
	}
	else if (k == MOUSE_BUTTON_RIGHT)
	{
		keyState = (core->mouse.buttons.right == DOWN);
	}
	else if (k == MOUSE_BUTTON_MIDDLE)
	{
		keyState = (core->mouse.buttons.middle == DOWN);
	}
	else if (k >= MOUSE_BUTTON_EXTRA_START && k < MOUSE_BUTTON_EXTRA_START+mouseExtraButtons)
	{
		keyState  = core->mouse.buttons.extra[k - MOUSE_BUTTON_EXTRA_START];
	}
	else if (k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
	{
		int v = k - JOY_BUTTON_0;

		for(size_t i = 0; i < core->getNumJoysticks(); ++i)
			if(Joystick *j = core->getJoystick(i))
				if(j->isEnabled())
					if( ((keyState = j->getButton(v))) )
						break;
	}
	else if (k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		int v = k - JOY_AXIS_0_POS;

		for(size_t i = 0; i < core->getNumJoysticks(); ++i)
			if(Joystick *j = core->getJoystick(i))
				if(j->isEnabled())
				{
					float ax = j->getAxisUncalibrated(v);
					keyState = ax > JOY_AXIS_THRESHOLD;
					if(keyState)
						break;
				}
	}
	else if (k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		int v = k - JOY_AXIS_0_NEG;

		for(size_t i = 0; i < core->getNumJoysticks(); ++i)
			if(Joystick *j = core->getJoystick(i))
				if(j->isEnabled())
				{
					float ax = j->getAxisUncalibrated(v);
					keyState = ax < -JOY_AXIS_THRESHOLD;
					if(keyState)
						break;
				}
	}

	return keyState;
}

bool ActionMapper::getKeyState(int k, int source)
{
	if(!k)
		return false;

	ActionSet& as = (*core->pActionSets)[source];

	bool keyState = false;
	if (k >= 0 && k < KEY_MAXARRAY)
	{
		keyState = core->getKeyState(k) && as.hasKey(k);
	}
	else if (k == MOUSE_BUTTON_LEFT)
	{
		keyState = core->mouse.buttons.left == DOWN && as.hasMouse(k);
	}
	else if (k == MOUSE_BUTTON_RIGHT)
	{
		keyState = core->mouse.buttons.right == DOWN && as.hasMouse(k);
	}
	else if (k == MOUSE_BUTTON_MIDDLE)
	{
		keyState = core->mouse.buttons.middle == DOWN && as.hasMouse(k);
	}
	else if (k >= MOUSE_BUTTON_EXTRA_START && k < MOUSE_BUTTON_EXTRA_START+mouseExtraButtons)
	{
		keyState  = core->mouse.buttons.extra[k - MOUSE_BUTTON_EXTRA_START] && as.hasMouse(k);
	}
	else if (k >= JOY_BUTTON_0 && k < JOY_BUTTON_END)
	{
		Joystick *j = core->getJoystick(as.joystickID);
		if(j && j->isEnabled())
			keyState = j->getButton( - JOY_BUTTON_0);
	}
	else if (k >= JOY_AXIS_0_POS && k < JOY_AXIS_END_POS)
	{
		Joystick *j = core->getJoystick(as.joystickID);
		if(j && j->isEnabled())
		{
			float ax = j->getAxisUncalibrated(k - JOY_AXIS_0_POS);
			keyState = ax > JOY_AXIS_THRESHOLD;
		}
	}
	else if (k >= JOY_AXIS_0_NEG && k < JOY_AXIS_END_NEG)
	{
		Joystick *j = core->getJoystick(as.joystickID);
		if(j && j->isEnabled())
		{
			float ax = j->getAxisUncalibrated(k - JOY_AXIS_0_NEG);
			keyState = ax < -JOY_AXIS_THRESHOLD;
		}
	}

	return keyState;
}

void ActionMapper::onUpdate (float dt)
{
	if (inUpdate) return;
	inUpdate = true;

	if (cleared) cleared = false;
	ActionDataSet::iterator i;
	KeyDownMap oldKeyDownMap = keyDownMap;
	for (i = actionData.begin(); i != actionData.end(); ++i)
	{
		ButtonList::iterator j;
		j = i->buttonList.begin();
		for (; j != i->buttonList.end(); j++)
		{
			int k = (*j);
			ActionData *ad = &(*i);
			int keyState = ad->source < 0
				? getKeyState(k) // any source goes
				: getKeyState(k, ad->source); // specific source

			if (keyState != oldKeyDownMap[k])
			{
				keyDownMap[k] = keyState;
				if (inputEnabled)
				{
					if (ad->event)
					{
						if (ad->state==-1 || keyState == ad->state)
						{
							ad->event->act();
						}
					}
					else
					{
						action(ad->id, keyState, ad->source);
					}
					if (core->loopDone) goto out;
				}
				if (cleared) { cleared = false; goto out; } // actionData has been cleared, stop iteration
			}
		}
	}

out:
	inUpdate = false;

}

void ActionMapper::clearActions()
{
	cleared = true;
	keyDownMap.clear();
	actionData.clear();
}

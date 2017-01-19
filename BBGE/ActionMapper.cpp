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


InputDevice getDeviceForActionbutton(int k)
{
	if(k <= KEY_MAXARRAY)
		return INPUT_KEYBOARD;
	if(k < MOUSE_BUTTON_EXTRA_END)
		return INPUT_MOUSE;
	return INPUT_JOYSTICK;
}

ActionMapper::ActionMapper()
{
	cleared = false;
	inputEnabled = true;
	inUpdate = false;
	//memset(keyStatus, 0, sizeof(keyStatus));
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
					if (getKeyState(*ii, source))
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
			if (getKeyState(*i, source))
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
		//keyStatus[k] = core->getKeyState(k);
	}
}

void ActionMapper::addAction(Event *event, int k, int state)
{
	ActionData data;
	data.event = event;
	data.state = state;
	data.buttonList.push_back(k);
	actionData.push_back(data);

	//keyStatus[k] = core->getKeyState(k);
}

Event* ActionMapper::addCreatedEvent(Event *event)
{
	for (size_t i = 0; i < createdEvents.size(); i++)
	{
		if (createdEvents[i] == event)
			return event;
	}
	createdEvents.push_back(event);
	return event;
}

void ActionMapper::clearCreatedEvents()
{
	for (size_t i = 0; i < createdEvents.size(); i++)
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

/*
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
*/

void ActionMapper::onUpdate (float dt)
{
	if (inUpdate)
		return;

	inUpdate = true;
	cleared = false;

	for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); ++i)
	{
		for (ButtonList::iterator j = i->buttonList.begin(); j != i->buttonList.end(); j++)
		{
			const int k = (*j);
			const ActionData *ad = &(*i);
			const bool keyChanged = isKeyChanged(k, ad->source);

			if (keyChanged)
			{
				bool keyState = getKeyState(k, ad->source);
				if (inputEnabled)
				{
					if (ad->event)
					{
						if (ad->state==-1 || keyState == !!ad->state)
						{
							ad->event->act();
						}
					}
					else
					{
						action(ad->id, keyState, ad->source, getDeviceForActionbutton(k));
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

bool ActionMapper::getKeyState(int k, int sourceID)
{
	if(sourceID < 0)
		return getKeyState(k);
	return core->getActionStatus(sourceID)->getKeyState(k);
}

bool ActionMapper::getKeyState(int k)
{
	// all including sentinel
	const int m = core->getMaxActionStatusIndex();
	for(int i = -1; i <= m; ++i)
		if(core->getActionStatus(i)->getKeyState(k))
			return true;
	return false;
}

void ActionMapper::clearActions()
{
	cleared = true;
	actionData.clear();
}

bool ActionMapper::isKeyChanged(int k, int sourceID)
{
	if(sourceID < 0)
		return isKeyChanged(k);
	return core->getActionStatus(sourceID)->isKeyChanged(k);
}

bool ActionMapper::isKeyChanged(int k)
{
	// all including sentinel
	const int m = core->getMaxActionStatusIndex();
	for(int i = -1; i <= m; ++i)
		if(core->getActionStatus(i)->isKeyChanged(k))
			return true;
	return false;
}

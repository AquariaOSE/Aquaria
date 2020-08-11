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

typedef std::vector<unsigned> ButtonList;

// TODO: clean this up, make single vector of tagged unions, sorted by key
struct LegacyActionData
{
	LegacyActionData()
		: id(-1), state(-1)
		, event(0)
	{
	}

	int id; // -1 if no associated event
	int state; // -1, 0, or 1
	Event *event;
	ButtonList buttonList;
};

enum ActionState
{
	ACSTATE_ACTIVE = 0x1,    // action is active right now (button is held down)
	ACSTATE_ACCEPTED = 0x2   // handle that action when it comes in
};

ActionMapper::ActionMapper()
{
	cleared = false;
	inputEnabled = true;
	inUpdate = false;
	InputMapper::RegisterActionMapper(this);
}

ActionMapper::~ActionMapper()
{
	InputMapper::UnregisterActionMapper(this);
	clearCreatedEvents();
}

LegacyActionData *ActionMapper::getActionDataByID(int actionID)
{
	for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); ++i)
	{
		if (i->id == actionID)
			return &(*i);
	}
	return 0;
}

bool ActionMapper::isActing(unsigned actionID) const
{
	return actionID < _activeActions.size() ? _activeActions[actionID] : false;
}

void ActionMapper::addAction(unsigned actionID, unsigned k)
{
	assert(k);
	if(!k)
		return;

	LegacyActionData *ad = getActionDataByID(actionID);

	if (!ad)
	{
		LegacyActionData data;
		data.id = actionID;
		actionData.push_back(data);
		ad = &actionData.back();
	}

	if (ad)
	{
		if(std::find(ad->buttonList.begin(), ad->buttonList.end(), k) == ad->buttonList.end())
			ad->buttonList.push_back(k);
	}
}

void ActionMapper::addAction(Event *event, unsigned k, int state)
{
	LegacyActionData data;
	data.event = event;
	data.state = state;
	data.buttonList.push_back(k);
	actionData.push_back(data);
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

void ActionMapper::onUpdate (float dt)
{
	if(inputEnabled && !inUpdate)
	{
		inUpdate = true;
		updateDirectInput();
		updateActions();
		inUpdate = false;
	}

	_actionChanges.clear();
	_inputChanges.clear();

}

void ActionMapper::updateActions()
{
	// Trigger queued events and propagate changes
	for(size_t i = 0; i < _actionChanges.size(); ++i)
	{
		const ActionUpdate& am = _actionChanges[i];
		if(am.id >= _activeActions.size())
			_activeActions.resize(am.id);
		_activeActions[am.id] = am.state;
		action(am.id, am.state, am.playerID, am.device);
	}
}

void ActionMapper::updateDirectInput()
{
	cleared = false;
	for(size_t q = 0; q < _inputChanges.size(); ++q)
	{
		int ik = _inputChanges[q];
		bool keyState;
		unsigned k;
		if(ik < 0)
		{
			keyState = false;
			k = -ik;
		}
		else
		{
			keyState = true;
			k = ik;
		}

		for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); ++i)
		{
			const LegacyActionData& ad = (*i);
			for (ButtonList::const_iterator j = ad.buttonList.begin(); j != ad.buttonList.end(); j++)
			{
				if(k == (*j))
				{
					if (ad.event)
					{
						if (ad.state==-1 || keyState == !!ad.state)
						{
							ad.event->act();
						}
					}
					else
					{
						action(ad.id, keyState, -1, INP_DEV_NODEVICE);
					}
					if (core->loopDone || cleared)
						return;
				}
			}
		}
	}
}

void ActionMapper::clearActions()
{
	cleared = true;
	actionData.clear();
}

void ActionMapper::recvAction(unsigned actionID, bool keyState, int playerID, InputDeviceType device)
{
	if(!inputEnabled)
		return;

	// enqueue change for later
	// Since the update order isn't actually *defined* anywhere but it'm almost sure there
	// will be subtle bugs if things aren't updated in the "right" order
	// (aka as it happened to be, so far),
	// use a queue so that the code can keep believing it's polling changes, while in fact
	// under the hood it's a push-based system.
	// Pushing every change to a queue also ensures we can't miss a button press that
	// lasts shorter than one frame.
	ActionUpdate am;
	am.id = actionID;
	am.state = keyState;
	am.playerID = playerID;
	am.device = device;
	_actionChanges.push_back(am);
}

void ActionMapper::recvDirectInput(unsigned k, bool keyState)
{
	if(!inputEnabled)
		return;

	_inputChanges.push_back(keyState ? (int)k : -(int)k);
}

void ActionMapper::ImportInput(const NamedAction *actions, size_t N)
{
	// FIXME controllerfixup
	InputMapper *im = NULL;
	ActionSet as;

	/*
	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
	const ActionSet& as = dsq->user.control.actionSets[i];
	*/

	for(size_t i = 0; i < N; ++i)
		as.importAction(im, actions[i].name, actions[i].actionID);
}

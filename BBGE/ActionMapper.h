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
#ifndef _actionmapper_
#define _actionmapper_

#include "Base.h"
#include "ActionInput.h"

InputDevice getDeviceForActionbutton(int k);

class Event;
class ActionMapper;

#include "ActionStatus.h"
#include "ActionSet.h"
#include "Joystick.h"

typedef std::vector<int> ButtonList;

struct ActionData
{
	ActionData()
		: id(-1), state(-1), source(-1)
		, event(0)
	{
	}

	int id, state, source;
	Event *event;
	ButtonList buttonList;
};

class ActionMapper
{
public:

	// funcs
	ActionMapper();
	virtual ~ActionMapper();

	void addAction(Event *event, int k, int state=-1);
	void addAction(int actionID, int k, int source);

	bool isActing(int actionID, int source);
	virtual void action(int actionID, int state, int source, InputDevice device) = 0;


	void clearActions();

	bool isInputEnabled() { return inputEnabled; }

	// vars

	typedef std::vector<ActionData> ActionDataSet;
	ActionDataSet actionData;

	bool cleared;

	virtual void enableInput();
	virtual void disableInput();

	Event *addCreatedEvent(Event *event);
	void clearCreatedEvents();

	//bool pollAction(int actionID, int source);
	bool getKeyState(int k);
	bool getKeyState(int k, int sourceID);

	ActionData *getActionDataByIDAndSource(int actionID, int source);

protected:

	std::vector<Event*> createdEvents;

	bool inUpdate;
	bool inputEnabled;
	void onUpdate (float dt);
private:
	bool isKeyChanged(int k);
	bool isKeyChanged(int k, int sourceID);
	//bool _pollActionData(const ActionData& ad);
};

#endif




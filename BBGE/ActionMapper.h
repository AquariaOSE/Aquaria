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

class Event;
class ActionMapper;

#include <map>
#include <list>
#include "ActionSet.h"
#include "Joystick.h"

typedef std::vector<int> ButtonList;

struct ActionData
{
	ActionData() { id=-1; state=-1; event=0; }

	int id, state;
	Event *event;
	ButtonList buttonList;
};

enum ActionButtonType
{
	// must start at > 512 (SDL scancodes end there)
	MOUSE_BUTTON_LEFT	=  1000,
	MOUSE_BUTTON_RIGHT	= 1001,
	MOUSE_BUTTON_MIDDLE	= 1002,
	MOUSE_BUTTON_EXTRA_START = 1003,

	JOY_BUTTON_0		= 2000,
	JOY_BUTTON_END = JOY_BUTTON_0 + MAX_JOYSTICK_BTN, // one past end

	JOY_AXIS_0_POS			= 3000,
	JOY_AXIS_0_NEG			= 3100,
	JOY_AXIS_END_POS = JOY_AXIS_0_POS + MAX_JOYSTICK_AXIS,
	JOY_AXIS_END_NEG = JOY_AXIS_0_NEG + MAX_JOYSTICK_AXIS,
};

class ActionMapper
{
public:

	// funcs
	ActionMapper();
	virtual ~ActionMapper();

	void addAction(Event *event, int k, int state=-1);
	void addAction(int actionID, int k, int source);

	void removeAction(int actionID);
	void removeAllActions();

	bool isActing(int actionID);
	virtual void action(int actionID, int state, int source) = 0;


	void clearActions();

	bool isInputEnabled() { return inputEnabled; }

	// vars

	typedef std::list<ActionData> ActionDataSet;
	ActionDataSet actionData;

	typedef std::map <int, int> KeyDownMap;
	KeyDownMap keyDownMap;

	bool cleared;

	virtual void enableInput();
	virtual void disableInput();

	Event *addCreatedEvent(Event *event);
	void clearCreatedEvents();

	bool pollAction(int actionID);
	bool getKeyState(int k);

	ActionData *getActionDataByID(int actionID);
protected:

	std::vector<Event*>createdEvents;

	bool inUpdate;
	bool inputEnabled;
	void onUpdate (float dt);
};

#endif




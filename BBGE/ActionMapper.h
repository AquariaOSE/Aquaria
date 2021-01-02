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

#include "ActionSet.h"

typedef std::vector<int> ButtonList;

struct ActionData
{
	ActionData() { id=-1; state=-1; event=0; }

	int id, state;
	Event *event;
	ButtonList buttonList;
};

#define ACTION_EVENT		= -1

class ActionMapper
{
public:

	// funcs
	ActionMapper();
	virtual ~ActionMapper();

	void addAction(Event *event, int k, int state=-1);
	void addAction(int actionID, int k);

	void removeAction(int actionID);
	void removeAllActions();

	bool isActing(int actionID);
	virtual void action(int actionID, int state){}


	void clearActions();

	bool isInputEnabled() { return inputEnabled; }

	// vars

	typedef std::list<ActionData> ActionDataSet;
	ActionDataSet actionData;

	typedef std::map <int, int> KeyDownMap;
	KeyDownMap keyDownMap;

	bool cleared;



	enum {
		MOUSE_BUTTON_LEFT	=  999,
		MOUSE_BUTTON_RIGHT	= 1000,
		MOUSE_BUTTON_MIDDLE	= 1001,

		JOY1_BUTTON_0		= 2000,
		JOY1_BUTTON_1		= 2001,
		JOY1_BUTTON_2		= 2002,
		JOY1_BUTTON_3		= 2003,
		JOY1_BUTTON_4		= 2004,
		JOY1_BUTTON_5		= 2005,
		JOY1_BUTTON_6		= 2006,
		JOY1_BUTTON_7		= 2007,
		JOY1_BUTTON_8		= 2008,
		JOY1_BUTTON_9		= 2009,
		JOY1_BUTTON_10		= 2010,
		JOY1_BUTTON_11		= 2011,
		JOY1_BUTTON_12		= 2012,
		JOY1_BUTTON_13		= 2013,
		JOY1_BUTTON_14		= 2014,
		JOY1_BUTTON_15		= 2015,
		JOY1_BUTTON_16		= 2016,


		X360_BTN_START		= 3016,
		X360_BTN_BACK		= 3017,

		JOY1_DPAD_LEFT		= 4000,
		JOY1_DPAD_RIGHT		= 4001,
		JOY1_DPAD_DOWN		= 4002,
		JOY1_DPAD_UP		= 4003,
		JOY1_STICK_LEFT		= 4010,
		JOY1_STICK_RIGHT	= 4011,
		JOY1_STICK_DOWN		= 4012,
		JOY1_STICK_UP		= 4013,
	};

	enum { DPAD_LEFT = 0, DPAD_RIGHT, DPAD_UP, DPAD_DOWN };



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




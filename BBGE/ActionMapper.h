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

class Event;
class ActionMapper;

#include "ActionSet.h"
#include "Joystick.h"

struct LegacyActionData;

class ActionMapper
{
public:

	// funcs
	ActionMapper();
	virtual ~ActionMapper();

	// Overridden to react to events (both legacy and input-mapped)
	virtual void action(int actionID, int state, int playerID, InputDeviceType device) = 0;


	// Legacy actions -- binds keys directly to actions.
	// This bypasses the input mapper and checks for keys directly!
	void addAction(Event *event, unsigned k, int state = -1);
	void addAction(unsigned actionID, unsigned k);
	bool isActing(unsigned actionID) const;


	void clearActions();

	bool isInputEnabled() { return inputEnabled; }

	// vars

	typedef std::vector<LegacyActionData> ActionDataSet;
	ActionDataSet actionData;

	bool cleared;

	virtual void enableInput();
	virtual void disableInput();

	Event *addCreatedEvent(Event *event);
	void clearCreatedEvents();


	// called by InputMapper whenever stuff happens
	void recvAction(unsigned actionID, bool state, int playerID, InputDeviceType device);
	void recvDirectInput(unsigned k, bool state);

protected:
	/*template<size_t N> void importInput(const NamedAction (&a)[N])
	{
		importInput(&a[0], N);
	}
	void importInput(const NamedAction *actions, size_t N);*/

	std::vector<Event*> createdEvents;

	bool inputEnabled;
	void onUpdate (float dt);

private:
	void updateDirectInput();
	void updateActions();

	LegacyActionData *getActionDataByID(int actionID);

	struct ActionUpdate
	{
		unsigned id;
		int playerID;
		bool state;
		InputDeviceType device;
	};
	std::vector<ActionUpdate> _actionChanges;
	std::vector<int> _inputChanges; // >0: pressed, <0: released

	bool inUpdate;
	std::vector<unsigned> _activeActions;
};

#endif




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
#ifndef BBGE_STATE_DATA_H
#define BBGE_STATE_DATA_H

#include "Base.h"
#include <map>
#include "ActionMapper.h"
#include "Event.h"

class StateManager;
class RenderObject;

class StateObject;


class StateObject : public ActionMapper
{
	friend class StateManager;
public:
	StateObject();
	virtual ~StateObject();

	virtual void applyState() {}
	virtual void removeState();
	virtual void update(float dt);

	static void addRenderObject(RenderObject *renderObject, unsigned layer);

	virtual void action(int actionID, int state, int source, InputDevice device) {}

protected:
	void registerState(StateObject *sb, const std::string &name);

private:
	std::string _statename;
};

class StateManager
{
public:
	friend class StateObject;
	StateManager();
	virtual ~StateManager();
	void clearStateObjects();

	void leaveState();

	virtual void applyState (const std::string &state) {}
	virtual void removeState (const std::string& state) {}


	void enqueueJumpState (const std::string &state, bool force = false, bool staged = false);
	bool isStateJumpPending();


	StateObject *getStateObject(const std::string &state);
	inline StateObject *getTopStateObject() const { return currentState; }

	virtual bool canChangeState() { return true; }
protected:
	void registerStateObject(StateObject *stateObject, const std::string &name);
	virtual void onUpdate(float dt);
	std::string enqueuedJumpState;

private:
	StateObject *currentState;
	std::string currentStateName;

	typedef std::map<std::string, StateObject*> StateObjectMap;
	StateObjectMap stateObjects;


private:
	void jumpState (const std::string &state); // call enqueueJumpState
	void enterState(const std::string &state);
};


#endif

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

class StateData
{
public:
	StateData();
	~StateData();
	std::vector <RenderObject*> renderObjects;
	std::vector <EventPtr> events;
	std::vector <RenderObject*> garbage;

	StateObject *stateObject;

	void clearGarbage();
	void addRenderObject(RenderObject *renderObject, unsigned layer);
	void eraseRenderObjects();
	void removeRenderObject(RenderObject *renderObject);
	void removeRenderObjectFromList(RenderObject *renderObject);

	std::string name;
};

class StateObject : public ActionMapper
{
public:
	StateObject();
	virtual ~StateObject();

	virtual void applyState(){}
	virtual void removeState();
	virtual void update(float dt);

	void addRenderObject(RenderObject *renderObject, unsigned layer);
	void removeRenderObject(RenderObject *renderObject);
	std::string name;

	virtual void action(int actionID, int state, int source, InputDevice device) {}

protected:
	void registerState(StateObject *sb, const std::string &name);
};

class StateManager
{
public:
	friend class StateObject;
	StateManager();
	~StateManager();


	void clearStateObjects();

	virtual void applyState (const std::string &state) {}
	virtual void removeState (std::string state);


	void enqueueJumpState (const std::string &state, bool force = false, bool staged = false);
	bool isStateJumpPending();
	void pushState(const std::string &state);
	void popState();
	void popAllStates();


	StateData *getState(const std::string &state);
	StateData *getTopStateData();
	StateObject *getTopStateObject();

	virtual bool canChangeState()=0;

	StateObject *addStateInstance(StateObject *s);
	void clearStateInstances();

	typedef std::vector<StateObject*> StateInstances;
	StateInstances stateInstances;
protected:
	int enqueuedStateStage;
	std::string enqueuedJumpState;
	bool stateChangeFlag;
	std::string getNameFromDerivedClassTypeName(const std::string &typeidName);
	void registerStateObject(StateObject *stateObject, const std::string &name="");
	virtual void onUpdate(float dt);

	// std::stack is horrendously overweight, so we use a simple stack
	// implementation instead.
	#define STATE_STACK_SIZE 64
	StateData *states[STATE_STACK_SIZE];
	int statesTopIndex;
	bool states_empty()     {return statesTopIndex == -1;}
	bool states_full()      {return statesTopIndex == STATE_STACK_SIZE-1;}
	StateData *states_top() {return statesTopIndex >= 0 ? states[statesTopIndex] : 0;}

	typedef std::map<std::string, StateObject*> StateObjectMap;
	StateObjectMap stateObjects;


private:
	void jumpState (const std::string &state); // call enqueueJumpState
};

extern StateManager *stateManager;

#endif

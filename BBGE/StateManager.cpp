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
#include "StateManager.h"
#include "Core.h"


//========[ StateObject ]========//
StateObject::StateObject() : ActionMapper()
{
}

void StateObject::registerState(StateObject *sb, const std::string &name)
{
	core->registerStateObject(sb, name);
}

StateObject::~StateObject()
{
}

void StateObject::removeState()
{
	clearActions();
	clearCreatedEvents();
}

void StateObject::addRenderObject(RenderObject *renderObject, unsigned layer)
{
	renderObject->roflags |= RO_TEMPORARY;
	core->addRenderObject(renderObject, layer);
}

void StateObject::update(float dt)
{
	ActionMapper::onUpdate(dt);
}

//========[ STATEMANAGER ]========//

StateManager::StateManager()
	: currentState(NULL)
{
}

StateManager::~StateManager()
{
	clearStateObjects();
}

StateObject *StateManager::getStateObject(const std::string &state)
{
	StateObjectMap::iterator it = stateObjects.find(state);
	return it != stateObjects.end() ? it->second : NULL;
}

void StateManager::jumpState (const std::string &state)
{
	if (canChangeState())
	{
		leaveState();
		enterState(state);
	}
}

void StateManager::enterState(const std::string& state)
{
	StateObject *newstate = getStateObject(state);
	currentState = newstate;

	if(newstate)
		newstate->applyState();

	this->applyState(state);
	currentStateName = state;
}

void StateManager::leaveState()
{
	if(currentState)
		currentState->removeState();
	this->removeState(currentStateName);

	currentState = NULL;
	currentStateName.clear();
}

/*
enqueueJumpState
force will force the state change regardless of the result of canChangeState()
staged = if true, the manager will pop all the states and allow the system to clear garbage before pushing the new state
*/
void StateManager::enqueueJumpState (const std::string &state, bool force, bool staged)
{
	if (force || canChangeState())
	{
		enqueuedJumpState = state;
		stringToLower(enqueuedJumpState);
	}
}

bool StateManager::isStateJumpPending()
{
	return !enqueuedJumpState.empty();
}

void StateManager::registerStateObject(StateObject *stateObject, const std::string &name)
{
	stateObject->_statename = name;
	stringToLower(stateObject->_statename);

	StateObject *old = getStateObject(stateObject->_statename);
	if(old)
		delete old;

	stateObjects[stateObject->_statename] = stateObject;

}

void StateManager::clearStateObjects()
{
	for(StateObjectMap::iterator it = stateObjects.begin(); it != stateObjects.end(); ++it)
		delete it->second;
	stateObjects.clear();
}

void StateManager::onUpdate(float dt)
{
	if (currentState)
		currentState->update(dt);

	if (canChangeState() && !enqueuedJumpState.empty())
	{
		std::string copy;
		copy.swap(enqueuedJumpState);
		jumpState(copy);
	}
}

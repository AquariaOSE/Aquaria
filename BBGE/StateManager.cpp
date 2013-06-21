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

StateManager *stateManager = 0;

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
	//stateManager->getState(name)->eraseRenderObjects();
}

void StateObject::addRenderObject(RenderObject *renderObject, int layer)
{
	stateManager->getState(name)->addRenderObject(renderObject, layer);
}

void StateObject::removeRenderObject(RenderObject *renderObject)
{
	stateManager->getState(name)->removeRenderObject(renderObject);
}

void StateObject::action(int id, int state)
{
}

void StateObject::update(float dt)
{
	ActionMapper::onUpdate(dt);
}

//========[ STATEDATA ]========//
StateData::StateData()
{
	stateObject = 0;
}

StateData::~StateData()
{
	/*
	std::ostringstream os;
	os << "removing " << renderObjects.size() << " render Objects";
	MessageBox(0,os.str().c_str(),"",MB_OK);
	*/
	for (int i = 0; i < renderObjects.size(); i++)
	{
		removeRenderObject (renderObjects[i]);
		delete renderObjects[i];
	}
}

void StateData::addRenderObject(RenderObject *renderObject, int layer)
{
	core->addRenderObject(renderObject, layer);
	renderObjects.push_back (renderObject);
	renderObject->setStateDataObject(this);
	renderObject->layer = layer;
}

void StateData::removeRenderObject(RenderObject *renderObject)
{
	core->enqueueRenderObjectDeletion(renderObject);

	removeRenderObjectFromList(renderObject);
}

void StateData::removeRenderObjectFromList(RenderObject *renderObject)
{
	for (std::vector<RenderObject*>::iterator i = renderObjects.begin(); i != renderObjects.end(); )
	{
		if ((*i) == renderObject)
		{
			i = renderObjects.erase(i);
		}
		else
		{
			i ++;
		}
	}
}

// assume this only happens on render state end
void StateData::eraseRenderObjects() 
{
	// why clear garbage here?
	//core->clearGarbage();

	for (int i = 0; i < renderObjects.size(); i++)
	{
		RenderObject *r = renderObjects[i];
		if (r && !r->isDead())
		{
			core->enqueueRenderObjectDeletion(renderObjects[i]);
		}
	}
	renderObjects.clear();
}

//========[ STATEMANAGER ]========//

StateManager::StateManager()
{
	stateManager = this;
	stateChangeFlag = false;
	enqueuedStateStage = -1;
	statesTopIndex = -1;
}

StateManager::~StateManager()
{
	clearStateObjects();
}

StateData *StateManager::getState (const std::string &state)
{
	for (int i = 0; i <= statesTopIndex; i++) {
		StateData *s = states[i];
		if (s->name == state)
			return s;
	}

	return 0;
}

void StateManager::removeState(std::string state)
{
	getState(state)->eraseRenderObjects();
}

void StateManager::jumpState (const std::string &state)
{
	if (canChangeState())
	{
		popAllStates();
		pushState(state);
	}
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
		if (staged)
		{
			enqueuedStateStage = 0;
		}
		else
		{
			enqueuedStateStage = -1;
		}
	}
}

bool StateManager::isStateJumpPending()
{
	return !enqueuedJumpState.empty();
}

void StateManager::pushState(const std::string &s)
{
	std::string state = s;
	stringToLower(state);

	if (canChangeState())
	{
		if (states_full())
		{
			debugLog("state stack overflow!!");
			return;
		}
		StateData *s = new StateData;
		s->name = state;
		states[++statesTopIndex] = s;

		applyState(state);

		if (stateObjects[state])
		{
			s->stateObject = stateObjects[state];
			stateObjects[state]->applyState();
		}
		
		stateChangeFlag = true;
	}
}

void StateManager::popAllStates()
{
	if (!states_empty())
	{
		popState();
		popAllStates();
	}
}

void StateManager::popState()
{
	if (canChangeState() && !states_empty())
	{
		if (stateObjects[(*(states_top())).name])
			stateObjects[(*(states_top())).name]->removeState();
		//states_top()->eraseRenderObjects();
		std::string n = (*(states_top())).name;
		removeState(n);
		delete states_top();
		if (core->getNestedMains()==1)
			core->clearGarbage();
		statesTopIndex--;
		stateChangeFlag = true;
	}
}

std::string StateManager::getNameFromDerivedClassTypeName(const std::string &typeidName)
{
/*
	int loc = typeidName.find_last_of("class ");
	if (loc != std::string::npos)
	{
		std::string tmp = typeidName.substr(6, typeidName.length());
		int loc2 = tmp.find_last_of("::");
		if (loc2 != std::string::npos)
			return tmp.substr(loc2+1, tmp.size());
		else
			return tmp;
	}
	else
		return typeidName;
		*/
	return "";
}

void StateManager::registerStateObject(StateObject *stateObject, const std::string &name)
{
	//const char *c = typeid(*stateObject).name();

	stateObject->name = name;
	stringToLower(stateObject->name);

	//getNameFromDerivedClassTypeName(c);
	if (stateObject->name.empty())
	{
		fatalError("StateManager::registerStateObject - Empty name.");
	}

	if (!stateObjects[stateObject->name])
		stateObjects[stateObject->name] = stateObject;
		/*
	if (c)
		free((void*)c);
		*/
}

StateObject *StateManager::addStateInstance(StateObject *s)
{
	stateInstances.push_back(s);
	return s;
}

void StateManager::clearStateInstances()
{
	for (int i = 0; i < stateInstances.size(); i++)
	{
		StateObject *obj = stateInstances[i];
		delete obj;
	}
	stateInstances.clear();
}


void StateManager::clearStateObjects()
{

	stateObjects.clear();
}

void StateManager::onUpdate(float dt)
{
	for (int i = 0; i <= statesTopIndex; i++)
	{
		StateObject *obj = stateObjects[states[i]->name];
		if (obj)
			obj->update(dt);
	}

	if (canChangeState() && !enqueuedJumpState.empty())
	{
		if (enqueuedStateStage == 0)
		{
			enqueuedStateStage = -1;
			popAllStates();
		}
		else if (enqueuedStateStage == -1)
		{
			std::string copy = enqueuedJumpState;
			enqueuedJumpState = "";
			jumpState(copy);
		}
	}
}

StateData *StateManager::getTopStateData()
{
	if (!states_empty())
		return states_top();
	else
		return 0;
}

StateObject *StateManager::getTopStateObject()
{
	StateData *s = getTopStateData();
	if (s)
	{
		return s->stateObject;
	}
	else
		return 0;
}

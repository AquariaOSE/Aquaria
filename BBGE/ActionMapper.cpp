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
#include "ActionMapper.h"
#include "Core.h"

//bool ActionMapper::isActing(const std::string &action)
//{
//	ButtonList::iterator i = actionMap[action].begin();
//	for (; i != actionMap[action].end(); i++)
//	{
//		if (keyDownMap[(*i)])
//			return true;
//	}
//	return false;
// //return keyDownMap[actionMap[action]];
//}

//bool ActionMapper::isActing(int actionID)
//{
//	std::string action = "A";
//	action[0] = char(actionID);
//	ButtonList::iterator i = actionMap[action].begin();
//	for (; i != actionMap[action].end(); i++)
//	{
//		if (keyDownMap[(*i)])
//			return true;
//	}
//	return false;
// //return keyDownMap[actionMap[action]];
//}

//void ActionMapper::addAction (const std::string &action, int k)
//{
//	actionMap[action].push_back(k);
//	keyDownMap[k] = core->getKeyState(k);
//}
//
//void ActionMapper::addAction (int actionID, int k)
//{
//	std::string action = "A";
//	action[0] = char(actionID);
//	actionMap[action].push_back(k);
//	keyDownMap[k] = core->getKeyState(k);
//}

ActionMapper::ActionMapper()
{
	cleared = false;
	inputEnabled = true;
	inUpdate = false;
}

ActionMapper::~ActionMapper()
{
	clearCreatedEvents();
}

ActionData *ActionMapper::getActionDataByID(int actionID)
{
	for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end(); i++)
	{
		if ((*i).id == actionID)
			return &(*i);
	}
	return 0;
}

bool ActionMapper::isActing(int actionID)
{
	ActionData *ad = getActionDataByID(actionID);
	if (ad)
	{
		ButtonList::iterator i = ad->buttonList.begin();
		for (; i != ad->buttonList.end(); i++)
		{
			if (keyDownMap[(*i)])
				return true;
		}
	}
	return false;
}

void ActionMapper::addAction (int actionID, int k)
{
	ActionData *ad = getActionDataByID(actionID);

	if (ad)
	{
		/*
		std::ostringstream os;
		os << "Action ID [" << actionID << "] already exists!";
		debugLog(os.str());
		*/
	}
	else
	{
		ActionData data;
		data.id = actionID;
		actionData.push_back(data);
		ad = getActionDataByID(actionID);
		if (!ad)
		{
			std::ostringstream os;
			os << "Could not create action for Action ID [" << actionID << "]";
			errorLog(os.str());
			return;
		}
	}

	if (ad)
	{
		ad->buttonList.push_back(k);
		keyDownMap[k] = core->getKeyState(k);
	}
}



void ActionMapper::addAction(Event *event, int k, int state)
{
	ActionData data;
	data.event = event;
	data.state = state;
	data.buttonList.push_back(k);
	actionData.push_back(data);

	keyDownMap[k] = core->getKeyState(k);
}

Event* ActionMapper::addCreatedEvent(Event *event)
{
	for (int i = 0; i < createdEvents.size(); i++)
	{
		if (createdEvents[i] == event)
			return event;
	}
	createdEvents.push_back(event);
	return event;
}

void ActionMapper::clearCreatedEvents()
{
	for (int i = 0; i < createdEvents.size(); i++)
	{
		delete createdEvents[i];
	}
	createdEvents.clear();
}

/*
void ActionMapper::addMouseButtonAction (const std::string &action, int b)
{
	actionMap[action].push_back (0-b);
	keyDownMap[0-b] = mouse_b & b;
}

void ActionMapper::addJoystickButtonAction (const std::string &action, int b)
{
	if (num_joysticks)
	{
		actionMap[action].push_back (b+9000);
		keyDownMap[b+9000] = joy[0].button[b].b;
	}
}

int ActionMapper::getDPad(int dir)
{
//	for (int = 0; i < joy[0].num_sticks; i++)
	//int s = 4;
	int s = 0;
	switch (dir)
	{
	case 0:
		return (joy[0].stick[s].axis[0].d1);
	break;
	case 1:
		return (joy[0].stick[s].axis[0].d2);
	break;
	case 2:
		return (joy[0].stick[s].axis[1].d1);
	break;
	case 3:
		return (joy[0].stick[s].axis[1].d2);
	break;
	}
	return 0;
}


void ActionMapper::addJoystickDPadAction (const std::string &action, int dir)
{
	if (num_joysticks)
	{
		actionMap[action].push_back (dir+8000);
		keyDownMap[dir+8000] = getDPad(dir);
	}
}
*/

void ActionMapper::enableInput()
{
	inputEnabled = true;
}

void ActionMapper::disableInput()
{
	inputEnabled = false;
}

void ActionMapper::removeAction(int actionID)
{
	ActionData *ad = getActionDataByID(actionID);
	if (ad)
	{
		ButtonList::iterator i = ad->buttonList.begin();
		for (; i != ad->buttonList.end(); i++)
		{
			int k = (*i);
			cleared = true; // it's a hack, but it works	
			keyDownMap.erase(k);
		}
		for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end();)
		{
			if (i->id == actionID)
				i = actionData.erase(i);
			else
				i++;
		}
	}
}

//int ActionMapper::getKeyForAction (std::string action)
//{
//	ButtonList b = actionMap[action];
//	if (!b.empty())
//	{
//		return b[0];
//	}
//	else
//	{
//		debugLog ("no action: " +action);
//		return -1;
//	}
//}

bool ActionMapper::pollAction(int actionID)
{
	bool down = false;

	ActionData *ad = getActionDataByID(actionID);
	if (ad)
	{
		ButtonList *blist = &ad->buttonList;
		ButtonList::iterator j;
		j = blist->begin();
		
		for (; j != blist->end(); j++)
		{
			if (getKeyState((*j)))
			{
				down = true;
				break;
			}
		}
	}

	return down;
}

bool ActionMapper::getKeyState(int k)
{
	bool keyState = false;
	if (k == KEY_ANYKEY)
	{
		keyState = false;
		for (int i = 0; i < KEY_MAXARRAY; i ++)
		{
			if (core->getKeyState(i))
			{
				keyState = true;
				break;
			}
		}
	}
	else if (k >= 0 && k < KEY_MAXARRAY)
	{
		keyState = (core->getKeyState(k));
	}
	else if (k == MOUSE_BUTTON_LEFT)
	{
		keyState = (core->mouse.buttons.left == DOWN);
	}
	else if (k == MOUSE_BUTTON_RIGHT)
	{
		keyState = (core->mouse.buttons.right == DOWN);
	}
	else if (k == MOUSE_BUTTON_MIDDLE)
	{
		keyState = (core->mouse.buttons.middle == DOWN);
	}			
	else if (k >= JOY1_BUTTON_0 && k <= JOY1_BUTTON_16)
	{
		int v = k - JOY1_BUTTON_0;
		
		if (core->joystickEnabled)
			keyState = core->joystick.buttons[v];
	}
	else if (k == JOY1_STICK_LEFT)
	{
		keyState = core->joystick.position.x < -0.6f;
	}
	else if (k == JOY1_STICK_RIGHT)
	{
		keyState = core->joystick.position.x > 0.6f;
	}
	else if (k == JOY1_STICK_UP)
	{
		keyState = core->joystick.position.y < -0.6f;
	}
	else if (k == JOY1_STICK_DOWN)
	{
		keyState = core->joystick.position.y > 0.6f;
	}
	else if (k == X360_BTN_START)
	{
		keyState = core->joystick.btnStart;
	}
	else if (k == X360_BTN_BACK)
	{
		keyState = core->joystick.btnSelect;
	}
	else if (k == JOY1_DPAD_LEFT)
	{
		keyState = core->joystick.dpadLeft;
	}
	else if (k == JOY1_DPAD_RIGHT)
	{
		keyState = core->joystick.dpadRight;
	}
	else if (k == JOY1_DPAD_UP)
	{
		keyState = core->joystick.dpadUp;
	}
	else if (k == JOY1_DPAD_DOWN)
	{
		keyState = core->joystick.dpadDown;
	}

	return keyState;
}

void ActionMapper::onUpdate (float dt)
{	
	if (inUpdate) return;
	inUpdate = true;
	/*
	if (num_joysticks)
		poll_joystick();
	*/
	if (cleared) cleared = false;
	ActionDataSet::iterator i;
	KeyDownMap oldKeyDownMap = keyDownMap;
	for (i = actionData.begin(); i != actionData.end(); ++i)
	{
		ButtonList::iterator j;
		j = i->buttonList.begin();
		for (; j != i->buttonList.end(); j++)
		{
			int k = (*j);
			int keyState=false;
			//joystick

			keyState = getKeyState(k);

			if (keyState != oldKeyDownMap[k])
			{				
				keyDownMap[k] = keyState;
				if (inputEnabled)
				{
					ActionData *ad = &(*i);
					if (ad->event)
					{
						if (ad->state==-1 || keyState == ad->state)
						{
							ad->event->act();
						}
					}
					else
					{
						action(ad->id, keyState);
					}
					if (core->loopDone) goto out;
				}
				if (cleared) { cleared = false; goto out; } // actionData has been cleared, stop iteration
			}
		}
	}

out:
	inUpdate = false;
//		keyDownMap[k] = ;
}

void ActionMapper::clearActions()
{
	cleared = true;
	keyDownMap.clear();
	actionData.clear();
}

void ActionMapper::removeAllActions()
{
	std::vector <int> deleteList;
	ActionDataSet::iterator i;
	for (i = actionData.begin(); i != actionData.end(); i++)
	{
		deleteList.push_back(i->id);
	}
	for (int c = 0; c < deleteList.size(); c++)
	{
		removeAction (deleteList[c]);
	}
	actionData.clear();
}

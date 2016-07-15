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
#include "ActionSet.h"
#include "Core.h"

JoystickConfig::JoystickConfig()
{
	s1ax = 0;
	s1ay = 0;
	s2ax = 0;
	s2ay = 0;
	s1dead = 0.3f;
	s2dead = 0.3f;
}

ActionSet::ActionSet()
{
	enabled = true;
	joystickID = 0;
}

void ActionSet::clearActions()
{
	inputSet.clear();
}

int ActionSet::assignJoystickByName()
{
	int idx = _whichJoystickForName();
	if(idx >= 0)
		assignJoystickIdx(idx);
	return idx;
}

void ActionSet::assignJoystickIdx(int idx)
{
	if(idx < 0)
	{
		joystickID = -1;
		joystickName.clear();
		joystickGUID.clear();
	}
	else if(idx < (int)core->getNumJoysticks())
	{
		if(Joystick *j = core->getJoystick(idx))
		{
			joystickGUID = j->getGUID();
			joystickName = j->getName();
			joystickID = idx;
		}
	}
}

int ActionSet::_whichJoystickForName()
{
	if(joystickGUID.length())
		for(size_t i = 0; i < core->getNumJoysticks(); ++i)
			if(Joystick *j = core->getJoystick(i))
				if(j->getGUID()[0] && joystickGUID == j->getGUID())
					return int(i);

	if(joystickName.length())
		for(size_t i = 0; i < core->getNumJoysticks(); ++i)
			if(Joystick *j = core->getJoystick(i))
				if(joystickName == j->getName())
					return int(i);

	return -1;
}

ActionInput *ActionSet::getActionInputByName(const std::string &name)
{
	for (ActionInputSet::iterator i = inputSet.begin(); i != inputSet.end(); i++)
	{
		if (nocasecmp((*i).name, name) == 0)
		{
			return &(*i);
		}
	}
	return 0;
}



void ActionSet::importAction(ActionMapper *mapper, const std::string &name, int actionID, int sourceID) const
{
	if (!enabled) return;
	if (!mapper) return;

	for (int i = 0; i < inputSet.size(); i++)
	{
		const ActionInput *actionInput = &inputSet[i];
		if (actionInput->name == name)
		{
			for (int i = 0; i < INP_MSESIZE; i++)
				if (actionInput->mse[i])
					mapper->addAction(actionID, actionInput->mse[i], sourceID);
			for (int i = 0; i < INP_KEYSIZE; i++)
				if (actionInput->key[i])
					mapper->addAction(actionID, actionInput->key[i], sourceID);
			for (int i = 0; i < INP_JOYSIZE; i++)
				if (actionInput->joy[i])
					mapper->addAction(actionID, actionInput->joy[i], sourceID);
			return;
		}
	}

}

void ActionSet::importAction(ActionMapper *mapper, const std::string &name, Event *event, int state) const
{
	if (!enabled) return;
	if (!mapper) return;

	for (int i = 0; i < inputSet.size(); i++)
	{
		const ActionInput *actionInput = &inputSet[i];
		if (actionInput->name == name)
		{
			for (int i = 0; i < INP_MSESIZE; i++)
				if (actionInput->mse[i])
					mapper->addAction(event, actionInput->mse[i], state);
			for (int i = 0; i < INP_KEYSIZE; i++)
				if (actionInput->key[i])
					mapper->addAction(event, actionInput->key[i], state);
			for (int i = 0; i < INP_JOYSIZE; i++)
				if (actionInput->joy[i])
					mapper->addAction(event, actionInput->joy[i], state);

			return;
		}
	}
}

ActionInput *ActionSet::addActionInput(const std::string &name)
{
	ActionInput *a = getActionInputByName(name);
	if(a)
		return a;

	ActionInput newa;
	newa.name = name;
	inputSet.push_back(newa);
	return &inputSet.back();
}

/*
std::string ActionSet::insertInputIntoString(const std::string &string)
{
	std::string str = string;

	int start = str.find('{');
	int end = str.find('}');
	if (start == std::string::npos || end == std::string::npos)
		return string;
	std::string code = str.substr(start+1, end - start);
	stringToLower(code);
	std::string part1 = str.substr(0, start);
	std::string part3 = str.substr(end+1, str.size());


	int thing = code.find(':');
	std::string input = code.substr(0, thing);
	std::string button = code.substr(thing+1, code.size());

	char buttonType;
	int buttonNum;

	std::istringstream is(button);
	is >> buttonType >> buttonNum;

	ActionInput *actionInput=0;
	actionInput = getActionInputByName(input);
	if (!actionInput)
	{
		// don't have that input, bail
		return string;
	}
	int inputCode=0;
	switch(buttonType)
	{
		case 'k':
			inputCode = actionInput->key[buttonNum];
			break;
		case 'j':
			inputCode = actionInput->joy[buttonNum];
			break;
		case 'm':
			inputCode = actionInput->mse[buttonNum];
			break;
	}

	std::string part2 = getInputCodeToUserString(inputCode);
	return part1 + part2 + part3;
}
*/


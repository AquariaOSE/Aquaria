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

void ActionSet::clearActions()
{
	inputSet.clear();
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

//void ActionSet::loadAction(const std::string &name, int inputCode, InputSetType set)
//{
//	ActionInput *a = getActionInputByName(name);
//	if (!a)
//	{
//		ActionInput newa;
//		newa.name = name;
//		inputSet.push_back(newa);
//		a = getActionInputByName(name);
//		
//		if (!a) return;
//	}
//
//	switch(set)
//	{
//	case INPUTSET_KEY:
//		a->keyCodes.push_back(inputCode);
//	break;
//	case INPUTSET_JOY:
//		a->joyCodes.push_back(inputCode);
//	break;
//	case INPUTSET_MOUSE:
//		a->mouseCodes.push_back(inputCode);
//	break;
//	case INPUTSET_GENERAL:
//	default:
//		a->inputCodes.push_back(inputCode);
//	break;
//	}
//}
//
//void ActionSet::loadAction(const std::string &name, const std::vector<int> &inputCodes, InputSetType set)
//{
//
//	ActionInput *a = getActionInputByName(name);
//	if (!a)
//	{
//		ActionInput newa;
//		newa.name = name;
//		inputSet.push_back(newa);
//		a = getActionInputByName(name);
//		
//		if (!a) return;
//	}
//
//	switch(set)
//	{
//	case INPUTSET_KEY:
//		a->keyCodes = inputCodes;
//	break;
//	case INPUTSET_JOY:
//		a->joyCodes = inputCodes;
//	break;
//	case INPUTSET_MOUSE:
//		a->mouseCodes = inputCodes;
//	break;
//	case INPUTSET_GENERAL:
//	default:
//		a->inputCodes = inputCodes;
//	break;
//	}
//}

void ActionSet::importAction(ActionMapper *mapper, const std::string &name, int actionID)
{
	if (!mapper) return;

	for (int i = 0; i < inputSet.size(); i++)
	{
		ActionInput *actionInput = &inputSet[i];
		if (actionInput->name == name)
		{
			for (int i = 0; i < INP_MSESIZE; i++)
				if (actionInput->mse[i])
					mapper->addAction(actionID, actionInput->mse[i]);
			for (int i = 0; i < INP_KEYSIZE; i++)
				if (actionInput->key[i])
					mapper->addAction(actionID, actionInput->key[i]);
			for (int i = 0; i < INP_JOYSIZE; i++)
				if (actionInput->joy[i])
					mapper->addAction(actionID, actionInput->joy[i]);
			return;
		}
	}

}

void ActionSet::importAction(ActionMapper *mapper, const std::string &name, Event *event, int state)
{
	if (!mapper) return;

	for (int i = 0; i < inputSet.size(); i++)
	{
		ActionInput *actionInput = &inputSet[i];
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
	if (!a)
	{
		ActionInput newa;
		newa.name = name;
		inputSet.push_back(newa);
		a = getActionInputByName(name);

		if (!a) return 0;
	}

	return a;
}

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
	
	//{ToggleHelp:k0}
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


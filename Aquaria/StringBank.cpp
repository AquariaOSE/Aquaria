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
#include "DSQ.h"

StringBank::StringBank()
{
}

void StringBank::load()
{
	stringMap.clear();

	_load("data/stringbank.txt");
	if (dsq->mod.isActive())
		_load(dsq->mod.getPath() + "stringbank.txt");
}

void StringBank::_load(const std::string &file)
{
	//debugLog("StringBank::load("+file+")");

	std::ifstream in(file.c_str());

	std::string line;
	int idx;

	while (in >> idx)
	{
		std::getline(in, line);

		//std::ostringstream os;
		//os << idx << ": StringBank Read Line: " << line;
		//debugLog(os.str());

		if (!line.empty() && line[0] == ' ')
			line = line.substr(1, line.size());
		for (int i = 0; i < line.size(); i++)
		{
			if (line[i] == '|')
				line[i] = '\n';
		}
		stringMap[idx] = line;
	}
}

std::string StringBank::get(int idx)
{
	return stringMap[idx];
}


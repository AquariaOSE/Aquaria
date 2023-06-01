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
#include "StringBank.h"
#include "ttvfs_stdio.h"
#include "Base.h"

StringBank stringbank;
static const std::string emptyStr;

StringBank::StringBank()
{
}

void StringBank::clear()
{
	stringMap.clear();
}

bool StringBank::empty() const
{
	return stringMap.empty();
}

bool StringBank::load(const std::string &file)
{
	debugLog("Load stringbank: " + file);
	InStream in(file.c_str());
	if(!in)
		return false;

	std::string line;
	int idx;

	while (in >> idx)
	{
		std::getline(in, line);

		if (!line.empty() && line[0] == ' ')
			line = line.substr(1, line.size());
		for (size_t i = 0; i < line.size(); i++)
		{
			if (line[i] == '|')
				line[i] = '\n';
		}
		stringMap[idx] = line;
	}
	return true;
}

const std::string& StringBank::get(int idx) const
{
	StringMap::const_iterator it = stringMap.find(idx);
	return it != stringMap.end() ? it->second : emptyStr;
}

void StringBank::set(int idx, const char *str)
{
	stringMap[idx] = str;
}

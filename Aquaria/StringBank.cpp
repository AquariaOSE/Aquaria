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

	// First, load the default string banks
	_load("data/stringbank.txt");
	if (dsq->mod.isActive())
		_load(dsq->mod.getPath() + "stringbank.txt");

	// Then, load localized ones. If some entries in these are missing, the default for each is taken.
	std::string fname = localisePath("data/stringbank.txt");
	_load(fname);

	if (dsq->mod.isActive()) {
		fname = localisePath(dsq->mod.getPath() + "stringbank.txt", dsq->mod.getPath());
		_load(fname);
	}

	if(stringMap.empty())
		exit_error("Failed to load data/stringbank.txt");
}

void StringBank::_load(const std::string &file)
{


	InStream in(file.c_str());

	std::string line;
	int idx;

	while (in >> idx)
	{
		std::getline(in, line);



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

const std::string& StringBank::get(int idx)
{
	return stringMap[idx];
}


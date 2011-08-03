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
#include "FileVars.h"

FileVars::FileVars()
{

}

void FileVars::load(const std::string &absPath)
{
	std::string vname;
	std::ifstream in(absPath.c_str());
	while (in >> vname)
	{
		std::string val;
		in >> val;
		if (val.find('.') != std::string::npos)
		{
			std::istringstream is(val);
			is >> floats[vname];
		}
		else
		{
			std::istringstream is(val);
			is >> ints[vname];
		}
	}
}

float FileVars::f(const std::string &vname)
{
	return floats[vname];
}

int FileVars::i(const std::string &vname)
{
	return ints[vname];
}

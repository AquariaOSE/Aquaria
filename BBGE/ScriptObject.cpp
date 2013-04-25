/*
Copyright (C) 2007, 2012 - Bit-Blot

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

#include <sstream>

#include "ScriptObject.h"

static const char *scriptObjTypeNames[] =
{
	/* (1 << 0) */ "RenderObject",
	/* (1 << 1) */ "Entity",
	/* (1 << 2) */ "Ingredient",
	/* (1 << 3) */ "CollideEntity",
	/* (1 << 4) */ "ScriptedEntity",
	/* (1 << 5) */ "Beam",
	/* (1 << 6) */ "Shot",
	/* (1 << 7) */ "Web",
	/* (1 << 8) */ "Bone",
	/* (1 << 9) */ "Path/Node",
	/* (1 <<10) */ "Quad",
	/* (1 <<11) */ "Text",
	/* (1 <<12) */ "PauseQuad",
	NULL
};

std::string ScriptObject::getTypeString(unsigned int ty)
{
	if (ty == SCO_NONE)
		return "NO TYPE";

	bool more = false;
	std::ostringstream os;
	for (int i = 0; scriptObjTypeNames[i]; ++i)
	{
		if (ty & (1 << i))
		{
			if (more)
				os << ", ";
			os << scriptObjTypeNames[i];
			more = true;
		}
	}
	return os.str();
}


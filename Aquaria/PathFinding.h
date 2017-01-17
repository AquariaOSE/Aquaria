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
#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "../BBGE/Base.h"

#include "TileVector.h"
#include <assert.h>

class RenderObject;
class SearchGrid;
class Game;

namespace PathFinding
{
	class State;

	void forceMinimumPath(VectorPath &path, const Vector &start, const Vector &dest);
	void molestPath(VectorPath &path);
	void generatePath(RenderObject *go, TileVector g1, TileVector g2, int offx=0, int offy=0);

	bool generatePathSimple(VectorPath& path, const Vector& start, const Vector& end, unsigned int step = 0, unsigned int obs = 0);

	State *initFindPath();
	void deleteFindPath(State *state);

	void beginFindPath(State *state, const Vector& start, const Vector& end, unsigned int obs = 0);
	bool updateFindPath(State *state, int limit);
	bool finishFindPath(State *state, VectorPath& path, unsigned step = 0);
	void getStats(State *state, unsigned& stepsDone, unsigned& nodesExpanded);
	void purgeFindPath(PathFinding::State *state);
}

#endif




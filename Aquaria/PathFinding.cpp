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

#include <jps.hh>
#include "PathFinding.h"
#include "DSQ.h"
#include "Game.h"


class SearchGridRaw
{
public:
	SearchGridRaw(ObsType blocking) : blockingObsBits(blocking) {}
	inline bool operator()(unsigned x, unsigned y) const
	{
		return (game->getGridRaw(TileVector(x, y)) & blockingObsBits) == OT_EMPTY;
	}

	ObsType blockingObsBits;
};

class PathFinding::State : public ScriptObject
{
public:
	State(ObsType obs) : grid(obs), searcher(grid), result(JPS_NO_PATH) {}
	SearchGridRaw grid;
	JPS::Searcher<SearchGridRaw> searcher;
	JPS_Result result;
};

static void generateVectorPath(const JPS::PathVector& rawpath, VectorPath& vp, int offx, int offy)
{
	for(JPS::PathVector::const_iterator it = rawpath.cbegin(); it != rawpath.cend(); ++it)
		vp.addPathNode(Vector((it->x*TILE_SIZE)+TILE_SIZE/2+offx, (it->y*TILE_SIZE)+TILE_SIZE/2)+offy, 0);
}


void PathFinding::forceMinimumPath(VectorPath &path, const Vector &start, const Vector &dest)
{
	if (path.getNumPathNodes() <= 2)
	{

		path.clear();
		path.addPathNode(start, 0);
		path.addPathNode(dest, 1);
	}
}

void PathFinding::molestPath(VectorPath &path)
{
	size_t sz=path.getNumPathNodes();
	if(!sz)
		return;

	size_t i = 0;
	// make normals
	std::vector<Vector> normals;
	normals.resize(sz);
	for (i = 0; i < sz; i++)
	{
		Vector node = path.getPathNode(i)->value;
		const int sample = 20;
		{
			Vector n = game->getWallNormal(node, sample);
			if (!n.isZero())
			{
				n.setLength2D(200);
				TileVector test(node + n);
				if (game->isObstructed(test))
				{
					n.setLength2D(100);
					test = TileVector(node+n);
					if (game->isObstructed(test))
					{
						n.setLength2D(50);
						test = TileVector(node+n);
						if (game->isObstructed(test))
						{
							n = Vector(0,0,0);
						}
					}
				}
				normals[i] = n;
			}
		}
	}

	// use wall normal to push out node a bit
	std::vector<Vector> newNormals;
	newNormals.resize(normals.size());
	if(normals.size() > 0) {
		for (i = 1; i < normals.size()-1; i++) {
			newNormals[i] = (normals[i] + normals[i-1] + normals[i+1])/3.0f;
		}
	}
	for (i = 1; i < sz-1; i++)
		path.getPathNode(i)->value += newNormals[i];

	// kill bowls
	int start = 0;
	int runs=0;
	bool hadSuccess = false;
	int lastSuccessNode = 0;
	int adjust = 2;
	sz=path.getNumPathNodes();

	for (i = start; i < sz-1; i++)
	{
		runs++;
		if (runs > 8000)
		{
			debugLog("kill bowls ran too much");
			start = sz*100;
		}
		lastSuccessNode = 0;
		hadSuccess = false;
		Vector node = path.getPathNode(i)->value;
		for (size_t j = sz-1; j >= i+adjust; j--)
		{
			Vector target = path.getPathNode(j)->value;
			if (game->trace(node, target))
			{
				hadSuccess = true;
				lastSuccessNode = j;
				break;
			}
		}
		if (hadSuccess)
		{
			// this code will only delete things that are bowl-ish
			// (things that take you on detours)
			++i;
			path.removeNodes(i, lastSuccessNode-1);
			hadSuccess = false;
		}
		sz = path.getNumPathNodes();
	}
	sz=path.getNumPathNodes();

	// remove last node
	//path.removeNodes(path.getNumPathNodes()-2, path.getNumPathNodes()-2);

	path.realPercentageCalc();
}

void PathFinding::generatePath(RenderObject *ro, TileVector start, TileVector goal, int offx, int offy)
{
	ro->position.ensureData();
	VectorPath& vp = ro->position.data->path;
	vp.clear();

	SearchGridRaw grid(OT_BLOCKING);
	JPS::PathVector path;
	if(JPS::findPath(path, grid, start.x, start.y, goal.x, goal.y, 1))
	{
		vp.addPathNode(ro->position, 0);
		generateVectorPath(path, vp, offx, offy);
	}
}

bool PathFinding::generatePathSimple(VectorPath& path, const Vector& start, const Vector& end, unsigned int step /* = 0 */, unsigned int obs /* = 0 */)
{
	if(obs == OT_EMPTY)
		obs = OT_BLOCKING;

	SearchGridRaw grid((ObsType)obs);
	JPS::PathVector p;
	TileVector tstart(start);
	TileVector tend(end);
	if(!JPS::findPath(p, grid, tstart.x, tstart.y, tend.x, tend.y, step))
		return false;

	generateVectorPath(p, path, 0, 0);
	molestPath(path);
	return true;
}

PathFinding::State *PathFinding::initFindPath()
{
	State *state = new PathFinding::State(OT_BLOCKING);
	return state;
}

void PathFinding::deleteFindPath(State *state)
{
	delete state;
}

void PathFinding::beginFindPath(PathFinding::State *state, const Vector& start, const Vector& end, unsigned int obs /* = 0 */)
{
	if(obs == OT_EMPTY)
		obs = OT_BLOCKING;

	state->grid.blockingObsBits = (ObsType)obs;
	TileVector tstart(start);
	TileVector tend(end);
	JPS::Position istart = JPS::Pos(tstart.x, tstart.y);
	JPS::Position iend = JPS::Pos(tend.x, tend.y);
	state->result = state->searcher.findPathInit(istart, iend);
}

bool PathFinding::updateFindPath(PathFinding::State *state, int limit)
{
	if(state->result == JPS_NEED_MORE_STEPS)
	{
		state->result = state->searcher.findPathStep(limit);
		return state->result != JPS_NEED_MORE_STEPS;
	}
	return true; // done
}

bool PathFinding::finishFindPath(PathFinding::State *state, VectorPath& path, unsigned step /* = 0 */)
{
	if(state->result != JPS_FOUND_PATH)
		return state->result == JPS_EMPTY_PATH;

	JPS::PathVector rawpath;
	state->searcher.findPathFinish(rawpath, step);
	generateVectorPath(rawpath, path, 0, 0);
	molestPath(path);
	return true;
}

void PathFinding::purgeFindPath(PathFinding::State *state)
{
	state->searcher.freeMemory();
}

void PathFinding::getStats(PathFinding::State *state, unsigned& stepsDone, unsigned& nodesExpanded)
{
	stepsDone = (unsigned)state->searcher.getStepsDone();
	nodesExpanded = (unsigned)state->searcher.getNodesExpanded();
}


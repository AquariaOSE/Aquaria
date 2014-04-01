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

#include <JPS.h>
#include "PathFinding.h"
#include "DSQ.h"
#include "Game.h"

class SearchGrid
{
public:
	SearchGrid() : game(dsq->game) {}
	inline bool operator()(unsigned x, unsigned y) const
	{
		return game->getGrid(TileVector(x, y)) == OT_EMPTY;
	}

private:
	const Game *game;
};

static void generateVectorPath(const JPS::PathVector& rawpath, VectorPath& vp, int offx, int offy)
{
	for(JPS::PathVector::const_iterator it = rawpath.begin(); it != rawpath.end(); ++it)
		vp.addPathNode(Vector((it->x*TILE_SIZE)+TILE_SIZE/2+offx, (it->y*TILE_SIZE)+TILE_SIZE/2)+offy, 0);
}


void PathFinding::forceMinimumPath(VectorPath &path, const Vector &start, const Vector &dest)
{
	if (path.getNumPathNodes() <= 2)
	{
		//debugLog(" Path is <= 2 nodes... setting up simple path");
		path.clear();
		path.addPathNode(start, 0);
		path.addPathNode(dest, 1);
	}
}

void PathFinding::molestPath(VectorPath &path)
{
	int sz=path.getNumPathNodes();
	if(!sz)
		return;

	int i = 0;
	// make normals
	std::vector<Vector> normals;
	normals.resize(sz);
	for (i = 0; i < sz; i++)
	{
		Vector node = path.getPathNode(i)->value;
		float dist;
		int sample = 20;
		float maxDist = sample * TILE_SIZE;
		{
			Vector n = dsq->game->getWallNormal(node, sample, &dist);
			if (dist != -1 && (n.x != 0 || n.y != 0))
			{
				n.setLength2D(200);
				TileVector test(node + n);
				if (dsq->game->isObstructed(test))
				{
					n.setLength2D(100);
					test = TileVector(node+n);
					if (dsq->game->isObstructed(test))
					{
						n.setLength2D(50);
						test = TileVector(node+n);
						if (dsq->game->isObstructed(test))
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
	for (i = 1; i < normals.size()-1; i++)
		newNormals[i] = (normals[i] + normals[i-1] + normals[i+1])/3.0f;
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
		for (int j = sz-3; j >= i+adjust; j--)
		{
			Vector target = path.getPathNode(j)->value;
			if (dsq->game->trace(node, target))
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

	SearchGrid grid;
	JPS::PathVector path;
	if(JPS::findPath(path, grid, start.x, start.y, goal.x, goal.y, 10))
	{
		vp.addPathNode(ro->position, 0);
		generateVectorPath(path, vp, offx, offy);
	}
}

bool PathFinding::generatePathSimple(VectorPath& path, const Vector& start, const Vector& end, unsigned int step /* = 0 */)
{
	SearchGrid grid;
	JPS::PathVector p;
	TileVector tstart(start);
	TileVector tend(end);
	if(!JPS::findPath(p, grid, tstart.x, tstart.y, tend.x, tend.y, step))
		return false;

	generateVectorPath(p, path, 0, 0);
	molestPath(path);
	return true;
}

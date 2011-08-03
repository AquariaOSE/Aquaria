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
#include "PathFinding.h"
#include "DSQ.h"
#include "Game.h"


const int divs = 6;
const int MAX_ZONES=1000;
const int MAX_STEPS = 5000;
const int cutOff = int((divs*divs)*0.75f);

namespace PathFindingGlobals
{
	// This isn't used by the current code, so I've commented it out to
	// save 4MB of RAM.  --achurch
	//int zones[MAX_ZONES][MAX_ZONES];

	MapSearchNode node_goal;
	MapSearchNode node_start;
	RenderObject *render_object;
	bool hate_diagonals;
}

float MapSearchNode::GoalDistanceEstimate( MapSearchNode &nodeGoal )
{	
	float xd = float( ( (float)x - (float)nodeGoal.x ) );
	float yd = float( ( (float)y - (float)nodeGoal.y) );

	return ((xd*xd) + (yd*yd));
	//return 0;
	/*
	int r = 10;
	float c = 0;
	for (int x = -r; x < r; x+=2)
	{
		for (int y = -r; y < r; y+=2)
		{
			if (dsq->game->getGrid(TileVector(this->x+x, this->y+y)))
			{
				//c+= r*TILE_SIZE
				c++;			
			}
		}
	}	
	return ((xd*xd) + (yd*yd)) + c * (2*TILE_SIZE);
	*/


	
	/*
	float xd = float( ( (float)x - (float)nodeGoal.x ) );
	float yd = float( ( (float)y - (float)nodeGoal.y) );
	int dist = ((xd*xd) + (yd*yd));
	return (int(dist/80)*80);
	*/
	
	//return ((xd*xd) + (yd*yd));
		
	

	

	// + c; //+ c * (2*TILE_SIZE);
}

bool MapSearchNode::IsGoal( MapSearchNode &nodeGoal )
{
	Vector v(x, y);
	Vector g(nodeGoal.x, nodeGoal.y);
	if (divs > 1)
	{
		if ((v - g).getSquaredLength2D() <= sqr(divs+1))
		{	
			// HACK: remember this
			//debugLog ("really close to the goal!");
			return true;
		}
	}
	if( int(x/divs) == int(nodeGoal.x/divs) && int(y/divs) == int(nodeGoal.y/divs))
	{
		return true;
	}

	return false;
}

// This generates the successors to the given Node. It uses a helper function called
// AddSuccessor to give the successors to the AStar class. The A* specific initialisation
// is done for each node internally, so here you just set the state information that
// is specific to the application
bool MapSearchNode::GetSuccessors( AStarSearch *astarsearch, MapSearchNode *parent_node )
{

	int parent_x = -1; 
	int parent_y = -1; 

	if( parent_node )
	{
		parent_x = parent_node->x;
		parent_y = parent_node->y;
	}
	

	MapSearchNode NewNode;

	int i = divs;
	// push each possible move except allowing the search to go backwards
	if ((GetMap (x-i, y) <= 0)  && !((parent_x == x-i) && (parent_y == y)))
	{
		NewNode = MapSearchNode( x-i, y );
		astarsearch->AddSuccessor( NewNode );
	}	

	if ((GetMap (x, y-i) <= 0)  && !((parent_x == x) && (parent_y == y-i)))
	{
		NewNode = MapSearchNode( x, y-i );
		astarsearch->AddSuccessor( NewNode );
	}	

	if ((GetMap (x+i, y) <= 0) && !((parent_x == x+i) && (parent_y == y)))
	{
		NewNode = MapSearchNode( x+i, y );
		astarsearch->AddSuccessor( NewNode );
	}
	
	if ((GetMap (x, y+i) <= 0) && !((parent_x == x) && (parent_y == y+i)))
	{
		NewNode = MapSearchNode( x, y+i );
		astarsearch->AddSuccessor( NewNode );
	}	

	if (!PathFindingGlobals::hate_diagonals)
	{
		if ((GetMap (x-i, y-i) < 1)  && !((parent_x == x-i) && (parent_y == y-i)))
		{
			NewNode = MapSearchNode( x-i, y-i );
			astarsearch->AddSuccessor( NewNode );
		}	

		if ((GetMap (x-i, y+i) < 1)  && !((parent_x == x-i) && (parent_y == y+i)))
		{
			NewNode = MapSearchNode( x-i, y+i );
			astarsearch->AddSuccessor( NewNode );
		}	

		if ((GetMap (x+i, y+i) <1) && !((parent_x == x+i) && (parent_y == y+i)))
		{
			NewNode = MapSearchNode( x+i, y+i );
			astarsearch->AddSuccessor( NewNode );
		}	

		if ((GetMap (x+i, y-i) < 1) && !((parent_x == x+i) && (parent_y == y-i)))
		{
			NewNode = MapSearchNode( x+i, y-i );
			astarsearch->AddSuccessor( NewNode );
		}	
	}

	return true;
}

// given this node, what does it cost to move to successor. In the case
// of our map the answer is the map terrain value at this node since that is 
// conceptually where we're moving
float MapSearchNode::GetCost( MapSearchNode &successor )
{
	float cost = 1;	
	/*
	if (PathFindingGlobals::hate_diagonals)
	{
		if (successor.x != x && successor.y != y)
		{
			cost = 0.1;
		}
	}
	*/
	
	//Vector p(x, y);
	//penalize moving towards obstructions
	/*
	int r = 20;
	float costy=0;
	int c = 0;
	float v = 0;
	float dist = sqr(r*TILE_SIZE);
	TileVector tme(this->x, this->y);
	for (int x = -r; x < r; x++)
	{
		for (int y = -r; y < r; y++)
		{
			TileVector t(this->x + x, this->y + y);
			if (dsq->game->isObstructed(t))
			{
				
				Vector diff = t.worldVector() - tme.worldVector();
				int d = diff.getSquaredLength2D();
				if (d < dist)
				{
					costy += 0.1;
				}

				//TileVector tme(this->x, this->y);
				//Vector diff = t.worldVector() - tme.worldVector();
				//int d = diff.getSquaredLength2D();
				//if (d < dist)
				//{
				//	v += dist-diff.getSquaredLength2D();
				//}
				
			}
			c++;
		}
	}
	cost += costy;
	*/
	/*
	if (v > 0)
	{
		v /= float(c);
		v /= float(dist);
		cost += v*TILE_SIZE*10;
	}
	*/

	
	
	
	//penalize changing direction to tempt computer into moving in "straighter" paths
	/*
	if (successor.y != y && (dir == LEFT || dir == RIGHT))
		cost +=39;
	if (successor.x != x && (dir == UP || dir == DOWN))
		cost +=39;
	*/
	
	return cost;
}

int MapSearchNode::GetMap (int tx, int ty)
{
	//return 0;
	//return PathFindingGlobals::zones[int(tx/divs)][int(ty/divs)] > cutOff;
	int v = dsq->game->getGrid(TileVector(tx,ty));
	/*
	if (v != 0 && v != 1)
	{
		std::ostringstream os;
		os << "v: " << v;
		debugLog(os.str());
	}
	*/
	return v;
	/*
	if (dsq->game->getGrid(TileVector(x,y))
		|| dsq->game->getGrid(TileVector(x+1,y))
		|| dsq->game->getGrid(TileVector(x-1,y))
		|| dsq->game->getGrid(TileVector(x+2,y))
		|| dsq->game->getGrid(TileVector(x-2,y))
		*/
	
	/*
	int r = 3;
	for (int x = -r; x < r; x++)
	{
		for (int y = -r; y < r; y++)
		{
			if (dsq->game->getGrid(TileVector(tx+x, ty+y)))
				return 1;
		}
	}
	return 0;
	*/

	/*
	// ignore the start node
	if (x == KittyTown::instance->nodeStart->x && y == KittyTown::instance->nodeStart->y)
	{
		obs = -1;
	}
	return obs;
	*/
}

// same state in a maze search is simply when (x,y) are the same
bool MapSearchNode::IsSameState( MapSearchNode &rhs )
{	
	if( (int(x/divs) == int(rhs.x/divs)) && (int(y/divs) == int(rhs.y/divs)) )
		return true;
	else
		return false;
}

void PathFinding::generateZones()
{
	return;

	/*
	for (int x = 0; x < MAX_ZONES; x++)
	{
		for (int y = 0; y < MAX_ZONES; y++)
		{
			PathFindingGlobals::zones[x][y] = 0;
		}
	}
	for (int x = 0; x < MAX_GRID; x+=divs)
	{
		for (int y = 0; y < MAX_GRID; y+=divs)
		{
			for (int xx = x; xx < x + divs; xx++)
			{
				for (int yy = y; yy < y + divs; yy++)
				{
					if (dsq->game->getGrid(TileVector(xx,yy)) > 0)
					{
						PathFindingGlobals::zones[int(x/divs)][int(y/divs)]++;
					}
				}
			}
		}
	}
	*/
}

void PathFinding::forceMinimumPath(VectorPath &path, const Vector &start, const Vector &dest)
{
	if (path.getNumPathNodes() <= 2)
	{
		debugLog(" Path is <= 2 nodes... setting up simple path");
		path.clear();
		path.addPathNode(start, 0);
		path.addPathNode(dest, 1);
	}
}

void PathFinding::molestPath(VectorPath &path)
{
	//path.cut(2);

	int sz=path.getNumPathNodes();

	/*
	//normals.resize(sz);

	*/
	//float maxDist = 15155;
	

	int i = 0;
	// make normals
	std::vector<Vector> normals;
	normals.resize(sz);
	for (i = 0; i < sz; i++)
	{
		Vector node = path.getPathNode(i)->value;
		float dist;
		/*
		float coverage = dsq->game->getCoverage(node, 100);
		int sample = 10;
		if (coverage > 0.4f)
			sample = 5;
		*/
		int sample = 20;
		float maxDist = sample * TILE_SIZE;
		//sqrtf(sqr(sample*TILE_SIZE)+sqr(sample*TILE_SIZE));
		
		//if (coverage < 0.6f)
		{
			Vector n = dsq->game->getWallNormal(node, sample, &dist);
			if (dist != -1 && (n.x != 0 || n.y != 0))
			{
				/*
				if (dist > maxDist)
					maxDist = dist;
				n *= (maxDist-dist); // *(1.0f-coverage);
				*/
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
				std::ostringstream os;
				os << "pushing node [" << i << "] out by (" << n.x << ", " << n.y << ") - dist: " << dist << " maxDist: " << maxDist;
				debugLog(os.str());
				//path.getPathNode(i)->value += n;
				normals[i] = n;
			}
			/*
			std::ostringstream os;
			os << "largest maxDist: " << maxDist;
			debugLog(os.str());
			*/
		}
	}
	
	std::vector<Vector> newNormals;
	newNormals.resize(normals.size());
	for (i = 1; i < normals.size()-1; i++)
	{
		
		// not doing smoothing!
		Vector thisOne = normals[i];
		
		Vector lastOne = normals[i-1];
		Vector nextOne = normals[i+1];
		newNormals[i] = (thisOne + lastOne + nextOne)/3.0f;
		
		//newNormals[i] = thisOne;
	}
	for (i = 1; i < sz-1; i++)
	{
		path.getPathNode(i)->value += newNormals[i];
	}


	// kill bowls
	int start = 0;
	//int minDist = 150;
	int runs=0;
	bool hadSuccess = false;
	int lastSuccessNode = 0;
	//int adjust = int(minDist/float(TILE_SIZE*8));
	int adjust = 2; // 1
//bowl_loop:
	sz=path.getNumPathNodes();

	std::ostringstream os;
	os << "kill bowls # " << runs;
	debugLog(os.str());
	
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
		//for (int j = i+adjust; j < sz-1; j++)
		{
			Vector target = path.getPathNode(j)->value;
			//if ((target-node).getSquaredLength2D() >= sqr(minDist))
			{
				if (dsq->game->trace(node, target))
				{				
					hadSuccess = true;
					lastSuccessNode = j;
					break;
				}
				/*
				else if (hadSuccess)
				{
					//break;
				}
				*/
			}
		}
		if (hadSuccess)
		{
			// only do this if
			//VectorPath copy = path.copySection(i,lastSuccessNode);
			/*
			// this code will only delete things that are bowl-ish
			// (things that take you on detours)
			float len = path.getSubSectionLength(i, lastSuccessNode);
			float shortCut = (path.getPathNode(lastSuccessNode)->value - path.getPathNode(i)->value).getLength2D();

			if (len > shortCut+TILE_SIZE*4)
			*/
			{
				path.removeNodes(i+1, lastSuccessNode-1);
				std::ostringstream os;
				os << "killing bowl: " << i+1 << " - " << lastSuccessNode-1;
				debugLog(os.str());
				//start = lastSuccessNode - (lastSuccessNode-i);
				//start = i+1;
				//i = i+1;
				i++;
			}
			hadSuccess = false;
			//start += 2;
			//goto bowl_loop;
		}
		sz = path.getNumPathNodes();
	}
	debugLog("kill bowls done");
	sz=path.getNumPathNodes();

	// remove last node
	path.removeNodes(path.getNumPathNodes()-2, path.getNumPathNodes()-2);
	
	/*
loop:
	for (int i = 0; i < sz-2; i++)
	{
		Vector node = path.getPathNode(i)->value;
		Vector next = path.getPathNode(i+1)->value;
		Vector next2 = path.getPathNode(i+2)->value;
		int dist1 = (next - node).getSquaredLength2D() + (next2 - next).getSquaredLength2D();
		int dist2 = (next2 - node).getSquaredLength2D();
		if (dist2 <= dist1)
		{
			// remove next
			path.removeNode(i+1);
			goto loop;
		}	
	}
	*/

	path.realPercentageCalc();
	//path.calculatePercentages();
	/*
	int sz=path.getNumPathNodes();
	std::vector<Vector> normals;
	normals.resize(sz);
	for (int i = 1; i < sz-1; i++)
	{
		Vector node = path.getPathNode(i)->value;
		Vector normal = dsq->game->getWallNormal(node, 10);
		if (normal.x != 0 && normal.y != 0)
		{
			normal = normal*TILE_SIZE*10;
		}
		normals[i] = normal;
		//path.getPathNode(i)->value = node;
	}
	for (int i = 1; i < sz-1; i++)
	{
		Vector normal = normals[i];
		Vector lastNormal = normals[i-1];

		//Vector node = path.getPathNode(i)->value;
		//// average with the 
		//Vector prev = path.getPathNode(i-1)->value;
		//Vector next = path.getPathNode(i+1)->value;

		//node = (node + prev)/2.0f;

		normal = (normal + lastNormal)/2.0f;
		path.getPathNode(i)->value += normal;
	}
	*/
	/*
	for (int i = 1; i < sz; i++)
	{
		Vector node = path.getPathNode(i)->value;
		Vector p0 = path.getPathNode(i-1)->value;
		Vector p1 = path.getPathNode(i)->value;
		Vector p = p1 - p0;
		if (i < sz-1)
		{
			p += path.getPathNode(i+1)->value - path.getPathNode(i)->value;
			p /= 2.0f;
		}
		Vector pl = p.getPerpendicularLeft();
		Vector pr = p.getPerpendicularRight();
		pl.normalize2D();
		pr.normalize2D();
		TileVector tl(node), tr(node);
		int left, right;
		int maxCheck = 40;
		for (left = 0; left < maxCheck; left++)
		{
			if (dsq->game->isObstructed(tl))
				break;
			tl.x += pl.x;
			tl.y += pl.y;
		}
		for (right = 0; right < maxCheck; right++)
		{
			if (dsq->game->isObstructed(tr))
				break;
			tr.x += pr.x;
			tr.y += pr.y;
		}
		if (left == maxCheck && right == maxCheck)
		{
			continue;
		}
		else if (left != 0 || right != 0)
		{
			//Vector normal = dsq->game->getWallNormal(node);
			//if (normal.x != 0 && normal.y != 0)
			//{
			//	if (left < right)
			//		path.getPathNode(i)->value += normal * (right-left)*TILE_SIZE;
			//	if (right > left)
			//		path.getPathNode(i)->value += normal * (left-right)*TILE_SIZE;
			//}

			
			//int leftSz = left * TILE_SIZE;
			////if (leftSz <= 0) leftSz = 1;
			//int rightSz = right * TILE_SIZE;
			////if (rightSz <= 0) rightSz = 1;
			//pl |= leftSz;
			//pr |= rightSz;
			//
			

			path.getPathNode(i)->value = (tr.worldVector() + tl.worldVector())/2.0f;

			//path.getPathNode(i)->value = tl.worldVector() + (tr.worldVector() - tl.worldVector())/2.0f;//(node + pl) + (pr-pl)/2.0f;
			path.getPathNode(i)->value.z = 0;
		}
	}
	*/

	/*
	for (int i = 1; i < sz; i++)
	{
		Vector node = path.getPathNode(i)->value;
		Vector pl = p.getPerpendicularLeft();
		Vector pr = p.getPerpendicularRight();
		pl.normalize2D();
		pr.normalize2D();
		TileVector tl(node), tr(node);
		int left, right;
		int maxCheck = 40;
		for (int i = 0; i < maxCheck; i++)
		{
			dsq->game->position
		}
		if (left == maxCheck && right == maxCheck)
		{
			continue;
		}
		else if (left != 0 || right != 0)
		{
			//Vector normal = dsq->game->getWallNormal(node);
			//if (normal.x != 0 && normal.y != 0)
			//{
			//	if (left < right)
			//		path.getPathNode(i)->value += normal * (right-left)*TILE_SIZE;
			//	if (right > left)
			//		path.getPathNode(i)->value += normal * (left-right)*TILE_SIZE;
			//}

			
			//int leftSz = left * TILE_SIZE;
			////if (leftSz <= 0) leftSz = 1;
			//int rightSz = right * TILE_SIZE;
			////if (rightSz <= 0) rightSz = 1;
			//pl |= leftSz;
			//pr |= rightSz;
			//
			

			path.getPathNode(i)->value = (tr.worldVector() + tl.worldVector())/2.0f;

			//path.getPathNode(i)->value = tl.worldVector() + (tr.worldVector() - tl.worldVector())/2.0f;//(node + pl) + (pr-pl)/2.0f;
			path.getPathNode(i)->value.z = 0;
		}
	}
	*/
	
}

void PathFinding::generatePath(RenderObject *ro, TileVector start, TileVector goal, int offx, int offy, bool hate_diagonals)
{
	//return;

	int sx = start.x;
	int sy = start.y;
	int gx = goal.x;
	int gy = goal.y;
	

	PathFindingGlobals::hate_diagonals = hate_diagonals;
	/*
	if (offx >= TILE_SIZE/2-1)
		offx--;
	if (offy >= TILE_SIZE/2-1)
		offy--;
	if (offx <= TILE_SIZE/2+1)
		offx++;
	if (offy <= TILE_SIZE/2+1)
		offy++;
	*/
	ro->position.ensureData();
	ro->position.data->path.clear();

	PathFindingGlobals::render_object = ro;
	AStarSearch astarsearch;

	// Create a start state
	MapSearchNode nodeStart;
	nodeStart.x = sx;
	nodeStart.y = sy;
	PathFindingGlobals::node_start = nodeStart;

	if (nodeStart.GetMap(gx, gy) > 0)
	{
		std::ostringstream os;
		os << "goal (" << gx << ", " << gy << ") blocked";
		debugLog (os.str());
		return;
	}

	// Define the goal state
	
	MapSearchNode nodeEnd;
	nodeEnd.x = gx;		
	nodeEnd.y = gy; 

	PathFindingGlobals::node_goal = nodeEnd;
	
	// Set Start and goal states
	
	astarsearch.SetStartAndGoalStates( nodeStart, nodeEnd );

	unsigned int SearchState;
	unsigned int SearchSteps = 0;

	do
	{
		SearchState = astarsearch.SearchStep();

		if (SearchState != AStarSearch::SEARCH_STATE_SEARCHING)
			break;

		SearchSteps++;

		if (SearchSteps > MAX_STEPS) break;
	}
	while( SearchState == AStarSearch::SEARCH_STATE_SEARCHING );

	if( SearchState == AStarSearch::SEARCH_STATE_SUCCEEDED )
	{
		//errorLog ("Search found goal state",0);

			MapSearchNode *node = astarsearch.GetSolutionStart();
			int steps = 0;

			//node->PrintNodeInfo();
			ro->position.data->path.addPathNode(Vector((node->x*TILE_SIZE)+TILE_SIZE/2+offx, (node->y*TILE_SIZE)+TILE_SIZE/2)+offy, 0);
			for( ;; )
			{
				node = astarsearch.GetSolutionNext();

				if( !node )
				{
					break;
				}

				//node->PrintNodeInfo();
				ro->position.data->path.addPathNode(Vector((node->x*TILE_SIZE)+TILE_SIZE/2+offx, (node->y*TILE_SIZE)+TILE_SIZE/2)+offy, steps);
				steps ++;
			};
			//ro->position.path.addPathNode(Vector(goal.x*TILE_SIZE, goal.y*TILE_SIZE), steps);
			/*
			std::ostringstream os;
			os << "Solution steps " << steps;
			msg(os.str());
			*/
			
			// Once you're done with the solution you can free the nodes up
			astarsearch.FreeSolutionNodes();
	}
	else if( SearchState == AStarSearch::SEARCH_STATE_FAILED )
	{
		debugLog("Search terminated. Did not find goal state");
		
		//astarsearch.FreeSolutionNodes();
		astarsearch.FreeStartAndGoalNodes();
	}
	else
	{
		// exceeded count
		debugLog("Path too long");
		
		astarsearch.FreeAllNodes();
		astarsearch.FreeStartAndGoalNodes();
		//astarsearch.FreeSolutionNodes();		
	}

	if (astarsearch.m_AllocateNodeCount != astarsearch.m_FreeNodeCount)
	{
		debugLog("astar memory leak");
	}
	//return path_vector;
}



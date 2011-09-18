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
#include "Game.h"
#include "Avatar.h"
#include "GridRender.h"
#include "Gradient.h"
#include "TTFFont.h"
#include "RoundedRect.h"

#define GEM_GRAB			 10

namespace WorldMapRenderNamespace
{
	const float WORLDMAP_UNDERLAY_ALPHA = 0.8;

	float baseMapSegAlpha		= 0.4;
	float visibleMapSegAlpha	= 0.8;

	const float blinkPeriod		= 0.2;

	// Fraction of the screen width and height we consider "visited".
	// (We don't mark the entire screen "visited" because the player may
	// overlook things on the edge of the screen while moving.)
	const float visitedFraction	= 0.8;

	enum VisMethod
	{
		VIS_VERTEX		= 0,
		VIS_WRITE		= 1
	};

	VisMethod visMethod = VIS_VERTEX;

	std::vector<Quad *> tiles;

	std::vector<Quad *> particles;

	Quad *activeQuad=0, *lastActiveQuad=0, *originalActiveQuad=0;
	Quad *lastVisQuad=0, *visQuad=0;
	WorldMapTile *lastVisTile=0;

	float xMin, yMin, xMax, yMax;

	float zoomMin = 0.2;
	float zoomMax = 1;
	const float exteriorZoomMax = 1;
	const float interiorZoomMax = 1.8;

	bool editorActive=false;

	Quad *tophud=0;
	
	Gradient *underlay = 0;
}

using namespace WorldMapRenderNamespace;

std::vector <Quad*> grid;

class GemMover;

GemMover *mover=0;

WorldMapTile *activeTile=0;

const float beaconSpawnBitTime = 0.05;

class WorldMapBoundQuad : public Quad
{
public:
	WorldMapBoundQuad(const Vector &posToUse)
	{
		position = posToUse;
		truePosition = posToUse;
		followCamera = 1;
	}

	void render()
	{
		setProperPosition();
		Quad::render();
	}

protected:
	Vector truePosition;
	void setProperPosition()
	{
		Vector wp = parent->getWorldCollidePosition(truePosition);
		Vector diff = wp - core->center;

		float w2 = core->getVirtualWidth()/2;
		float h2 = core->getVirtualHeight()/2;

		if (diff.x < -w2)
			wp.x = core->center.x - w2;
		if (diff.x > w2)
			wp.x = core->center.x + w2;
		if (diff.y < -h2)
			wp.y = core->center.y - h2;
		if (diff.y > h2)
			wp.y = core->center.y + h2;

		Vector move = wp - getWorldPosition();
		// FIXME: This is a quick HACK to get the current world map
		// scale factor so we can set the position properly, without
		// having to play with multiple levels of parent pointers or
		// anything like that.  (If we don't scale the move vector
		// properly, the dots overshoot at high zoom or don't go far
		// enough at low zoom and we can end up with a weird "disco"
		// effect -- see icculus bug 4542.)
		const float x0 = getWorldPosition().x;
		position.x += 1;
		const float x1 = getWorldPosition().x;
		position.x -= 1;
		position += move / (x1-x0);
	}
};

class BeaconRender : public Quad
{
public:
	BeaconRender(BeaconData *beaconData) : Quad(), beaconData(beaconData)
	{
		renderQuad = false;
		setTexture("gui/minimap/ripple"); //"particles/softring"); // or whatever
		position = beaconData->pos;
		truePosition = beaconData->pos;
		followCamera = 1;
		/*
		scale = Vector(0.2, 0.2);
		scale.interpolateTo(Vector(0.5, 0.5), 0.75, -1, 1, 1);
		*/
		alpha = 0.5;
		color = beaconData->color;

		/*
		pe = new ParticleEffect();
		pe->load("sparkle");
		pe->followCamera = 1;
		pe->start();
		core->addRenderObject(pe, LR_PARTICLES);
		*/
		spawnBitTimer = 0;
	}
	
	//float spawnBitTimer;
	Vector truePosition;

	ParticleEffect *pe;
	
	float spawnBitTimer;
	
	
	void render()
	{
		//setProperPosition();
		Quad::render();
	}
	
protected:
	BeaconData *beaconData;
	
	void setProperPosition()
	{
		Vector wp = parent->getWorldCollidePosition(truePosition);
		Vector diff = wp - core->center;

		float w2 = core->getVirtualWidth()/2;
		float h2 = core->getVirtualHeight()/2;

		if (diff.x < -w2)
			wp.x = core->center.x - w2;
		if (diff.x > w2)
			wp.x = core->center.x + w2;
		if (diff.y < -h2)
			wp.y = core->center.y - h2;
		if (diff.y > h2)
			wp.y = core->center.y + h2;

		Vector move = wp - getWorldPosition();
		position += move;
	}
	
	
	void onUpdate(float dt)
	{
		Quad::onUpdate(dt);
		
		//setProperPosition();

		if (!dsq->game->worldMapRender->isOn()) return;

		const int lenRange = 125;
		const float pscale = 0.7;

		float leftOver = dt;

		while (leftOver)
		{
			spawnBitTimer -= leftOver;
			if (spawnBitTimer <= 0)
			{
				leftOver = -spawnBitTimer;
				spawnBitTimer = beaconSpawnBitTime;

				float r = (rand()%100)/100.0f;
				float radius = r * 2*PI;
				float len = (rand()%lenRange);
				int x = sinf(radius)*len;
				int y = cosf(radius)*len;

				//truePosition +
				float t = 0.75;
				WorldMapBoundQuad *q = new WorldMapBoundQuad(Vector(x, y, 0));
				q->setTexture("particles/glow");
				q->alpha.ensureData();
				q->alpha.data->path.addPathNode(0.0, 0.0);
				q->alpha.data->path.addPathNode(1.0, 0.5);
				q->alpha.data->path.addPathNode(0.0, 1.0);
				q->alpha.startPath(0.5);
				q->alphaMod = 0.5;
				q->color = color;

				q->scale = Vector(pscale, pscale);
				//q->fadeAlphaWithLife = 1;
				q->setLife(1);
				q->setDecayRate(1.0f/t);
				
				q->setBlendType(BLEND_ADD);
				addChild(q, PM_POINTER);

				std::ostringstream os;
				os << "children: " << children.size();
				debugLog(os.str());
			}
			else
			{
				leftOver = 0;
			}
		}



		/*

		*/
	}

};

class GemMover : public Quad
{
public:
	GemMover(GemData *gemData) : Quad(), gemData(gemData)
	{
		setTexture("Gems/" + gemData->name);
		position = gemData->pos;
		followCamera = 1;
		blink = false;
		blinkTimer = 0;
		canMove = gemData->canMove;
		//canMove = true;
		//gemData->userString = "HI THERE!";

		std::string useString = gemData->userString;

		/*
		if (gemData->userString.empty())
		{
			if (gemData->name == "savepoint")
				useString = "Memory Crystal";
			if (gemData->name == "cook")
				useString = "Kitchen";
		}
		*/

		/*
		text = new BitmapText(&dsq->smallFont);
		text->position = Vector(0, -20);
		text->setText(gemData->userString);
		addChild(text, PM_POINTER);
		*/

		text = new TTFText(&dsq->fontArialSmall);//new DebugFont(10, useString);
		text->offset = Vector(0, 4); //Vector(0,5);
		text->setText(useString);
		text->setAlign(ALIGN_CENTER);

		textBG = new RoundedRect();
		textBG->setWidthHeight(text->getWidth() + 20, 25, 10);  // 30
		textBG->alpha = 0;
		textBG->followCamera = 1;
		game->addRenderObject(textBG, LR_WORLDMAPHUD);

		textBG->addChild(text, PM_POINTER);
		//game->addRenderObject(text, LR_WORLDMAPHUD);
	}

	void destroy()
	{
		if (textBG)
		{
			textBG->safeKill();
			textBG = 0;
		}
		Quad::destroy();
	}

	GemData *getGemData() { return gemData; }

	void setBlink(bool blink)
	{
		this->blink = blink;

		//if (blink)
		//{
		//	scale = Vector(0.5, 0.5);
		//	scale.interpolateTo(Vector(1,1), 0.5, -1, 1);
		//	/*
		//	alpha = 0.5;
		//	alpha.interpolateTo(1, 0.5, -1, 1);
		//	*/
		//}
		//else
		//{
		//	scale.stop();
		//	scale = Vector(1,1);
		//	/*
		//	alpha.stop();
		//	alpha = 1;
		//	*/
		//}
	}
	bool canMove;
protected:
	
	float blinkTimer;
	bool blink;
	GemData *gemData;
	//BitmapText *text;
	TTFText *text;
	RoundedRect *textBG;
	void onUpdate(float dt)
	{
		Quad::onUpdate(dt);

		Vector sz = parent->scale;

		if (sz.x < zoomMin)
			sz.x = sz.y = zoomMin;
		if (sz.x > zoomMax)
			sz.x = sz.y = zoomMax;
		
		if (sz.x > 1.0f)
		{
			scale.x = (1.0f/sz.x);
			scale.y = (1.0f/sz.y);
		}
		else
		{
			scale = Vector(1,1);
		}

		Vector wp = getWorldPosition();

		if (blink)
		{
			blinkTimer += dt;
			if (blinkTimer > blinkPeriod)
			{
				if (alphaMod == 0)
					alphaMod = 1;
				else
					alphaMod = 0;

				blinkTimer = 0;
			}
		}

		if (canMove)
		{
			if (mover == 0)
			{
				if (core->mouse.buttons.left && (core->mouse.position - wp).isLength2DIn(GEM_GRAB))
				{
					core->sound->playSfx("Gem-Move");
					mover = this;
					//offset = Vector(position - core->mouse.position);
					//position += core->mouse.position - wp;
					//offset = Vector(0, 4);
				}
			}
			else if (mover == this)
			{
				//position = core->mouse.position;
				position += (core->mouse.position - wp)/parent->scale.x;
				if (!core->mouse.buttons.left)
				{
					mover = 0;
					core->sound->playSfx("Gem-Place");
					//position += offset;
					//offset = Vector(0,0);
					//offset = Vector(0,0);
					gemData->pos = position;
				}
			}
		}


		if (textBG)
		{
			textBG->position = getWorldPosition() + Vector(0, -20);
		}

		if ((core->mouse.position - wp).isLength2DIn(GEM_GRAB))
		{
			//text->alpha.interpolateTo(1, 0.1);
			/*
			if (!gemData->userString.empty())
			textBG->alpha.interpolateTo(1, 0.1);
			*/
			if (!gemData->userString.empty())
				textBG->show();
		}
		else
		{
			/*
			text->alpha.interpolateTo(0, 0.1);
			textBG->alpha.interpolateTo(0, 0.1);
			*/
			if (textBG->alpha == 1)
				textBG->hide();
		}
	}
};

typedef std::list <GemMover*> GemMovers;
GemMovers gemMovers;

typedef std::list <BeaconRender*> BeaconRenders;
BeaconRenders beaconRenders;

std::vector<Quad*> quads;

void WorldMapRender::setProperTileColor(WorldMapTile *tile)
{
	if (tile)
	{
		if (tile->q)
		{
			if (!tile->revealed)
				tile->q->alphaMod = 0;

			if (tile->revealed)
				tile->q->alphaMod = 0.5;

			if (activeTile && (tile->layer != activeTile->layer || (tile->layer > 0 && activeTile != tile)))
				tile->q->alphaMod *= 0.5;

			tile->q->color = Vector(0.7, 0.8, 1);
		}
		else
		{
			debugLog("no Q!");
		}
	}
}

#ifdef AQUARIA_BUILD_MAPVIS

static void tileDataToVis(WorldMapTile *tile, Vector **vis)
{
	const unsigned char *data = tile->getData();

	if (data != 0)
	{
		const unsigned int rowSize = MAPVIS_SUBDIV/8;
		for (unsigned int y = 0; y < MAPVIS_SUBDIV; y++, data += rowSize)
		{
			for (unsigned int x = 0; x < MAPVIS_SUBDIV; x += 8)
			{
				unsigned char dataByte = data[x/8];
				for (unsigned int x2 = 0; x2 < 8; x2++)
				{
					vis[x+x2][y].z = (dataByte & (1 << x2)) ? visibleMapSegAlpha : baseMapSegAlpha;
				}
			}
		}
	}
	else
	{
		for (int x = 0; x < MAPVIS_SUBDIV; x++)
		{
			for (int y = 0; y < MAPVIS_SUBDIV; y++)
			{
				vis[x][y].z = baseMapSegAlpha;
			}
		}
		return;
	}
}

// Returns a copy of the original texture data.
static unsigned char *tileDataToAlpha(WorldMapTile *tile)
{
	const unsigned char *data = tile->getData();
	const unsigned int ab = int(baseMapSegAlpha * (1<<8) + 0.5f);
	const unsigned int av = int(visibleMapSegAlpha * (1<<8) + 0.5f);

	const unsigned int texWidth = tile->q->texture->width;
	const unsigned int texHeight = tile->q->texture->height;
	if (texWidth % MAPVIS_SUBDIV != 0 || texHeight % MAPVIS_SUBDIV != 0)
	{
		std::ostringstream os;
		os << "Texture size " << texWidth << "x" << texHeight
		   << " not a multiple of MAPVIS_SUBDIV " << MAPVIS_SUBDIV
		   << ", can't edit";
		debugLog(os.str());
		return 0;
	}
	const unsigned int scaleX = texWidth / MAPVIS_SUBDIV;
	const unsigned int scaleY = texHeight / MAPVIS_SUBDIV;

	unsigned char *savedTexData = new unsigned char[texWidth * texHeight * 4];
	tile->q->texture->read(0, 0, texWidth, texHeight, savedTexData);

	unsigned char *texData = new unsigned char[texWidth * texHeight * 4];
	memcpy(texData, savedTexData, texWidth * texHeight * 4);

	if (data != 0)
	{
		const unsigned int rowSize = MAPVIS_SUBDIV/8;
		for (unsigned int y = 0; y < MAPVIS_SUBDIV; y++, data += rowSize)
		{
			unsigned char *texOut = &texData[(y*scaleY) * texWidth * 4];
			for (unsigned int x = 0; x < MAPVIS_SUBDIV; x += 8)
			{
				unsigned char dataByte = data[x/8];
				for (unsigned int x2 = 0; x2 < 8; x2++, texOut += scaleX*4)
				{
					const bool visited = (dataByte & (1 << x2)) != 0;
					const unsigned int alphaMod = visited ? av : ab;
					for (unsigned int pixelY = 0; pixelY < scaleY; pixelY++)
					{
						unsigned char *ptr = &texOut[pixelY * texWidth * 4];
						for (unsigned int pixelX = 0; pixelX < scaleX; pixelX++, ptr += 4)
						{
							if (ptr[3] == 0)
								continue;
							ptr[3] = (ptr[3] * alphaMod + 128) >> 8;
						}
					}
				}
			}
		}
	}
	else
	{
		unsigned char *texOut = texData;
		for (unsigned int y = 0; y < texHeight; y++)
		{
			for (unsigned int x = 0; x < texWidth; x++, texOut += 4)
			{
				texOut[3] = (texOut[3] * ab + 128) >> 8;
			}
		}
	}

	tile->q->texture->write(0, 0, texWidth, texHeight, texData);
	delete[] texData;

	return savedTexData;
}

static void resetTileAlpha(WorldMapTile *tile, const unsigned char *savedTexData)
{
	tile->q->texture->write(0, 0, tile->q->texture->width, tile->q->texture->height, savedTexData);
}

#endif  // AQUARIA_BUILD_MAPVIS


void WorldMapRender::setVis(WorldMapTile *tile)
{
	if (!tile) return;
#ifdef AQUARIA_BUILD_MAPVIS
	/*
	if (lastVisQuad)
	{
		lastVisQuad->alphaMod = 0.5;
		lastVisQuad->color = Vector(0.7, 0.8, 1);
	}
	*/

	tile->q->color = Vector(1,1,1);
	tile->q->alphaMod = 1;
	
	if (visMethod == VIS_VERTEX)
	{
		tile->q->setSegs(MAPVIS_SUBDIV, MAPVIS_SUBDIV, 0, 0, 0, 0, 2.0, 1);
		tileDataToVis(tile, tile->q->getDrawGrid());
	}
	else if (visMethod == VIS_WRITE)
	{
		savedTexData = tileDataToAlpha(tile);
	}

	lastVisQuad = tile->q;
	lastVisTile = tile;
#endif
}

void WorldMapRender::clearVis(WorldMapTile *tile)
{
	if (!tile) return;
#ifdef AQUARIA_BUILD_MAPVIS
	if (visMethod == VIS_VERTEX)
	{
		if (tile->q)
			tile->q->deleteGrid();
	}
	else if (visMethod == VIS_WRITE)
	{
		if (savedTexData)
		{
			resetTileAlpha(tile, savedTexData);
			delete[] savedTexData;
			savedTexData = 0;
		}
	}
#endif
}


WorldMapRender::WorldMapRender() : RenderObject(), ActionMapper()
{
	doubleClickTimer = 0;
	inputDelay = 0;
	editorActive=false;
	mb = false;
	activeQuad=0;
	lastActiveQuad=0;
	originalActiveQuad=0;
	lastVisQuad=0;
	visQuad=0;
	lastVisTile=0;

	originalActiveTile = activeTile = 0;

	areaLabel = 0;

	on = false;
	alpha = 0;

	scale = Vector(1, 1);
	followCamera = 1;
	cull = false;
	position = Vector(400,300);

	activeTile = 0;
	activeQuad = 0;

	lastMousePosition = core->mouse.position;

	bg = 0;

	savedTexData = 0;

	/*
	bg = new Quad("", Vector(400,300));
	bg->setWidthHeight(810, 610);
	bg->setSegs(32, 32, 0.5, 0.5, 0.008, 0.008, 2.0, 1);
	bg->alphaMod = 0.5;
	bg->alpha = 0;
	bg->followCamera = 1;
	bg->repeatTextureToFill(true);
	//bg->parentManagedPointer = 1;
	dsq->game->addRenderObject(bg, LR_MESSAGEBOX);

	bg->renderQuad = false;
	*/

	int num = dsq->continuity.worldMap.getNumWorldMapTiles();
	std::string n = dsq->game->sceneName;
	stringToUpper(n);
	for (int i = 0; i < num; i++)
	{
		WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
		if (tile)
		{
			if (tile->name == n)
			{
				activeTile = tile;
				break;
			}
		}
	}

	tiles.clear();

	for (int i = 0; i < num; i++)
	{
		WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
		if (tile)
		{
			Vector pos(tile->gridPos.x, tile->gridPos.y);

			Quad *q = new Quad;
			std::string tn = "Gui/WorldMap/" + tile->name;
			q->setTexture(tn);
			q->position = pos;
			q->alphaMod = 0;

			tile->q = q;

			setProperTileColor(tile);
			
			q->setWidthHeight(q->getWidth()*tile->scale, q->getHeight()*tile->scale);
			q->scale = Vector(0.25f*tile->scale2, 0.25f*tile->scale2);

			if (tile == activeTile)
				activeQuad = q;

			if (activeQuad == q)
			{
				setVis(tile);
			}
		
			addChild(q, PM_POINTER);

			tiles.push_back(q);
		}
	}
	shareAlphaWithChildren = 1;

	dsq->user.control.actionSet.importAction(this, "SwimLeft",	ACTION_SWIMLEFT);
	dsq->user.control.actionSet.importAction(this, "SwimRight", ACTION_SWIMRIGHT);
	dsq->user.control.actionSet.importAction(this, "SwimUp",	ACTION_SWIMUP);
	dsq->user.control.actionSet.importAction(this, "SwimDown",	ACTION_SWIMDOWN);

	// where old scale + position set were

	tophud = new Quad("gui/worldmap-ui", Vector(400,64));
	tophud->followCamera = 1;
	tophud->alpha = 0;
	dsq->game->addRenderObject(tophud, LR_WORLDMAPHUD);

	//int fontSize = 6;
	float aly = 26, aly2 = 18;
	float sz = 0.6;

	//hover
	areaLabel = new BitmapText(&dsq->smallFont);
	areaLabel->scale = Vector(sz,sz);
	//areaLabel->setFontSize(fontSize);
	areaLabel->setAlign(ALIGN_CENTER);
	areaLabel->followCamera = 1;
	areaLabel->position = Vector(150,aly);
	dsq->game->addRenderObject(areaLabel, LR_WORLDMAPHUD);
	areaLabel->alpha = 0;

	//in
	areaLabel2 = new BitmapText(&dsq->smallFont);
	//areaLabel2->setFontSize(fontSize);
	areaLabel2->scale = Vector(sz,sz);
	areaLabel2->followCamera = 1;
	areaLabel2->setAlign(ALIGN_CENTER);
	areaLabel2->position = Vector(400,aly2);
	dsq->game->addRenderObject(areaLabel2, LR_WORLDMAPHUD);
	areaLabel2->alpha = 0;

	//select
	areaLabel3 = new BitmapText(&dsq->smallFont);
	areaLabel3->scale = Vector(sz,sz);
	//areaLabel3->setFontSize(fontSize);
	areaLabel3->followCamera = 1;
	areaLabel3->setAlign(ALIGN_CENTER);
	areaLabel3->position = Vector(650, aly);
	areaLabel3->alpha = 0;
	dsq->game->addRenderObject(areaLabel3, LR_WORLDMAPHUD);

	if (activeTile)
	{
		areaLabel2->setText(dsq->continuity.stringBank.get(activeTile->stringIndex));
	}

	originalActiveTile = activeTile;

	bindInput();
	
	underlay = new Gradient;
	//underlay->makeVertical(Vector(0.5,0.5,1), Vector(0,0,0.5));
	underlay->makeVertical(Vector(0.25,0.25,0.5), Vector(0,0,0.25));
	underlay->position = Vector(400,300);
	underlay->autoWidth = AUTO_VIRTUALWIDTH;
	underlay->autoHeight = AUTO_VIRTUALHEIGHT;
	underlay->followCamera = 1;
	underlay->alpha = 0;
	dsq->game->addRenderObject(underlay, LR_HUDUNDERLAY);

	addHintQuad1 = new Quad("gems/pyramidyellow", Vector(0,0));
	addHintQuad1->followCamera = 1;
	addHintQuad1->alpha = 0;
	dsq->game->addRenderObject(addHintQuad1, LR_WORLDMAPHUD);

	addHintQuad2 = new Quad("gems/pyramidpurple", Vector(0,0));
	addHintQuad2->followCamera = 1;
	addHintQuad2->alpha = 0;
	dsq->game->addRenderObject(addHintQuad2, LR_WORLDMAPHUD);

	//helpButton->event.set(MakeFunctionEvent(WorldMapRender, onToggleHelpScreen));
	helpButton = new AquariaMenuItem;
	helpButton->event.setActionMapperCallback(this, ACTION_TOGGLEHELPSCREEN, 0);
	helpButton->useQuad("gui/icon-help");
	helpButton->useGlow("particles/glow", 40, 40);
	helpButton->useSound("Click");
	helpButton->alpha = 0;
	//helpButton->position = Vector(800-20, 20);
	dsq->game->addRenderObject(helpButton, LR_WORLDMAPHUD);
}

void WorldMapRender::onToggleHelpScreen()
{
	game->onToggleHelpScreen();
}

void WorldMapRender::bindInput()
{
	clearActions();
	clearCreatedEvents();

	addAction(ACTION_TOGGLEWORLDMAPEDITOR, KEY_TAB);

	dsq->user.control.actionSet.importAction(this, "PrimaryAction",		ACTION_PRIMARY);
	dsq->user.control.actionSet.importAction(this, "SecondaryAction",	ACTION_SECONDARY);

	dsq->user.control.actionSet.importAction(this, "SwimLeft",			ACTION_SWIMLEFT);
	dsq->user.control.actionSet.importAction(this, "SwimRight",			ACTION_SWIMRIGHT);
	dsq->user.control.actionSet.importAction(this, "SwimUp",			ACTION_SWIMUP);
	dsq->user.control.actionSet.importAction(this, "SwimDown",			ACTION_SWIMDOWN);
}

void WorldMapRender::destroy()
{
	clearVis(activeTile);
	RenderObject::destroy();
	delete[] savedTexData;
}

bool WorldMapRender::isCursorOffHud()
{
	if (helpButton && helpButton->isCursorInMenuItem())
	{
		return false;
	}
	return true;
}

void WorldMapRender::onUpdate(float dt)
{
	if (AquariaGuiElement::currentGuiInputLevel > 0) return;

	RenderObject::onUpdate(dt);
	ActionMapper::onUpdate(dt);

	if (areaLabel)
		areaLabel->alpha.x = this->alpha.x;

	if (areaLabel2)
		areaLabel2->alpha.x = this->alpha.x;

	if (areaLabel3)
		areaLabel3->alpha.x = this->alpha.x;

	if (tophud)
		tophud->alpha.x = this->alpha.x;

	const float mmWidth  = game->miniMapRender->getMiniMapWidth();
	const float mmHeight = game->miniMapRender->getMiniMapHeight();
	if (addHintQuad1)
		addHintQuad1->position = game->miniMapRender->position + Vector(-mmWidth*3/22, -mmHeight/2-10);

	if (addHintQuad2)
		addHintQuad2->position = game->miniMapRender->position + Vector(mmWidth*3/22, -mmHeight/2-10);

	int offset = 26;
	if (helpButton)
		helpButton->position = Vector(core->getVirtualWidth()-core->getVirtualOffX()-offset, offset);

	if (alpha.x > 0)
	{
		//if (activeTile && activeTile==originalActiveTile && !gemMovers.empty())
		if (originalActiveTile && !gemMovers.empty())
		{
			gemMovers.back()->position = getAvatarWorldMapPosition();
		}
	}

	if (doubleClickTimer > 0)
	{
		doubleClickTimer -= dt;
		if (doubleClickTimer < 0)
			doubleClickTimer = 0;
	}

	if (isOn())
	{
		if (inputDelay > 0)
		{
			inputDelay -= dt;
			if (inputDelay < 0) inputDelay = 0;
		}
		else
		{
			WorldMapTile *selectedTile = 0;
			int sd=-1,d=0;
			for (int i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
			{
				WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
				if (tile && tile != activeTile)
				{
					if (tile->revealed || tile->prerevealed)
					{
						Quad *q = tile->q;
						if (q)
						{
							d = (q->getWorldPosition() - core->mouse.position).getSquaredLength2D();
							
							if (q->isCoordinateInsideWorld(core->mouse.position) && (sd == -1 || d < sd))
							{
								sd = d;
								selectedTile = tile;
								break;
							}
						}
					}
				}
			}

			if (!editorActive)
			{
				if (activeTile)
				{
					areaLabel3->setText(dsq->continuity.stringBank.get(activeTile->stringIndex));
				}
	
				if (selectedTile)
				{
					areaLabel->setText(dsq->continuity.stringBank.get(selectedTile->stringIndex));

					if (activeTile && !mover && !dsq->isNested() && isCursorOffHud())
					{
						if (!core->mouse.buttons.left && mb)
						{
							if ((activeTile != selectedTile) && selectedTile->q)
							{
								clearVis(activeTile);

								activeTile = selectedTile;
								activeQuad = activeTile->q;
								//activeTile->gridPos = activeTile->q->position;
								if (activeQuad)
								{
									dsq->clickRingEffect(activeQuad->getWorldPosition(), 0);

									dsq->sound->playSfx("bubble-lid");
									dsq->sound->playSfx("menuselect");
								}

								int num = dsq->continuity.worldMap.getNumWorldMapTiles();
								for (int i = 0; i < num; i++)
								{
									WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
									setProperTileColor(tile);
								}

								setVis(selectedTile);
							}

							mb = false;
						}
						else if (core->mouse.buttons.left && !mb)
						{
							mb = true;
						}
					}
					else
					{
						mb = false;
					}
				}
				else
				{
					areaLabel->setText("");
				}
			}
		}

		if (!core->mouse.buttons.left && mb)
			mb = false;

		if (core->mouse.buttons.middle || core->mouse.buttons.right)
		{
			// FIXME: For some reason, not all mouse movement events reach
			// this handler (at least under Linux/SDL), so when moving the
			// mouse quickly, the world map scrolling tends to lag behind.
			// We work around this by keeping our own "last position" vector
			// and calculating the mouse movement from that.  --achurch
			Vector mouseChange = core->mouse.position - lastMousePosition;
			internalOffset += mouseChange / scale.x;
		}

		
		float scrollSpeed = 2.0f;
		float amt = (400*dt)/scale.x;
		if (isActing(ACTION_SWIMLEFT))
		{
			internalOffset += Vector(amt, 0);
		}
		if (isActing(ACTION_SWIMRIGHT))
		{
			internalOffset += Vector(-amt, 0);
		}
		if (isActing(ACTION_SWIMDOWN))
		{
			if (core->getShiftState())
			{
				scale.stop();
				scale -= Vector(scrollSpeed*dt, scrollSpeed*dt);
			}
			else
			{
				internalOffset += Vector(0, -amt);
			}
		}
		if (isActing(ACTION_SWIMUP))
		{
			if (core->getShiftState())
			{
				scale.stop();
				scale += Vector(scrollSpeed*dt, scrollSpeed*dt);
			}
			else
			{
				internalOffset += Vector(0, amt);
			}	
		}

		if (core->joystickEnabled)
		{
			if (isActing(ACTION_SECONDARY))
			{
				if (core->joystick.position.y >= 0.6f)
					scale.interpolateTo(scale / 1.2f, 0.1f);
				else if (core->joystick.position.y <= -0.6f)
					scale.interpolateTo(scale * 1.2f, 0.1f);
			}
			else
			{
				// The negative multiplier is deliberate -- it makes the
				// map scroll as though the joystick was controlling the
				// cursor (which is fixed in the center of the screen).
				internalOffset += core->joystick.position * (-400*dt / scale.x);
			}
		}

		if (activeTile && activeTile->layer == 1)
		{
			zoomMax = interiorZoomMax;
		}
		else
		{
			zoomMax = exteriorZoomMax;
		}

		float scrollAmount = 0.2;//0.25;

		if (core->mouse.scrollWheelChange)
		{
			Vector target = scale;
			int changeLeft = core->mouse.scrollWheelChange;
			for (; changeLeft > 0; changeLeft--)
				target *= 1 + scrollAmount;
			for (; changeLeft < 0; changeLeft++)
				target /= 1 + scrollAmount;
			scale.interpolateTo(target, 0.1);
		}

		if (scale.x < zoomMin)
		{
			scale.stop();
			scale.x = scale.y = zoomMin;
		}
		if (scale.x > zoomMax)
		{
			scale.stop();
			scale.x = scale.y = zoomMax;
		}

		if (-internalOffset.x < xMin - 300/scale.x)
			internalOffset.x = -(xMin - 300/scale.x);
		else if (-internalOffset.x > xMax + 300/scale.x)
			internalOffset.x = -(xMax + 300/scale.x);
		if (-internalOffset.y < yMin - 225/scale.x)
			internalOffset.y = -(yMin - 225/scale.x);
		else if (-internalOffset.y > yMax + 150/scale.x)
			internalOffset.y = -(yMax + 150/scale.x);

		if (dsq->isDeveloperKeys() || dsq->mod.isActive())
		{
			if (editorActive)
			{
				if (activeTile && activeQuad)
				{
					float amt = dt*4;
					float a2 = dt*0.1f;

					if (core->getShiftState())
					{
						if (core->getKeyState(KEY_UP))
							activeTile->scale2 += -a2;
						if (core->getKeyState(KEY_DOWN))
							activeTile->scale2 += a2;
					}
					else
					{
						if (core->getCtrlState())
						{
							amt *= 50;
						}
						if (core->getKeyState(KEY_LEFT))
							activeTile->gridPos += Vector(-amt, 0);
						if (core->getKeyState(KEY_RIGHT))
							activeTile->gridPos += Vector(amt, 0);
						if (core->getKeyState(KEY_UP))
							activeTile->gridPos += Vector(0, -amt);
						if (core->getKeyState(KEY_DOWN))
							activeTile->gridPos += Vector(0, amt);
					}

					if (core->getKeyState(KEY_F2))
					{
						dsq->continuity.worldMap.save("data/WorldMap.txt");
					}

					activeQuad->position = activeTile->gridPos;
					activeQuad->scale = Vector(0.25f*activeTile->scale2, 0.25f*activeTile->scale2);
				}
			}
		}
	}
	else
	{
#ifdef AQUARIA_BUILD_MAPVIS
		if (!dsq->isInCutscene() && dsq->game->avatar && activeTile)
		{
			const float screenWidth  = core->getVirtualWidth()  * core->invGlobalScale;
			const float screenHeight = core->getVirtualHeight() * core->invGlobalScale;
			Vector camera = core->cameraPos;
			camera.x += screenWidth/2;
			camera.y += screenHeight/2;
			const float visWidth  = screenWidth  * visitedFraction;
			const float visHeight = screenHeight * visitedFraction;
			Vector tl, br;
			tl.x = (camera.x - visWidth/2 ) / dsq->game->cameraMax.x;
			tl.y = (camera.y - visHeight/2) / dsq->game->cameraMax.y;
			br.x = (camera.x + visWidth/2 ) / dsq->game->cameraMax.x;
			br.y = (camera.y + visHeight/2) / dsq->game->cameraMax.y;
			const int x0 = int(tl.x * MAPVIS_SUBDIV);
			const int y0 = int(tl.y * MAPVIS_SUBDIV);
			const int x1 = int(br.x * MAPVIS_SUBDIV);
			const int y1 = int(br.y * MAPVIS_SUBDIV);
			activeTile->markVisited(x0, y0, x1, y1);
			if (activeQuad)
			{
				if (visMethod == VIS_VERTEX)
				{
					for (int x = x0; x <= x1; x++)
					{
						for (int y = y0; y <= y1; y++)
						{
							activeQuad->setDrawGridAlpha(x, y, visibleMapSegAlpha);
						}
					}
				}
				else if (visMethod == VIS_WRITE)
				{
					// Do nothing -- we regenerate the tile on opening the map.
				}
			}
		}
#endif
	}

	lastMousePosition = core->mouse.position;
}

Vector WorldMapRender::getAvatarWorldMapPosition()
{
	Vector p;
	if (originalActiveTile && dsq->game && dsq->game->avatar)
	{
		Vector p = dsq->game->avatar->position;
		if (!dsq->game->avatar->warpInLocal.isZero())
		{
			p = dsq->game->avatar->warpInLocal;
		}
		return getWorldToTile(originalActiveTile, p, true, true);
	}
	return p;
}

Vector WorldMapRender::getWorldToTile(WorldMapTile *tile, Vector position, bool fromCenter, bool tilePos)
{
	Vector p;
	p = (position/TILE_SIZE) / (256*tile->scale);
	p *= 256*tile->scale*0.25f*tile->scale2;
	if (fromCenter)
		p -= Vector((128*tile->scale)*(0.25f*tile->scale2), (128*tile->scale)*(0.25f*tile->scale2));
	if (tilePos)
		p += tile->gridPos;
	return p;
}

void WorldMapRender::onRender()
{
	RenderObject::onRender();
}

bool WorldMapRender::isOn()
{
	return this->on;
}

GemMover *WorldMapRender::addGem(GemData *gemData)
{
	GemMover *g = new GemMover(gemData);
	addChild(g, PM_POINTER);
	gemMovers.push_back(g);
	g->update(0);
	return g;
}

void WorldMapRender::removeGem(GemMover *gem)
{
	dsq->continuity.removeGemData(gem->getGemData());
	fixGems();
}

void WorldMapRender::fixGems()
{
	for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
	{
		removeChild(*i);
		(*i)->destroy();
		delete *i;
	}
	gemMovers.clear();
	addAllGems();
}

void WorldMapRender::addAllGems()
{
	int c = 0;
	for (Continuity::Gems::reverse_iterator i = dsq->continuity.gems.rbegin(); i != dsq->continuity.gems.rend(); i++)
	{
		GemMover *g = addGem(&(*i));
		if (c == dsq->continuity.gems.size()-1)
			g->setBlink(true);
		else
			g->setBlink(false);
		c++;
	}
}

void WorldMapRender::toggle(bool turnON)
{
	if (AquariaGuiElement::currentGuiInputLevel > 0) return;
	if (dsq->game->miniMapRender->isRadarHide()) return;
	if (alpha.isInterpolating()) return;

	if (dsq->mod.isActive()) return;
	
	if (dsq->isNested()) return;

	if (!dsq->game->avatar) return;

	if (turnON && dsq->game->avatar->isSinging()) return;

	if (dsq->game->isInGameMenu()) return;

	if (!dsq->game->isActive()) return;
	
	if (turnON && dsq->game->isPaused()) return;

	if (!this->on && !dsq->game->avatar->isInputEnabled()) return;

	if (dsq->game->avatar && (dsq->game->avatar->isInDarkness() && dsq->continuity.form != FORM_SUN))
	{
		core->sound->playSfx("denied");
		return;
	}

	mb = false;
	this->on = turnON;
	if (on)
	{
		restoreVel = dsq->game->avatar->vel;
		dsq->game->avatar->vel = Vector(0,0,0);
		//dsq->game->avatar->idle();
		dsq->game->togglePause(true);

		core->sound->playSfx("menu-open");

		originalActiveTile = activeTile;

		if (activeTile)
		{
			internalOffset = -activeTile->gridPos;
			if (activeTile->layer == 1)
				scale = Vector(1.5,1.5);
			else
				scale = Vector(1,1);
			if (visMethod == VIS_WRITE)
			{
				// Texture isn't updated while moving, so force an update here
				clearVis(activeTile);
				setVis(activeTile);
			}
		}

		xMin = xMax = -internalOffset.x;
		yMin = yMax = -internalOffset.y;
		for (int i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
		{
			WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
			if (tile && (tile->revealed || tile->prerevealed) && tile->q)
			{
				Quad *q = tile->q;
				const float width = q->getWidth() * q->scale.x;
				const float height = q->getHeight() * q->scale.y;
				if (xMin > tile->gridPos.x - width/2)
					xMin = tile->gridPos.x - width/2;
				if (xMax < tile->gridPos.x + width/2)
					xMax = tile->gridPos.x + width/2;
				if (yMin > tile->gridPos.y - height/2)
					yMin = tile->gridPos.y - height/2;
				if (yMax < tile->gridPos.y + height/2)
					yMax = tile->gridPos.y + height/2;
			}
		}

		if (bg)
			bg->alpha.interpolateTo(1, 0.2);

		alpha.interpolateTo(1, 0.2);

		
		//dsq->game->hudUnderlay->alpha.interpolateTo(WORLDMAP_UNDERLAY_ALPHA, 0.2);
		underlay->alpha.interpolateTo(WORLDMAP_UNDERLAY_ALPHA, 0.2);

		addHintQuad1->alpha.interpolateTo(1.0, 0.2);
		addHintQuad2->alpha.interpolateTo(1.0, 0.2);
		helpButton->alpha.interpolateTo(1.0, 0.2);
		
		addAllGems();
		
		for (Continuity::Beacons::reverse_iterator i = dsq->continuity.beacons.rbegin(); i != dsq->continuity.beacons.rend(); i++)
		{
			if ((*i).on)
			{
				BeaconRender *b = new BeaconRender(&(*i));
				//b->position = (*i).pos;
				//game->addRenderObject(b, layer+1);
				addChild(b, PM_POINTER);
				beaconRenders.push_back(b);
			}
		}

		inputDelay = 0.5;
	}
	else if (!on)
	{
		inputDelay = 0.5;

		if (originalActiveTile && activeTile)
		{
			if (activeTile != originalActiveTile)
			{
				clearVis(activeTile);
				setVis(originalActiveTile);
				activeTile = originalActiveTile;
				activeQuad = activeTile->q;
			}
		}

		int num = dsq->continuity.worldMap.getNumWorldMapTiles();
		for (int i = 0; i < num; i++)
		{
			WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
			setProperTileColor(tile);
		}

		// again to set the correct color
		// lame, don't do that
		//setVis(activeTile);

		// just set the color
		if (activeTile)
		{
			activeTile->q->color = Vector(1,1,1);
			activeTile->q->alphaMod = 1;
		}


		//setVis(activeTile);
		/*
		for (int i = 0; i < LR_MENU; i++)
		{
			RenderObjectLayer *rl = dsq->getRenderObjectLayer(i);
			rl->visible = true;
		}
		*/

		core->sound->playSfx("Menu-Close");

		if (bg)
			bg->alpha.interpolateTo(0, 0.2);

		alpha.interpolateTo(0, 0.2);

		dsq->game->togglePause(false);
		//dsq->game->hudUnderlay->alpha.interpolateTo(0, 0.2);
		underlay->alpha.interpolateTo(0, 0.2);
		addHintQuad1->alpha.interpolateTo(0, 0.2);
		addHintQuad2->alpha.interpolateTo(0, 0.2);
		helpButton->alpha.interpolateTo(0, 0.2);


		for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
		{
			//removeChild(*i);
			(*i)->safeKill();
		}
		gemMovers.clear();
		
		for (BeaconRenders::iterator i = beaconRenders.begin(); i != beaconRenders.end(); i++)
		{
			//removeChild(*i);
			(*i)->safeKill();
		}
		beaconRenders.clear();

		dsq->game->avatar->vel = restoreVel;
	}
}

void WorldMapRender::createGemHint(const std::string &gfx)
{
	std::string useString = dsq->getUserInputString(dsq->continuity.stringBank.get(860), "", true);
	if (!useString.empty())
	{
		doubleClickTimer = 0;
		GemData *g = dsq->continuity.pickupGem(gfx, false);
		g->canMove = 1;
		g->pos = getAvatarWorldMapPosition() + Vector(0, -20);
		g->userString = useString;
		addGem(g);
		fixGems();
	}
}

void WorldMapRender::action (int id, int state)
{
	if (isOn())
	{
		if (id == ACTION_TOGGLEHELPSCREEN && !state)
		{
			onToggleHelpScreen();
		}
		if (id == ACTION_TOGGLEWORLDMAPEDITOR && !state)
		{
			if (dsq->isDeveloperKeys() || dsq->mod.isActive())
			{
				editorActive = !editorActive;

				if (editorActive)
				{
					areaLabel->setText("EDITING...");
				}
			}
		}

		if (id == ACTION_PRIMARY && state)
		{
			if (addHintQuad1->isCoordinateInRadius(core->mouse.position, 10))
			{
				createGemHint("pyramidyellow");
			}
			if (addHintQuad2->isCoordinateInRadius(core->mouse.position, 10))
			{
				createGemHint("pyramidpurple");
			}
		}

		if (id == ACTION_SECONDARY && !state)
		{
			if (!mover)
			{
				for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
				{
					if ((*i)->canMove && (core->mouse.position - (*i)->getWorldPosition()).isLength2DIn(GEM_GRAB))
					{
						removeGem(*i);
						break;
					}
				}
			}

		}

		/*
		if (id == ACTION_PRIMARY && state)
		{
			if (doubleClickTimer > 0)
			{
				doubleClickTimer = 0;
				GemData *g = dsq->continuity.pickupGem("pyramidyellow", false);
				g->canMove = 1;
				g->userString = dsq->getUserInputString("Enter Map Hint Name:", "");
				addGem(g);
			}
			else
			{
				if (doubleClickTimer == 0)
				{
					doubleClickTimer = DOUBLE_CLICK_DELAY;
				}
			}
		}
		*/
	}
}

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
#include "RenderGrid.h"

#define GEM_GRAB			 10

namespace WorldMapRenderNamespace
{
	const float WORLDMAP_UNDERLAY_ALPHA = 0.8f;

	float baseMapSegAlpha		= 0.4f;
	float visibleMapSegAlpha	= 0.8f;

	const float blinkPeriod		= 0.2f;

	// Fraction of the screen width and height we consider "visited".
	// (We don't mark the entire screen "visited" because the player may
	// overlook things on the edge of the screen while moving.)
	const float visitedFraction	= 0.8f;

	enum VisMethod
	{
		VIS_VERTEX		= 0, // Uses the RenderObject tile grid (RenderObject::setSegs()) to display visited areas
		VIS_WRITE		= 1  // Uses render-to-texture instead
	};

	const VisMethod visMethod = VIS_VERTEX;
	WorldMapRevealMethod revMethod = REVEAL_DEFAULT;

	std::vector<Quad *> tiles;

	Quad *activeQuad=0, *lastActiveQuad=0, *originalActiveQuad=0;
	Quad *lastVisQuad=0, *visQuad=0;
	WorldMapTile *lastVisTile=0;

	float xMin, yMin, xMax, yMax;

	float zoomMin = 0.2f;
	float zoomMax = 3.0f;
	const float exteriorZoomMax = 3.0f;
	const float interiorZoomMax = 3.0f;

	bool editorActive=false;

	Quad *tophud=0;

	Gradient *underlay = 0;
}

using namespace WorldMapRenderNamespace;

std::vector <Quad*> grid;

class GemMover;

GemMover *mover=0;

WorldMapTile *activeTile=0;

const float beaconSpawnBitTime = 0.05f;


void WorldMapRender::setRevealMethod(WorldMapRevealMethod m)
{
	switch(m)
	{
		case REVEAL_PARTIAL:
			revMethod = REVEAL_PARTIAL;
			baseMapSegAlpha = 0;
			break;

		default:
			revMethod = REVEAL_DEFAULT;
			baseMapSegAlpha = 0.4f;
	}
}


class WorldMapBoundQuad : public Quad
{
public:
	WorldMapBoundQuad(const Vector &posToUse)
	{
		position = posToUse;
		truePosition = posToUse;
		followCamera = 1;
	}

protected:
	Vector truePosition;
	void onUpdate(float dt) OVERRIDE
	{
		Quad::onUpdate(dt);

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
		setTexture("gui/minimap/ripple");
		position = beaconData->pos;
		truePosition = beaconData->pos;
		followCamera = 1;

		alpha = 0.5;
		color = beaconData->color;


		spawnBitTimer = 0;
	}


	Vector truePosition;

	ParticleEffect *pe;

	float spawnBitTimer;


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



		if (!game->worldMapRender->isOn()) return;

		const int lenRange = 125;
		const float pscale = 0.7f;

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

				q->setLife(1);
				q->setDecayRate(1.0f/t);

				q->setBlendType(BLEND_ADD);
				addChild(q, PM_POINTER);



			}
			else
			{
				leftOver = 0;
			}
		}



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
		alphaMod = 0.66f;
		canMove = gemData->canMove;



		std::string useString = gemData->userString;



		text = new TTFText(&dsq->fontArialSmall);
		text->offset = Vector(0, 4);
		text->setText(useString);
		text->setAlign(ALIGN_CENTER);

		textBG = new RoundedRect();
		textBG->setWidthHeight(text->getActualWidth() + 20, 25, 10);
		textBG->alpha = 0;
		textBG->followCamera = 1;
		game->addRenderObject(textBG, LR_WORLDMAPHUD);

		textBG->addChild(text, PM_POINTER);

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



	}
	bool canMove;
protected:

	float blinkTimer;
	bool blink;
	GemData *gemData;

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



				}
			}
			else if (mover == this)
			{

				position += (core->mouse.position - wp)/parent->scale.x;
				if (!core->mouse.buttons.left)
				{
					mover = 0;
					core->sound->playSfx("Gem-Place");



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


			if (!gemData->userString.empty())
				textBG->show();
		}
		else
		{

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
				tile->q->alphaMod = 0.5f;

			if (activeTile && (tile->layer != activeTile->layer || (tile->layer > 0 && activeTile != tile)))
				tile->q->alphaMod *= 0.5f;

			tile->q->color = Vector(0.7f, 0.8f, 1);
		}
		else
		{
			debugLog("no Q!");
		}
	}
}


static void tileDataToVis(WorldMapTile *tile, Array2d<Vector>& vis)
{
	const unsigned char *data = tile->getData();

	if (data != 0)
	{
		const float a = tile->prerevealed ? 0.4f :  baseMapSegAlpha;
		const size_t rowSize = MAPVIS_SUBDIV/8;
		for (size_t y = 0; y < MAPVIS_SUBDIV; y++, data += rowSize)
		{
			for (size_t x = 0; x < MAPVIS_SUBDIV; x += 8)
			{
				unsigned char dataByte = data[x/8];
				for (size_t x2 = 0; x2 < 8; x2++)
				{
					vis(x+x2,y).z = (dataByte & (1 << x2)) ? visibleMapSegAlpha : a;
				}
			}
		}
	}
	else
	{
		const float a = tile->prerevealed ? 0.4f :  baseMapSegAlpha;
		Vector *gp = vis.data();
		const size_t n = vis.linearsize();
		for(size_t i = 0; i < n; ++i)
			gp[i].z = a;
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
	tile->q->texture->readRGBA(savedTexData);

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

	tile->q->texture->writeRGBA(0, 0, texWidth, texHeight, texData);
	delete[] texData;

	return savedTexData;
}

static void resetTileAlpha(WorldMapTile *tile, const unsigned char *savedTexData)
{
	tile->q->texture->writeRGBA(0, 0, tile->q->texture->width, tile->q->texture->height, savedTexData);
}



void WorldMapRender::setVis(WorldMapTile *tile)
{
	if (!tile) return;


	tile->q->color = Vector(1,1,1);
	tile->q->alphaMod = 1;

	if (visMethod == VIS_VERTEX)
	{
		RenderGrid *g = tile->q->setSegs(MAPVIS_SUBDIV, MAPVIS_SUBDIV, 0, 0, 0, 0, 2.0, 1);
		if(g)
		{
			g->gridType = GRID_UNDEFINED;
			g->drawOrder = GRID_DRAW_WORLDMAP;
			tileDataToVis(tile, g->array2d());
		}
	}
	else if (visMethod == VIS_WRITE)
	{
		savedTexData = tileDataToAlpha(tile);
	}

	lastVisQuad = tile->q;
	lastVisTile = tile;
}

void WorldMapRender::clearVis(WorldMapTile *tile)
{
	if (!tile) return;
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



	const size_t num = dsq->continuity.worldMap.getNumWorldMapTiles();
	std::string n = game->sceneName;
	stringToUpper(n);
	std::vector<std::string> textodo(num);
	std::vector<Texture*> texs(num, NULL);
	textodo.reserve(num);
	for (size_t i = 0; i < num; i++)
	{
		WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
		if (tile)
		{
			if (tile->name == n)
				activeTile = tile;
			textodo[i] = "gui/worldmap/" + tile->name;
		}
	}

	tiles.clear();

	if(num)
		dsq->texmgr.loadBatch(&texs[0], &textodo[0], num);

	for (size_t i = 0; i < num; i++)
	{
		WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
		if (tile)
		{
			Vector pos(tile->gridPos.x, tile->gridPos.y);

			Quad *q = new Quad;
			q->setTexturePointer(texs[i]);
			q->position = pos;
			q->alphaMod = 0;

			tile->q = q;

			q->setWidthHeight(q->getWidth()*tile->scale, q->getHeight()*tile->scale);
			q->scale = Vector(0.25f*tile->scale2, 0.25f*tile->scale2);

			if (tile == activeTile)
				activeQuad = q;

			if (revMethod == REVEAL_PARTIAL || activeQuad == q)
			{
				setVis(tile);
			}

			setProperTileColor(tile);

			if(activeQuad == q)
			{
				activeTile->q->color = Vector(1,1,1);
				activeTile->q->alphaMod = 1;
			}

			addChild(q, PM_POINTER);

			tiles.push_back(q);
		}
	}
	shareAlphaWithChildren = 1;

	bindInput();

	tophud = new Quad("gui/worldmap-ui", Vector(400,64));
	tophud->followCamera = 1;
	tophud->alpha = 0;
	game->addRenderObject(tophud, LR_WORLDMAPHUD);


	float aly = 26, aly2 = 18;
	float sz = 0.6f;

	//hover
	areaLabel = new BitmapText(dsq->smallFont);
	areaLabel->scale = Vector(sz,sz);

	areaLabel->setAlign(ALIGN_CENTER);
	areaLabel->followCamera = 1;
	areaLabel->position = Vector(150,aly);
	game->addRenderObject(areaLabel, LR_WORLDMAPHUD);
	areaLabel->alpha = 0;

	//in
	areaLabel2 = new BitmapText(dsq->smallFont);

	areaLabel2->scale = Vector(sz,sz);
	areaLabel2->followCamera = 1;
	areaLabel2->setAlign(ALIGN_CENTER);
	areaLabel2->position = Vector(400,aly2);
	game->addRenderObject(areaLabel2, LR_WORLDMAPHUD);
	areaLabel2->alpha = 0;

	//select
	areaLabel3 = new BitmapText(dsq->smallFont);
	areaLabel3->scale = Vector(sz,sz);

	areaLabel3->followCamera = 1;
	areaLabel3->setAlign(ALIGN_CENTER);
	areaLabel3->position = Vector(650, aly);
	areaLabel3->alpha = 0;
	game->addRenderObject(areaLabel3, LR_WORLDMAPHUD);

	if (activeTile)
	{
		areaLabel2->setText(stringbank.get(activeTile->stringIndex));
	}

	originalActiveTile = activeTile;

	bindInput();

	underlay = new Gradient;

	underlay->makeVertical(Vector(0.25,0.25,0.5), Vector(0,0,0.25));
	underlay->position = Vector(400,300);
	underlay->autoWidth = AUTO_VIRTUALWIDTH;
	underlay->autoHeight = AUTO_VIRTUALHEIGHT;
	underlay->followCamera = 1;
	underlay->alpha = 0;
	game->addRenderObject(underlay, LR_HUDUNDERLAY);

	addHintQuad1 = new Quad("gems/pyramidyellow", Vector(0,0));
	addHintQuad1->followCamera = 1;
	addHintQuad1->alpha = 0;
	game->addRenderObject(addHintQuad1, LR_WORLDMAPHUD);

	addHintQuad2 = new Quad("gems/pyramidpurple", Vector(0,0));
	addHintQuad2->followCamera = 1;
	addHintQuad2->alpha = 0;
	game->addRenderObject(addHintQuad2, LR_WORLDMAPHUD);


	helpButton = new AquariaMenuItem;
	helpButton->event.setActionMapperCallback(this, ACTION_TOGGLEHELPSCREEN, 0);
	helpButton->useQuad("gui/icon-help");
	helpButton->useGlow("particles/glow", 40, 40);
	helpButton->useSound("Click");
	helpButton->alpha = 0;

	game->addRenderObject(helpButton, LR_WORLDMAPHUD);
}

void WorldMapRender::onToggleHelpScreen()
{
	game->toggleHelpScreen();
}

void WorldMapRender::bindInput()
{
	clearActions();
	clearCreatedEvents();

	addAction(ACTION_TOGGLEWORLDMAPEDITOR, KEY_TAB, -1);

	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		const ActionSet& as = dsq->user.control.actionSets[i];
		int sourceID = (int)i;

		as.importAction(this, "PrimaryAction",		ACTION_PRIMARY, sourceID);
		as.importAction(this, "SecondaryAction",	ACTION_SECONDARY, sourceID);

		as.importAction(this, "SwimLeft",			ACTION_SWIMLEFT, sourceID);
		as.importAction(this, "SwimRight",			ACTION_SWIMRIGHT, sourceID);
		as.importAction(this, "SwimUp",			ACTION_SWIMUP, sourceID);
		as.importAction(this, "SwimDown",			ACTION_SWIMDOWN, sourceID);
	}
}

void WorldMapRender::destroy()
{

	for (size_t i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
	{
		WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
		clearVis(tile);
	}

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
			float sd=-1,d=0;
			for (size_t i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
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

							if (q->isCoordinateInsideWorld(core->mouse.position) && (sd < 0 || d < sd))
							{
								sd = d;
								selectedTile = tile;
							}
						}
					}
				}
			}

			if (!editorActive)
			{
				if (activeTile)
				{
					areaLabel3->setText(stringbank.get(activeTile->stringIndex));
				}

				if (selectedTile)
				{
					areaLabel->setText(stringbank.get(selectedTile->stringIndex));

					if (activeTile && !mover && !dsq->isNested() && isCursorOffHud())
					{
						if (!core->mouse.buttons.left && mb)
						{
							if ((activeTile != selectedTile) && selectedTile->q)
							{
								debugLog("selectedTile: " + selectedTile->name);

								if(revMethod == REVEAL_DEFAULT)
									clearVis(activeTile);

								activeTile = selectedTile;
								activeQuad = activeTile->q;

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

		if(!editorActive)
		{
			float scrollSpeed = 2.0f;
			float amt = (400*dt)/scale.x;
			if (isActing(ACTION_SWIMLEFT, -1))
			{
				internalOffset += Vector(amt, 0);
			}
			if (isActing(ACTION_SWIMRIGHT, -1))
			{
				internalOffset += Vector(-amt, 0);
			}
			if (isActing(ACTION_SWIMDOWN, -1))
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
			if (isActing(ACTION_SWIMUP, -1))
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
				if (isActing(ACTION_SECONDARY, -1))
				{
					Vector jpos;
					for(size_t i = 0; i < core->getNumJoysticks(); ++i)
						if(Joystick *j = core->getJoystick(i))
							if(j && j->isEnabled())
								if(fabsf(j->position.y) > 0.6f)
								{
									jpos = j->position;
									break;
								}

					if (jpos.y >= 0.6f)
						scale.interpolateTo(scale / 1.2f, 0.1f);
					else if (jpos.y <= -0.6f)
						scale.interpolateTo(scale * 1.2f, 0.1f);
				}
				else
				{
					Vector jpos;
					for(size_t i = 0; i < core->getNumJoysticks(); ++i)
						if(Joystick *j = core->getJoystick(i))
							if(j && j->isEnabled())
								if(!j->position.isZero())
								{
									jpos = j->position;
									break;
								}
					// The negative multiplier is deliberate -- it makes the
					// map scroll as though the joystick was controlling the
					// cursor (which is fixed in the center of the screen).
					internalOffset += jpos * (-400*dt / scale.x);
				}
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

		float scrollAmount = 0.2f;

		if (core->mouse.scrollWheelChange)
		{
			Vector target = scale;
			int changeLeft = core->mouse.scrollWheelChange;
			for (; changeLeft > 0; changeLeft--)
				target *= 1 + scrollAmount;
			for (; changeLeft < 0; changeLeft++)
				target /= 1 + scrollAmount;
			scale.interpolateTo(target, 0.1f);
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

		if (dsq->canOpenEditor())
		{
			if (editorActive)
			{
				if (activeTile && activeQuad)
				{
					float amt = dt*4;
					float a2 = dt*0.1f;

					if (core->getShiftState())
					{
						if (core->getCtrlState())
							a2 *= 10.0f;
						if (core->getKeyState(KEY_UP))
							activeTile->scale2 += -a2;
						if (core->getKeyState(KEY_DOWN))
							activeTile->scale2 += a2;
					}
					else if (core->getAltState())
					{
						if (core->getCtrlState())
							a2 *= 10.0f;
						if (core->getKeyState(KEY_UP))
							activeTile->scale += -a2;
						if (core->getKeyState(KEY_DOWN))
							activeTile->scale += a2;
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
						dsq->continuity.worldMap.save();
					}

					activeQuad->position = activeTile->gridPos;
					activeQuad->scale = Vector(0.25f*activeTile->scale2, 0.25f*activeTile->scale2);
					if(activeQuad->texture)
						activeQuad->setWidthHeight(activeQuad->texture->width*activeTile->scale, // FG: HACK force resize proper
												   activeQuad->texture->height*activeTile->scale);
				}
				updateEditor();
			}
		}
	}
	else
	{
		if (!dsq->isInCutscene() && game->avatar && activeTile
			&& !game->sceneEditor.isOn()
			)
		{
			const float screenWidth  = core->getVirtualWidth()  * core->invGlobalScale;
			const float screenHeight = core->getVirtualHeight() * core->invGlobalScale;
			Vector camera = core->cameraPos;
			camera.x += screenWidth/2;
			camera.y += screenHeight/2;
			const float visWidth  = screenWidth  * visitedFraction;
			const float visHeight = screenHeight * visitedFraction;
			Vector tl, br;
			tl.x = (camera.x - visWidth/2 ) / game->cameraMax.x;
			tl.y = (camera.y - visHeight/2) / game->cameraMax.y;
			br.x = (camera.x + visWidth/2 ) / game->cameraMax.x;
			br.y = (camera.y + visHeight/2) / game->cameraMax.y;
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
	}

	lastMousePosition = core->mouse.position;
}

Vector WorldMapRender::getAvatarWorldMapPosition()
{
	Vector p;
	if (originalActiveTile && game && game->avatar)
	{
		Vector p = game->avatar->position;
		if (!game->avatar->warpInLocal.isZero())
		{
			p = game->avatar->warpInLocal;
		}
		return getWorldToTile(originalActiveTile, p, true, true);
	}
	return p;
}

Vector WorldMapRender::getWorldToTile(WorldMapTile *tile, Vector position, bool fromCenter, bool tilePos)
{
	if(!tile->q)
		return Vector();
	const float sizew = (float)tile->q->texture->width;
	const float halfw = sizew / 2.0f;
	const float sizeh = (float)tile->q->texture->height;
	const float halfh = sizeh / 2.0f;
	Vector p;
	p = Vector((position.x/TILE_SIZE) / (sizew*tile->scale), (position.y/TILE_SIZE) / (sizeh*tile->scale));
	p.x *= sizew*tile->scale*0.25f*tile->scale2;
	p.y *= sizeh*tile->scale*0.25f*tile->scale2;
	if (fromCenter)
		p -= Vector((halfw*tile->scale)*(0.25f*tile->scale2), (halfh*tile->scale)*(0.25f*tile->scale2));
	if (tilePos)
		p += tile->gridPos;
	return p;
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
	size_t c = 0;
	for (Continuity::Gems::reverse_iterator i = dsq->continuity.gems.rbegin(); i != dsq->continuity.gems.rend(); i++)
	{
		GemMover *g = addGem(&(*i));
		if (c == dsq->continuity.gems.size()-1 || i->blink)
			g->setBlink(true);
		else
			g->setBlink(false);
		c++;
	}
}

void WorldMapRender::toggle(bool turnON)
{
	if (AquariaGuiElement::currentGuiInputLevel > 0) return;
	if (game->miniMapRender->isRadarHide()) return;
	if (alpha.isInterpolating()) return;

	if (dsq->mod.isActive() && !dsq->mod.hasWorldMap()) return;

	if (dsq->isNested()) return;

	if (!game->avatar) return;

	if (turnON && game->avatar->isSinging()) return;

	if (game->isInGameMenu()) return;

	if (!game->isActive()) return;

	if (turnON && game->isPaused()) return;

	if (!this->on && !game->avatar->isInputEnabled()) return;

	const SeeMapMode mapmode = game->avatar->getSeeMapMode();

	if (mapmode == SEE_MAP_NEVER
		|| (mapmode == SEE_MAP_DEFAULT && game->avatar->isInDarkness() && dsq->continuity.form != FORM_SUN))
	{
		core->sound->playSfx("denied");
		return;
	}

	mb = false;
	this->on = turnON;
	if (on)
	{
		restoreVel = game->avatar->vel;
		game->avatar->vel = Vector(0,0,0);

		game->togglePause(true);

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
		for (size_t i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
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
			bg->alpha.interpolateTo(1, 0.2f);

		alpha.interpolateTo(1, 0.2f);



		underlay->alpha.interpolateTo(WORLDMAP_UNDERLAY_ALPHA, 0.2f);

		addHintQuad1->alpha.interpolateTo(1.0f, 0.2f);
		addHintQuad2->alpha.interpolateTo(1.0f, 0.2f);
		helpButton->alpha.interpolateTo(1.0f, 0.2f);

		addAllGems();

		for (Continuity::Beacons::reverse_iterator i = dsq->continuity.beacons.rbegin(); i != dsq->continuity.beacons.rend(); i++)
		{
			if ((*i).on)
			{
				BeaconRender *b = new BeaconRender(&(*i));


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
				if(revMethod == REVEAL_DEFAULT)
				{
					clearVis(activeTile);
					setVis(originalActiveTile);
				}
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
		if (activeTile && activeTile->q)
		{
			activeTile->q->color = Vector(1,1,1);
			activeTile->q->alphaMod = 1;
		}



		core->sound->playSfx("Menu-Close");

		if (bg)
			bg->alpha.interpolateTo(0, 0.2f);

		alpha.interpolateTo(0, 0.2f);

		game->togglePause(false);

		underlay->alpha.interpolateTo(0, 0.2f);
		addHintQuad1->alpha.interpolateTo(0, 0.2f);
		addHintQuad2->alpha.interpolateTo(0, 0.2f);
		helpButton->alpha.interpolateTo(0, 0.2f);


		for (GemMovers::iterator i = gemMovers.begin(); i != gemMovers.end(); i++)
		{

			(*i)->safeKill();
		}
		gemMovers.clear();

		for (BeaconRenders::iterator i = beaconRenders.begin(); i != beaconRenders.end(); i++)
		{

			(*i)->safeKill();
		}
		beaconRenders.clear();

		game->avatar->vel = restoreVel;
	}
}

void WorldMapRender::createGemHint(const std::string &gfx)
{
	std::string useString = dsq->getUserInputString(stringbank.get(860), "", true);
	if (!useString.empty())
	{
		doubleClickTimer = 0;
		GemData *g = dsq->continuity.pickupGem(gfx, false);
		g->canMove = 1;
		g->pos = getAvatarWorldMapPosition();
		g->userString = useString;
		addGem(g);
		fixGems();
	}
}

void WorldMapRender::updateEditor()
{
	std::ostringstream os;
	os << "EDITING...";
	if(activeTile)
	{
		os << " (" << activeTile->name << ")" << std::endl;
		os << "x=" << activeTile->gridPos.x << "; y=" << activeTile->gridPos.y << std::endl;
		os << "scale=" << activeTile->scale << "; scale2=" << activeTile->scale2;
	}
	areaLabel->setText(os.str());
}

void WorldMapRender::action (int id, int state, int source, InputDevice device)
{
	if (isOn())
	{
		if (id == ACTION_TOGGLEHELPSCREEN && !state)
		{
			onToggleHelpScreen();
		}
		if (id == ACTION_TOGGLEWORLDMAPEDITOR && !state)
		{
			if (dsq->canOpenEditor())
			{
				editorActive = !editorActive;

				if (editorActive)
				{
					updateEditor();
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


	}
}

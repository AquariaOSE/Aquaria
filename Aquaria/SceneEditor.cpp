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

#include "SceneEditor.h"
#include "../BBGE/MathFunctions.h"
#include "Image.h"
#include "../BBGE/Gradient.h"
#include "../BBGE/DebugFont.h"

#include "Game.h"
#include "DSQ.h"
#include "Avatar.h"
#include "GridRender.h"
#include "Shot.h"
#include "Tile.h"
#include "TileRender.h"

#ifdef BBGE_BUILD_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <shellapi.h>
#endif

static const int minSelectionSize = 64;


static TileStorage& tilesForLayer(unsigned layer)
{
	assert(layer < MAX_TILE_LAYERS);
	return dsq->tilemgr.tilestore[layer];
}

SelectedEntity::SelectedEntity()
{
	clear();
}

void SelectedEntity::setSelectEntity(const SelectedEntity &ent)
{
	(*this) = ent;
}

void SelectedEntity::clear()
{
	typeListIndex = -1;
	prevScale = 1;
	nameBased = false;
	index = -1;
	name = "";
	prevGfx="";
}

void SelectedEntity::setIndex(int idx)
{
	clear();
	if (idx > -1)
	{
		nameBased = false;
		typeListIndex = idx;
		index = game->entityTypeList[idx].idx;
		prevGfx = game->entityTypeList[idx].prevGfx;
		prevScale = game->entityTypeList[idx].prevScale;
		name = game->entityTypeList[idx].name;
	}
}

void SelectedEntity::setName(const std::string &name, const std::string &prevGfx)
{
	clear();
	nameBased = true;
	this->name = name;
	this->prevGfx = prevGfx;
}

std::string getMapTemplateFilename()
{
	if (dsq->mod.isActive())
		return std::string(dsq->mod.getPath() + "maptemplates/" + game->sceneName + ".png");
	else
		return std::string("maptemplates/" + game->sceneName + ".png");
	return "";
}

static void tileToQuad(Quad *q, const TileData& t)
{
	q->position = Vector(t.x, t.y);
	q->setTexturePointer(t.et->tex);
	q->setWidthHeight(t.et->w, t.et->h); // set AFTER texture
	q->scale = Vector(t.scalex, t.scaley);
	q->fhTo(!!(t.flags & TILEFLAG_FH));
	q->rotation.z = t.rotation;
	if(t.flags & TILEFLAG_REPEAT && t.rep)
	{
		q->setRepeatScale(Vector(t.rep->texscaleX, t.rep->texscaleY));
		q->repeatTextureToFill(true);
		q->setOverrideTexCoords(t.getTexcoords());
	}
	q->renderBorder = true;
	q->renderBorderColor = TileRender::GetTagColor(t.tag);
	q->borderAlpha = 1.0f;
	q->update(0); // to prevent 1 frame of lag when setting
}

static void quadToTile(TileData& t, const Quad *q)
{
	Vector posAndRot = q->getWorldPositionAndRotation();
	t.x = posAndRot.x;
	t.y = posAndRot.y;
	t.rotation = posAndRot.z;
	Vector scale = q->getRealScale();
	t.scalex = scale.x;
	t.scaley = scale.y;
}

class MultiTileHelper : public Quad
{
	TileStorage& _ts;
	std::vector<size_t> _indices;
	std::vector<Quad*> _quads;
	Vector lastScale;

	MultiTileHelper(TileStorage& ts, const size_t *indices, size_t n)
		: Quad()
		, _ts(ts), _indices(indices, indices + n)
	{
		_quads.reserve(n);
		this->cull = false;
	}

public:
	static MultiTileHelper *New(unsigned bgLayer, const size_t *indices, size_t n)
	{
		assert(n);
		TileStorage& ts = dsq->tilemgr.tilestore[bgLayer];
		MultiTileHelper *th = new MultiTileHelper(ts, indices, n);

		if(n == 1)
		{
			TileData& t = ts.tiles[indices[0]];
			t.flags |= TILEFLAG_EDITOR_HIDDEN;
			tileToQuad(th, t);
		}
		else
		{
			Vector center;
			for(size_t i = 0; i < n; ++i)
			{
				TileData& t = ts.tiles[indices[i]];
				center += Vector(t.x, t.y);
				t.flags |= TILEFLAG_EDITOR_HIDDEN;

				// clone tile to quad
				Quad *q = new Quad;
				tileToQuad(q, t);

				th->addChild(q, PM_POINTER, RBP_ON);
				th->_quads.push_back(q);
			}

			th->position = center / float(n);

			// distribute quads around center
			for(size_t i = 0; i < n; ++i)
			{
				Quad *q = th->_quads[i];
				q->position -= th->position;
			}
		}

		core->addRenderObject(th, LR_ELEMENTS1+bgLayer);
		return th;
	}

	virtual ~MultiTileHelper()
	{
	}

	void changeRepeatScale(const Vector& rs)
	{
		size_t n = _indices.size();
		assert(n == 1);
		TileData& t = _ts.tiles[_indices[0]];
		if((t.flags & TILEFLAG_REPEAT) && t.rep)
		{
			this->setRepeatScale(rs);
			t.rep->texscaleX = rs.x;
			t.rep->texscaleY = rs.y;
			t.refreshRepeat();
			this->setOverrideTexCoords(t.getTexcoords());
		}
	}

	virtual void onUpdate(float dt) OVERRIDE
	{
		if(scale != lastScale)
		{
			lastScale = scale;
			if(_indices.size() == 1)
			{
				TileData& t = _ts.tiles[_indices[0]];
				t.scalex = scale.x;
				t.scaley = scale.y;
				t.refreshRepeat();
				this->setOverrideTexCoords(t.getTexcoords());
			}
			else
			{
				for(size_t i = 0; i < _quads.size(); ++i)
				{
					TileData& t = _ts.tiles[_indices[i]];
					Quad *q = _quads[i];
					Vector s = q->getRealScale();
					t.scalex = s.x;
					t.scaley = s.y;
					t.refreshRepeat();
					q->setOverrideTexCoords(t.getTexcoords());
				}
			}
		}

		Quad::onUpdate(dt);
	}

	void finish()
	{
		size_t n = _indices.size();
		if(n == 1)
		{
			TileData& t = _ts.tiles[_indices[0]];
			t.flags &= ~TILEFLAG_EDITOR_HIDDEN;
			quadToTile(t, this);
		}
		else
		{
			for(size_t i = 0; i < n; ++i)
			{
				TileData& t = _ts.tiles[_indices[i]];
				Quad *q = _quads[i];
				t.flags &= ~TILEFLAG_EDITOR_HIDDEN;
				quadToTile(t, q);
			}
		}
		alphaMod = 0;
		this->safeKill(); // also deletes all children
		_ts.refreshAll();
	}
};


SceneEditor::SceneEditor() : ActionMapper(), on(false)
{
	autoSaveFile = 0;
	selectedIdx = -1;
	multi = NULL;
}

void SceneEditor::setBackgroundGradient()
{
	float g1r, g1g, g1b;
	float g2r, g2g, g2b;
	std::string gtop = dsq->getUserInputString("Set Gradient Top (3 rgb, spaced)", "");
	std::string gbtm = dsq->getUserInputString("Set Gradient Bottom (3 rgb, spaced)", "");
	std::istringstream is1(gtop);
	is1 >> g1r >> g1g >> g1b;
	std::istringstream is2(gbtm);
	is2 >> g2r >> g2g >> g2b;

	if (game->grad)
	{
		game->grad->makeVertical(Vector(g1r, g1g, g1b), Vector(g2r, g2g, g2b));
	}
}

void SceneEditor::updateSelectedElementPosition(Vector dist)
{
	if (state == ES_MOVING && multi)
	{
		// the actual tile position is updated when we release the mouse
		multi->position = oldPosition + dist;
	}
}

bool se_changedEntityType = false;
class SEQuad : public Quad
{
public:
	int entType;
	Quad *glow;
	DebugFont *label;
	float doubleClickTimer;
	std::string entName;
	bool mbld;
	SelectedEntity selectedEntity;

	SEQuad(std::string tex, Vector pos, int entType, std::string name) : Quad(tex, pos), entType(entType)
	{
		glow = new Quad;
		addChild(glow, PM_POINTER, RBP_ON);
		glow->setWidthHeight(48, 64);
		glow->alpha = 0;
		glow->setBlendType(BLEND_ADD);
		scale = Vector(0.5, 0.5);
		scale.interpolateTo(Vector(1,1), 0.5, 0, 0, 1);

		label = new DebugFont();
		label->setFontSize(5);
		label->position = Vector(-32,12);
		label->setText(name);
		label->alpha = 0;
		addChild(label, PM_POINTER);

		doubleClickTimer = 0;
		entName = name;
		mbld = false;
		selectedEntity.prevGfx = tex;
		selectedEntity.name = name;
	}

	void onUpdate(float dt)
	{
		if (doubleClickTimer > 0)
			doubleClickTimer -= dt;
		Quad::onUpdate(dt);


		if (game->sceneEditor.selectedEntity.name == selectedEntity.name
			|| (entType != -1 && game->sceneEditor.selectedEntity.typeListIndex == entType))
		{
			glow->alpha.interpolateTo(0.2f, 0.2f);
		}
		else
		{
			glow->alpha.interpolateTo(0, 0.1f);
		}
		if ((core->mouse.position - position).isLength2DIn(32))
		{
			label->alpha.interpolateTo(1, 0.1f);
			alpha.interpolateTo(1, 0.2f);
			scale.interpolateTo(Vector(1.5f, 1.5f), 0.3f);

			if (!core->mouse.buttons.left && mbld)
			{
				mbld = false;
			}
			else if (core->mouse.buttons.left && !mbld)
			{
				mbld = true;

				if (entType > -1)
					game->sceneEditor.selectedEntity.setIndex(entType);
				else
				{
					game->sceneEditor.selectedEntity.setName(selectedEntity.name, selectedEntity.prevGfx);
				}

				if (doubleClickTimer > 0)
				{
					doubleClickTimer = 0;
					if (!dsq->isFullscreen())
					{
						std::string fn = "scripts/entities/" + entName + ".lua";
						#ifdef BBGE_BUILD_WINDOWS
						debugLog("SHELL EXECUTE!");
						ShellExecute(0, "open", fn.c_str(), NULL, NULL, SW_SHOWNORMAL);
						#endif
					}
				}
				else
				{
					doubleClickTimer = 0.4f;
				}
			}
		}
		else
		{
			label->alpha.interpolateTo(0, 0.1f);
			alpha.interpolateTo(0.7f, 0.2f);
			scale.interpolateTo(Vector(1.0f, 1.0f), 0.3f);
			doubleClickTimer = 0;
		}
	}
};



std::vector<DebugButton*> mainMenu;
int execID = 0;
bool inMainMenu = false;

std::vector<SEQuad*> qs;
DebugFont *se_label=0;



void SceneEditorMenuReceiver::buttonPress(DebugButton *db)
{
	execID = db->buttonID;

	switch(execID)
	{
	case 200:
	{
		if (inMainMenu)
		{
			game->sceneEditor.closeMainMenu();
		}
		else
		{
			game->sceneEditor.openMainMenu();
		}
		execID = 0;
	}
	break;
	}
}

void SceneEditor::executeButtonID(int bid)
{
	switch(bid)
	{
	case 100:
		loadSceneByName();
	break;
	case 101:
		reloadScene();
	break;
	case 102:
		saveScene();
	break;
	case 103:
		// regen collision
		game->reconstructGrid(true);
	break;
	case 104:
		generateLevel();
	break;
	case 105:
		skinLevel();
	break;
	case 106:
		editModeElements();
	break;
	case 107:
		editModeEntities();
	break;
	case 108:
		editModePaths();
	break;
	case 110:
	{
		std::ostringstream os;
		os << game->gradTop.x << " " << game->gradTop.y << " " << game->gradTop.z;
		std::string read;
		read = dsq->getUserInputString("Enter Spaced R G B 0-1 Values for the Top Color (e.g. 1 0.5 0.5 for bright red)", os.str());

		if (!read.empty())
		{
			std::istringstream is(read);
			is >> game->gradTop.x >> game->gradTop.y >> game->gradTop.z;
		}

		std::ostringstream os2;
		os2 << game->gradBtm.x << " " << game->gradBtm.y << " " << game->gradBtm.z;
		read = dsq->getUserInputString("Enter Spaced R G B 0-1 Values for the Bottom Color (e.g. 0 0 0 for black)", os2.str());

		if (!read.empty())
		{
			std::istringstream is2(read);
			is2 >> game->gradBtm.x >> game->gradBtm.y >> game->gradBtm.z;
		}

		game->createGradient();
	}
	break;
	case 111:
	{
		std::string track = dsq->getUserInputString("Set Background Music Track", game->saveMusic);
		if (!track.empty())
		{
			game->setMusicToPlay(track);
			game->saveMusic = track;
			game->updateMusic();
		}
	}
	break;
	case 112:
	{
		selectEntityFromGroups();
	}
	break;
	case 113:
		game->toggleGridRender();
	break;
	case 114:
		dsq->screenshot();
	break;
	case 115:
	{
		dsq->returnToScene = game->sceneName;
		core->enqueueJumpState("AnimationEditor");
	}
	break;
	case 120:
	{
		dsq->returnToScene = game->sceneName;
		core->enqueueJumpState("ParticleEditor");
	}
	break;
	case 116:
		regenLevel();
	break;
	case 130:
		dsq->mod.recache();
	break;
	case 117:
	{
		std::ostringstream os;
		os << "Move by how much 'x y'? " << TILE_SIZE << " is one pixel on the map template.";
		std::string mv = dsq->getUserInputString(os.str());
		if(!mv.empty())
		{
			SimpleIStringStream is(mv.c_str(), SimpleIStringStream::REUSE);
			int x, y;
			is >> x >> y;
			if(x || y)
				moveEverythingBy(x, y);
		}
	}
	break;
	}
}

void SceneEditor::addMainMenuItem(const std::string &label, int bid)
{
	DebugButton *b = new DebugButton(bid, &menuReceiver, 350);
	b->label->setText(label);
	b->followCamera = 1;
	b->position.x = btnMenu->position.x + 20;
	b->position.y = btnMenu->position.y + (mainMenu.size()+1)*22;
	game->addRenderObject(b, LR_HUD);
	mainMenu.push_back(b);
}

void SceneEditor::openMainMenu()
{


	if (core->getNestedMains()>1)
	{
		return;
	}

	inMainMenu = true;

	if (placer)
		placer->renderQuad = 0;

	execID = -1;
	debugLog("Open Main Menu");

	mainMenu.clear();

	while (core->mouse.buttons.left)
	{
		core->run(FRAME_TIME);
	}

	addMainMenuItem("LOAD LEVEL...                    (SHIFT-F1)",            100);
	addMainMenuItem("RELOAD LEVEL                           (F1)",            101);
	addMainMenuItem("SAVE LEVEL                             (F2)",            102);
	addMainMenuItem("EDIT TILES                             (F5)",            106);
	addMainMenuItem("EDIT ENTITIES                          (F6)",            107);
	addMainMenuItem("EDIT NODES                             (F7)",            108);
	addMainMenuItem("REGEN COLLISIONS                    (ALT-R)",            103);
	addMainMenuItem("RECACHE TEXTURES                    (CTRL-R)",            130);

	addMainMenuItem("REGEN ROCK FROM MAPTEMPLATE       (F11+F12)",            116);

	addMainMenuItem("SET BG GRADIENT",                                        110);
	addMainMenuItem("SET MUSIC",                                            111);
	addMainMenuItem("ENTITY GROUPS                      (CTRL-E)",            112);
	if (game->gridRender)
		addMainMenuItem(std::string("TOGGLE TILE COLLISION RENDER ") + ((game->gridRender->alpha!=0) ? "OFF" : "ON ") + std::string("       (F9)"),            113);
	addMainMenuItem("SCREENSHOT                                 ",            114);



	addMainMenuItem("PARTICLE VIEWER                            ",            120);
	addMainMenuItem("ANIMATION EDITOR                           ",            115);
	addMainMenuItem("MOVE EVERYTHING ON MAP                     ",            117);


	while (1 && !core->getKeyState(KEY_TAB))
	{
		core->run(FRAME_TIME);
		if (execID != -1)
			break;
	}

	closeMainMenu();

	executeButtonID(execID);
}

void SceneEditor::closeMainMenu()
{
	inMainMenu = false;
	for (size_t i = 0; i < mainMenu.size(); i++)
	{
		mainMenu[i]->alpha = 0;
		mainMenu[i]->safeKill();
	}
	mainMenu.clear();


	if (placer)
		placer->renderQuad = 1;

	core->quitNestedMain();
}

void SceneEditor::init()
{
	entityPageNum = 0;
	multiSelecting = false;
	selectedTiles.clear();
	autoSaveTimer = 0;
	skinMinX = skinMinY = skinMaxX = skinMaxY = -1;
	editingEntity = 0;
	pathRender = new PathRender();
	core->getTopStateData()->addRenderObject(pathRender, LR_DEBUG_TEXT);
	pathRender->alpha = 0;

	editType = ET_ELEMENTS;
	state = ES_SELECTING;

	btnMenu = new DebugButton(200, &menuReceiver, 700);
	btnMenu->position = Vector(20, 20);
	btnMenu->label->setText("Menu");
	btnMenu->followCamera = 1;
	btnMenu->alpha = 0;

	game->addRenderObject(btnMenu, LR_HUD);

	selectedEntityType = 0;
	zoom = Vector(0.2f,0.2f);
	bgLayer = 5;
	text = new DebugFont();
	text->setFontSize(6);

	text->followCamera = 1;
	text->position = Vector(100,0);
	btnMenu->addChild(text, PM_POINTER);

	text->alpha = 0;
	on = false;

	addAction(MakeFunctionEvent(SceneEditor, loadScene), KEY_F1, 0);
	addAction(MakeFunctionEvent(SceneEditor, saveScene), KEY_F2, 0);

	addAction(MakeFunctionEvent(SceneEditor, moveToBack), KEY_Z, 0);
	addAction(MakeFunctionEvent(SceneEditor, moveToFront), KEY_X, 0);

	addAction(MakeFunctionEvent(SceneEditor, editModeElements), KEY_F5, 0);
	addAction(MakeFunctionEvent(SceneEditor, editModeEntities), KEY_F6, 0);
	addAction(MakeFunctionEvent(SceneEditor, editModePaths), KEY_F7, 0);

	addAction(MakeFunctionEvent(SceneEditor, mouseButtonLeft), MOUSE_BUTTON_LEFT, 1);
	addAction(MakeFunctionEvent(SceneEditor, mouseButtonRight), MOUSE_BUTTON_RIGHT, 1);
	addAction(MakeFunctionEvent(SceneEditor, mouseButtonLeftUp), MOUSE_BUTTON_LEFT, 0);
	addAction(MakeFunctionEvent(SceneEditor, mouseButtonRightUp), MOUSE_BUTTON_RIGHT, 0);

	addAction(MakeFunctionEvent(SceneEditor, placeElement), KEY_SPACE, 1);

	addAction(MakeFunctionEvent(SceneEditor, enterName), KEY_N, 0);
	addAction(MakeFunctionEvent(SceneEditor, changeShape), KEY_Y, 0);
	addAction(MakeFunctionEvent(SceneEditor, reversePath), KEY_T, 0);

	addAction(MakeFunctionEvent(SceneEditor, nextElement), KEY_R, 1);
	addAction(MakeFunctionEvent(SceneEditor, prevElement), KEY_E, 1);
	addAction(MakeFunctionEvent(SceneEditor, selectZero), KEY_HOME, 1);
	addAction(MakeFunctionEvent(SceneEditor, selectEnd), KEY_END, 1);

	addAction(MakeFunctionEvent(SceneEditor, placeAvatar), KEY_P, 0);
	addAction(MakeFunctionEvent(SceneEditor, deleteSelected), KEY_DELETE, 0);
	addAction(MakeFunctionEvent(SceneEditor, deleteSelected), KEY_BACKSPACE, 0);

	addAction(MakeFunctionEvent(SceneEditor, generateLevel), KEY_F11, 0);
	addAction(MakeFunctionEvent(SceneEditor, skinLevel), KEY_F12, 0);



	addAction(MakeFunctionEvent(SceneEditor, nextEntityType), KEY_RIGHT, 0);
	addAction(MakeFunctionEvent(SceneEditor, prevEntityType), KEY_LEFT, 0);
	addAction(MakeFunctionEvent(SceneEditor, down), KEY_DOWN, 0);
	addAction(MakeFunctionEvent(SceneEditor, up), KEY_UP, 0);


	addAction(MakeFunctionEvent(SceneEditor, flipElementHorz), KEY_T, 0);
	addAction(MakeFunctionEvent(SceneEditor, flipElementVert), KEY_Y, 0);

	addAction(MakeFunctionEvent(SceneEditor, toggleElementSolid), KEY_O, 0);
	addAction(MakeFunctionEvent(SceneEditor, toggleElementHurt), KEY_I, 0);
	addAction(MakeFunctionEvent(SceneEditor, toggleElementRepeat), KEY_L, 0);

	addAction(MakeFunctionEvent(SceneEditor, setGridPattern0), KEY_NUMPAD0, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern1), KEY_NUMPAD1, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern2), KEY_NUMPAD2, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern3), KEY_NUMPAD3, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern4), KEY_NUMPAD4, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern5), KEY_NUMPAD5, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern6), KEY_NUMPAD6, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern7), KEY_NUMPAD7, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern8), KEY_NUMPAD8, 0);
	addAction(MakeFunctionEvent(SceneEditor, setGridPattern9), KEY_NUMPAD9, 0);


	addAction(MakeFunctionEvent(SceneEditor, createAquarian), KEY_F, 0);

	addAction(MakeFunctionEvent(SceneEditor, dumpObs), KEY_F8, 0);



	addAction(ACTION_ZOOMIN,		KEY_PGUP, -1);
	addAction(ACTION_ZOOMOUT,		KEY_PGDN, -1);

	addAction(ACTION_CAMLEFT,		KEY_A, -1);
	addAction(ACTION_CAMRIGHT,		KEY_D, -1);
	addAction(ACTION_CAMUP,			KEY_W, -1);
	addAction(ACTION_CAMDOWN,		KEY_S, -1);

	addAction(ACTION_BGLAYEREND,	KEY_0, -1);

	addAction(ACTION_BGLAYER1,		KEY_1, -1);
	addAction(ACTION_BGLAYER2,		KEY_2, -1);
	addAction(ACTION_BGLAYER3,		KEY_3, -1);
	addAction(ACTION_BGLAYER4,		KEY_4, -1);
	addAction(ACTION_BGLAYER5,		KEY_5, -1);
	addAction(ACTION_BGLAYER6,		KEY_6, -1);
	addAction(ACTION_BGLAYER7,		KEY_7, -1);
	addAction(ACTION_BGLAYER8,		KEY_8, -1);
	addAction(ACTION_BGLAYER9,		KEY_9, -1);

	addAction(ACTION_BGLAYER10,		KEY_B, -1);
	addAction(ACTION_BGLAYER11,		KEY_N, -1);
	addAction(ACTION_BGLAYER12,		KEY_M, -1);

	addAction(ACTION_BGLAYER13,		KEY_J, -1);

	addAction(ACTION_BGLAYER14,		KEY_COMMA, -1);
	addAction(ACTION_BGLAYER15,		KEY_PERIOD, -1);
	addAction(ACTION_BGLAYER16,		KEY_SLASH, -1);

	addAction(ACTION_MULTISELECT,	KEY_LALT, -1);

	placer = new Quad;
	game->addRenderObject(placer, LR_HUD);
	placer->alpha = 0;
	curElementId = 0;
	selectedEntity.clear();
	nextElement();

	const ElementTemplate *et = dsq->tilemgr.tileset.getIfExists(0);
	if(!et)
		dsq->tilemgr.tileset.getAdjacent(0, 1, false, 1024);


	if (et)
	{
		placer->setTexture(et->gfx);
		placer->scale = Vector(1,1);
	}
	else
	{
		placer->setTexture("missingimage");
	}

	updateText();
}

void SceneEditor::createAquarian()
{

	static bool inCreateAqurian = false;
	if (inCreateAqurian) return;

	inCreateAqurian = true;
	std::string t = dsq->getUserInputString("Enter Aquarian:", "");
	if(!t.empty())
	{
		stringToUpper(t);
		Vector startPos = dsq->getGameCursorPosition();
		std::vector<TileDef> defs;
		defs.reserve(t.size());
		TileDef def(this->bgLayer);
		def.y = startPos.y;
		for (size_t i = 0; i < t.size(); i++)
		{
			if (t[i] >= 'A' && t[i] <= 'Z')
			{
				def.idx = 1024+int(t[i] - 'A');
			}
			else if (t[i] == '.' || t[i] == ' ')
			{
				def.idx = 1024+26;
			}
			else
				continue;

			def.x = startPos.x + 64*i;
			defs.push_back(def);
		}
		if(size_t n = defs.size())
			dsq->tilemgr.createTiles(&defs[0], n);
	}
	inCreateAqurian = false;
}

Path *SceneEditor::getSelectedPath()
{
	if (selectedIdx < game->getNumPaths())
	{
		return game->getPath(selectedIdx);
	}
	return 0;
}

void SceneEditor::enterName()
{
	if (editType == ET_PATHS)
	{
		Path *p = getSelectedPath();
		if (p)
		{
			std::string newname = dsq->getUserInputString("PathName", p->name);
			bool changed = newname != p->name;
			p->name = newname;
			if (changed)
			{
				p->refreshScript();
				p->init();
			}
		}
	}
}

void SceneEditor::changeShape()
{
	if (editType == ET_PATHS)
	{
		Path *p = getSelectedPath();
		if (p)
		{
			switch (p->pathShape)
			{
			case PATHSHAPE_RECT:
				p->pathShape = PATHSHAPE_CIRCLE;
			break;
			case PATHSHAPE_CIRCLE:
				p->pathShape = PATHSHAPE_RECT;
			break;
			}
		}
	}
}

void SceneEditor::reversePath()
{
	if (editType == ET_PATHS)
	{
		Path *p = getSelectedPath();
		if (p)
		{
			p->reverseNodes();
		}
	}
}

void SceneEditor::setGridPattern(int gi)
{
	if(core->getCtrlState())
	{
		const int tag = gi + 1; // key 0 is tag 0... otherwise it's all off by one
		if(size_t n = selectedTiles.size())
			getCurrentLayerTiles().setTag(tag, &selectedTiles[0], n);
	}
	else
	{
		if(size_t n = selectedTiles.size())
			getCurrentLayerTiles().setEffect(dsq->tilemgr.tileEffects, gi, &selectedTiles[0], n);
	}
}

void SceneEditor::setGridPattern0()
{	setGridPattern(-1); }

void SceneEditor::setGridPattern1()
{	setGridPattern(0);	}

void SceneEditor::setGridPattern2()
{	setGridPattern(1);	}

void SceneEditor::setGridPattern3()
{	setGridPattern(2);	}

void SceneEditor::setGridPattern4()
{	setGridPattern(3);	}

void SceneEditor::setGridPattern5()
{	setGridPattern(4);	}

void SceneEditor::setGridPattern6()
{	setGridPattern(5);	}

void SceneEditor::setGridPattern7()
{	setGridPattern(6);	}

void SceneEditor::setGridPattern8()
{	setGridPattern(7);	}

void SceneEditor::setGridPattern9()
{	setGridPattern(8);	}

void SceneEditor::moveToFront()
{
	if (selectedTiles.size() && !core->getShiftState())
	{
		getCurrentLayerTiles().moveToFront(&selectedTiles[0], selectedTiles.size());
	}
}

void SceneEditor::moveToBack()
{
	if(selectedTiles.size() && !core->getShiftState())
	{
		getCurrentLayerTiles().moveToBack(&selectedTiles[0], selectedTiles.size());
	}
}

void SceneEditor::clearSelection()
{
	getCurrentLayerTiles().clearSelection();
	selectedTiles.clear();
	editingEntity = NULL;
	editingPath = NULL;
	selectedIdx = -1;
}

void SceneEditor::editModeElements()
{
	setActiveLayer(bgLayer, editType != ET_ELEMENTS);
	editType = ET_ELEMENTS;
	if (const ElementTemplate *et = dsq->tilemgr.tileset.getIfExists(curElementId))
	{
		placer->setTexture(et->gfx);
		placer->scale = Vector(1,1);
	}
	placer->alpha = 0.5;
	pathRender->alpha = 0;
	editingEntity = NULL;
	editingPath = NULL;
}

void SceneEditor::editModeEntities()
{
	unselectTileLayer();
	clearSelection();
	editType = ET_ENTITIES;
	placer->setTexture(selectedEntity.prevGfx);
	placer->alpha = 0.5;
	pathRender->alpha = 0;
}

void SceneEditor::editModePaths()
{
	unselectTileLayer();
	clearSelection();
	editType = ET_PATHS;
	placer->alpha = 0;
	pathRender->alpha = 0.5;
}

int SceneEditor::getTileAtCursor()
{
	const TileStorage& ts = getCurrentLayerTiles();
	float minDist = HUGE_VALF;
	int selected = -1;
	const size_t n = ts.tiles.size();
	Vector cursor = dsq->getGameCursorPosition();
	for(size_t i = 0; i < n; ++i)
	{
		const TileData& t = ts.tiles[i];
		if (t.isVisible())
		{
			if (t.isCoordinateInside(cursor.x, cursor.y))
			{
				Vector v = cursor - Vector(t.x, t.y);
				float dist = v.getSquaredLength2D();
				if (dist < minDist || selected == -1)
				{
					minDist = dist;
					selected = int(i);
				}
			}
		}
	}
	return selected;
}

Entity *SceneEditor::getEntityAtCursor()
{
	int minDist = -1;
	Entity *selected = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->life == 1)
		{
			if (e->isCoordinateInside(dsq->getGameCursorPosition(), minSelectionSize))
			{
				Vector v = dsq->getGameCursorPosition() - e->position;
				int dist = v.getSquaredLength2D();
				if (dist < minDist || minDist == -1)
				{
					minDist = dist;
					selected = e;
				}
			}
		}
	}
	return selected;
}

void SceneEditor::deleteSelected()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ELEMENTS)
	{
		if (size_t n = selectedTiles.size())
		{
			TileStorage& ts = getCurrentLayerTiles();
			ts.deleteSome(&selectedTiles[0], n);
			selectedTiles.clear();
			game->reconstructGrid();
		}
	}
	else if (editType == ET_ENTITIES)
	{
		if (editingEntity)
		{
			if (editingEntity->getEntityType() != ET_AVATAR)
			{
				game->removeEntity(editingEntity);
				editingEntity = 0;
			}
		}
	}
	else if (editType == ET_PATHS)
	{
		if (core->getShiftState())
		{
			game->removePath(selectedIdx);
		}
		else
		{
			if (selectedIdx != -1)
			{
				Path *p = game->getPath(selectedIdx);
				if (p->nodes.size() == 1)
				{
					game->removePath(selectedIdx);
					selectedIdx = -1;
				}
				else
					p->removeNode(selectedNode);


			}
		}
	}
}

void SceneEditor::checkForRebuild()
{
	if (editType == ET_ELEMENTS && state != ES_SELECTING && !selectedTiles.empty())
	{
		const TileStorage& ts = getCurrentLayerTiles();
		const size_t n = ts.tiles.size();
		for (size_t i = 0; i < n; i++)
		{
			if(ts.tiles[i].flags & TILEFLAG_SOLID)
			{
				game->reconstructGrid();
				break;
			}
		}
	}
}

void SceneEditor::exitMoveState()
{
	destroyMultiTileHelper();
	checkForRebuild();
	state = ES_SELECTING;
}

void SceneEditor::enterAnyStateHelper(EditorStates newstate)
{
	state = newstate;
	if (editType == ET_ELEMENTS)
	{
		oldRepeatScale = Vector(1, 1); // not handled for multi-selection

		if(selectedTiles.size() == 1)
		{
			const TileData& t = getCurrentLayerTiles().tiles[selectedTiles[0]];
			if(t.flags & TILEFLAG_REPEAT && t.rep)
				oldRepeatScale = Vector(t.rep->texscaleX, t.rep->texscaleY);
		}

		MultiTileHelper *m = createMultiTileHelperFromSelection();
		oldScale = m->scale;
		oldPosition = m->position;
		oldRotation = m->rotation.z;
		cursorOffset = dsq->getGameCursorPosition();
	}
	else if (editType == ET_ENTITIES)
	{
		oldPosition = editingEntity->position;
		oldRotation = editingEntity->rotation.z;
		oldScale = editingEntity->scale;
		cursorOffset = editingEntity->position - dsq->getGameCursorPosition();
	}
	else if (editType == ET_PATHS)
	{
		oldPosition = game->getPath(selectedIdx)->nodes[selectedNode].position;
		cursorOffset = newstate == ES_SCALING
			? dsq->getGameCursorPosition()
			: oldPosition - dsq->getGameCursorPosition();
		oldScale = Vector(editingPath->rect.x2-editingPath->rect.x1,
			editingPath->rect.y2-editingPath->rect.y1);
	}
}

void SceneEditor::enterMoveState()
{
	if (state != ES_SELECTING) return;
	enterAnyStateHelper(ES_MOVING);
}

void SceneEditor::enterRotateState()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ENTITIES || editType == ET_ELEMENTS) // can't rotate nodes
	{
		enterAnyStateHelper(ES_ROTATING);
	}
}

void SceneEditor::enterScaleState()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ELEMENTS || editType == ET_PATHS) // can't scale entities
	{
		enterAnyStateHelper(ES_SCALING);
	}
}

void SceneEditor::updateEntitySaveData(Entity *editingEntity)
{
	if (editingEntity)
	{

		EntitySaveData *d = game->getEntitySaveDataForEntity(editingEntity);
		if (d)
		{
			std::ostringstream os;
			os << "idx: " << d->idx << " ";
			os << "name: " << editingEntity->name;

			debugLog(os.str());

			d->x = editingEntity->position.x;
			d->y = editingEntity->position.y;
			editingEntity->startPos = Vector(d->x, d->y);

			d->rot = editingEntity->rotation.z;
		}
		else
		{

		}
	}
}

void SceneEditor::mouseButtonLeftUp()
{
	if (multiSelecting || core->mouse.buttons.right) return;

	if (editType == ET_ENTITIES)
	{
		updateEntitySaveData(editingEntity);
	}
	if (state == ES_MOVING)
	{
		exitMoveState();
	}
	state = ES_SELECTING;
}

void SceneEditor::mouseButtonRightUp()
{
	if (multiSelecting || core->mouse.buttons.left) return;

	if (editType == ET_ENTITIES)
		updateEntitySaveData(editingEntity);
	if (editType == ET_ELEMENTS)
	{
		destroyMultiTileHelper();
		checkForRebuild();
	}
	state = ES_SELECTING;

}

static ElementFlag nextSolidEF(ElementFlag ef)
{
	switch(ef)
	{
		case EF_NONE: return EF_SOLID;
		case EF_SOLID: return EF_SOLID2;
		case EF_SOLID2: return EF_SOLID3;
		case EF_SOLID3: return EF_NONE;
	}
	return EF_NONE;
}

void SceneEditor::toggleElementSolid()
{
	TileStorage& ts = getCurrentLayerTiles();
	if (size_t n = selectedTiles.size())
	{
		for(size_t i = 0; i < n; ++i)
		{
			TileData& t = ts.tiles[selectedTiles[i]];
			t.flags = TileMgr::GetTileFlags(nextSolidEF(TileMgr::GetElementFlag((TileFlags)t.flags)));
		}
		game->reconstructGrid(true);
	}
}

void SceneEditor::toggleElementHurt()
{
	TileStorage& ts = getCurrentLayerTiles();
	if (size_t n = selectedTiles.size())
	{
		TileData& t0 = ts.tiles[selectedTiles[0]];
		const bool set = !(t0.flags & TILEFLAG_HURT);

		if(set)
			ts.changeFlags(TILEFLAG_SOLID | TILEFLAG_HURT, TILEFLAG_TRIM, &selectedTiles[0], n);
		else
			ts.changeFlags(0, TILEFLAG_SOLID | TILEFLAG_HURT | TILEFLAG_SOLID_IN | TILEFLAG_TRIM, &selectedTiles[0], n);

		game->reconstructGrid(true);
	}
}

void SceneEditor::toggleElementRepeat()
{
	TileStorage& ts = getCurrentLayerTiles();
	if (size_t n = selectedTiles.size())
	{
		TileData& t0 = ts.tiles[selectedTiles[0]];
		const bool set = !(t0.flags & TILEFLAG_REPEAT);
		if(set)
			ts.changeFlags(TILEFLAG_REPEAT, 0, &selectedTiles[0], n);
		else
			ts.changeFlags(0, TILEFLAG_REPEAT, &selectedTiles[0], n);
	}
}

void SceneEditor::mouseButtonLeft()
{
	if (multiSelecting || state != ES_SELECTING || core->mouse.buttons.right) return;
	if (editType == ET_ELEMENTS)
	{
		if (!selectedTiles.empty())
		{
			if (core->getShiftState())
			{
				cloneSelectedElement();
			}
			else
			{
				enterMoveState();
			}
		}
	}
	else if (editType == ET_ENTITIES)
	{
		if (editingEntity)
		{
			if (core->getShiftState())
			{
				cloneSelectedElement();
			}
			else
			{
				enterMoveState();
			}
		}
	}
	else if (editType == ET_PATHS)
	{
		if (selectedIdx != -1)
		{
			Path *p = getSelectedPath();
			editingPath = p;
			if (p && selectedNode < p->nodes.size())
			{
				if (core->getShiftState())
				{
					cloneSelectedElement();
				}
				else
				{
					enterMoveState();
				}
			}
		}
	}
}

void SceneEditor::mouseButtonRight()
{
	if (multiSelecting || state != ES_SELECTING || core->mouse.buttons.left) return;
	if (editType == ET_ENTITIES)
	{
		if (editingEntity)
		{
			if (core->getShiftState())
				enterScaleState();
			else
				enterRotateState();
		}
	}
	if (editType == ET_PATHS)
	{
		if (selectedIdx != -1)
		{
			debugLog("path scaling HERE!");
			Path *p = game->getPath(selectedIdx);
			editingPath = p;
			if (core->getShiftState())
			{
				enterScaleState();
			}
		}
	}
	if (editType == ET_ELEMENTS)
	{
		if (!selectedTiles.empty())
		{
			if (core->getShiftState())
				enterScaleState();
			else
				enterRotateState();
		}
	}
	if (editType == ET_ELEMENTS && state == ES_MOVING)
	{
	}
}

void SceneEditor::up()
{
	if (editType == ET_ELEMENTS && state == ES_SELECTING)
	{
		if (!selectedTiles.empty())
		{
			enterMoveState();
			updateSelectedElementPosition(Vector(0,-1));
			exitMoveState();
		}
	}
}

void SceneEditor::down()
{
	if (editType == ET_ELEMENTS && state == ES_SELECTING)
	{
		if (!selectedTiles.empty())
		{
			enterMoveState();
			updateSelectedElementPosition(Vector(0,1));
			exitMoveState();
		}
	}
}

void SceneEditor::regenLevel()
{
	generateLevel();
	skinLevel();
}

void SceneEditor::skinLevel()
{
	if (skinMinX == -1)
	{
		dsq->screenMessage("Cannot skin without generated level.");
		return;
	}

	skinLevel(skinMinX, skinMinY, skinMaxX, skinMaxY);
}

void SceneEditor::skinLevel(int minX, int minY, int maxX, int maxY)
{
	const int LAYER = 4;
	TileStorage& ts = tilesForLayer(LAYER);
	std::vector<TileDef> toAdd;
	toAdd.reserve(ts.tiles.size()); // assume the number of tiles isn't going to change much
	{
		std::vector<size_t> deleteTiles;
		deleteTiles.reserve(ts.tiles.size()); // pessimistically assume we have to delete everything
		for (size_t i = 0; i < ts.tiles.size(); i++)
		{
			const TileData& t = ts.tiles[i];
			if (t.et->idx >= 1 && t.et->idx <= 4) // delete all tiles designated as wall rocks
				deleteTiles.push_back(i);
		}

		ts.deleteSome(&deleteTiles[0], deleteTiles.size());
	}

	for (int x = minX; x < maxX-2; x++)
	{
		for (int y = minY; y < maxY; y++)
		{
			Vector offset, pOffset;
			Vector wallNormal;
			float rot=0;
			bool addTile = false;
			TileVector t(x,y);
			if (game->isObstructed(t, OT_MASK_BLACK)
				&& (
				!game->isObstructed(TileVector(x+1,y), OT_MASK_BLACK) ||
				!game->isObstructed(TileVector(x-1,y), OT_MASK_BLACK) ||
				!game->isObstructed(TileVector(x,y-1), OT_MASK_BLACK) ||
				!game->isObstructed(TileVector(x,y+1), OT_MASK_BLACK)
				)
				)
			{
				wallNormal = game->getWallNormal(t.worldVector(), 5, OT_MASK_BLACK);
				offset = wallNormal*(-TILE_SIZE*0.6f);
				MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), wallNormal, rot);
				rot = 180-(360-rot);
				addTile = true;
			}

			if(addTile)
			{
				const unsigned char u = tileProps.at(x, y-1, TPR_DEFAULT);
				const unsigned char d = tileProps.at(x, y+1, TPR_DEFAULT);
				const unsigned char l = tileProps.at(x-1, y, TPR_DEFAULT);
				const unsigned char r = tileProps.at(x+1, y, TPR_DEFAULT);
				if(u == TPR_DONT_SKIN
					|| d == TPR_DONT_SKIN
					|| l == TPR_DONT_SKIN
					|| r == TPR_DONT_SKIN)
				{
					addTile = false;
				}
			}

			if (addTile)
			{
				TileVector t(x,y);
				Vector p = t.worldVector();
				p += Vector(TILE_SIZE/2, TILE_SIZE/2,0);
				offset.z = 0;

				bool skip = false;
				for (size_t i = 0; i < toAdd.size(); i++)
				{
					const TileDef& d = toAdd[i];
					if ((p - Vector(d.x, d.y)).getSquaredLength2D() < (50*50))
					{
						skip = true;
						break;
					}
				}

				if (!skip)
				{
					const float NEAR_DIST = sqr(120);
					// count which tile are used nearby
					size_t useCount[4] = {0}; // index 0 is never used
					for (size_t i = 0; i < toAdd.size(); i++)
					{
						const TileDef& d = toAdd[i];
						if ((p - Vector(d.x, d.y)).getSquaredLength2D() < NEAR_DIST)
							useCount[d.idx-1]++;
					}

					// try to place the one that appeared least often so far
					size_t k = size_t(rand()) & 3; // tie breaker
					size_t kend = k + 4;
					size_t least = size_t(-1);
					size_t idx = 0;
					do
					{
						size_t i = k & 3;
						if(useCount[i] <= least)
						{
							least = useCount[i];
							idx = i;
						}
						++k;
					}
					while(k < kend);

					TileDef d(LAYER);
					d.x = p.x + offset.x;
					d.y = p.y + offset.y;
					d.idx = idx + 1; // bring into range [1..4]
					d.rot = rot;

					toAdd.push_back(d);
				}
			}
		}
	}

	std::ostringstream os;
	os << "skinlevel generated " << toAdd.size() << " tiles";
	debugLog(os.str());

	if(toAdd.size())
		dsq->tilemgr.createTiles(&toAdd[0], toAdd.size());

}

SceneEditor::TileProperty SceneEditor::GetColorProperty(unsigned char r, unsigned char g, unsigned char b)
{
	if(r >= 200 && g > 127 && g < 200 && b >= 200)
		return TPR_DONT_SKIN;

	return TPR_DEFAULT;
}

void SceneEditor::generateLevel()
{
	tileProps.clear();
	std::string file=getMapTemplateFilename();



	size_t maxX=0, maxY=0;

	const ImageData img = imageLoadGeneric(file.c_str(), true);
	if (img.pixels)
	{
		game->clearObsRows();

		assert(img.channels == 4);
		tileProps.init(img.w, img.h);
		int c = 0;

		for (size_t y = 0; y < img.h; y++)
		{
			bool isobs = false; // start assuming that there is no obstruction
			size_t xobs = 0;
			for(size_t x = 0; x < img.w; ++x)
			{
				tileProps(x, y) = GetColorProperty(img.pixels[c], img.pixels[c+1], img.pixels[c+2]);

				// anything that is close to black is obstruction
				bool obs = img.pixels[c] < 48 &&
							img.pixels[c+1] < 48 &&
							img.pixels[c+2] < 48;

				if(obs != isobs)
				{
					isobs = obs;
					if(obs)
					{
						xobs = x; // just changed from not-obs to obs, record start
						if (x > maxX)
							maxX = x;
						if (y > maxY)
							maxY = y;
					}
					else if(x != xobs) // safeguard against left side starting with obs
					{
						assert(x > xobs);
						game->addObsRow(xobs, y, x - xobs); // just changed from obs to not-obs, emit row
					}
				}

				c += img.channels;
			}
			if(isobs) // right side ends with obs, add final row
			{
				game->addObsRow(xobs, y, img.w - xobs);
				if (img.w > maxX)
					maxX = img.w;
				if (y > maxY)
					maxY = y;
			}
		}

		game->handleEditorMapGridUpdate();

		maxX--;
		maxY--;
		this->skinMinX = 4;
		this->skinMinY = 4;
		this->skinMaxX = maxX;
		this->skinMaxY = maxY;

		if (img.pixels != NULL)
			free(img.pixels);
	}
	else
	{
		debugLog("generateLevel: Failed to load mapTemplate");
	}
}

void SceneEditor::placeAvatar()
{
	game->avatar->position = dsq->getGameCursorPosition();
	game->action(ACTION_PLACE_AVATAR, 0, -1, INPUT_NODEVICE);
}

void SceneEditor::flipElementHorz()
{
	if (editType != ET_ELEMENTS)
		return;

	const size_t n = selectedTiles.size();

	if(core->getCtrlState())
	{
		if (n)
		{
			std::string inp = dsq->getUserInputString("Enter tag (int):");
			if(inp.empty())
				return;
			int tag = atoi(inp.c_str());
			getCurrentLayerTiles().setTag(tag, &selectedTiles[0], n);
		}
		return;
	}

	if (n)
	{
		TileStorage& ts = getCurrentLayerTiles();
		for(size_t i = 0; i < n; ++i)
			ts.tiles[selectedTiles[i]].flags ^= TILEFLAG_FH; // toggle bit
	}
	else
		this->placer->flipHorizontal();
}

void SceneEditor::flipElementVert()
{
	if (editType != ET_ELEMENTS)
		return;

	if (size_t n = selectedTiles.size())
	{
		TileStorage& ts = getCurrentLayerTiles();
		for(size_t i = 0; i < n; ++i)
			ts.tiles[selectedTiles[i]].flags ^= TILEFLAG_FV; // toggle bit
	}
	else
		this->placer->flipVertical();
}

void SceneEditor::updateMultiSelect()
{
	if (!multiSelecting) return;
	Vector secondPoint = dsq->getGameCursorPosition();
	if (!(multiSelectPoint-secondPoint).isLength2DIn(64))
	{
		Vector p1, p2;

		if (secondPoint.x < multiSelectPoint.x)
		{
			p1.x = secondPoint.x;
			p2.x = multiSelectPoint.x;
		}
		else
		{
			p1.x = multiSelectPoint.x;
			p2.x = secondPoint.x;
		}

		if (secondPoint.y < multiSelectPoint.y)
		{
			p1.y = secondPoint.y;
			p2.y = multiSelectPoint.y;
		}
		else
		{
			p1.y = multiSelectPoint.y;
			p2.y = secondPoint.y;
		}

		clearSelection();

		TileStorage& ts = getCurrentLayerTiles();
		for(size_t i = 0; i < ts.tiles.size(); ++i)
		{
			const TileData& t = ts.tiles[i];
			if(t.isVisible() && t.x >= p1.x && t.y >= p1.y && t.x <= p2.x && t.y <= p2.y)
			{
				selectedTiles.push_back(i);
			}
		}

		if(size_t N = selectedTiles.size())
			ts.select(&selectedTiles[0], N);
	}
}

void SceneEditor::unselectTileLayer()
{
	for(size_t i = 0; i < MAX_TILE_LAYERS; ++i)
		dsq->tileRenders[i]->renderBorders = false;
}

void SceneEditor::setActiveLayer(unsigned bglayer, bool force)
{
	if(this->bgLayer == bglayer && !force)
		return;

	unselectTileLayer();
	dsq->tileRenders[bglayer]->renderBorders = true;

	destroyMultiTileHelper();
	clearSelection();
	this->bgLayer = bglayer;
}

void SceneEditor::action(int id, int state, int source, InputDevice device)
{
	if(id >= ACTION_BGLAYER1 && id < ACTION_BGLAYEREND && state)
	{
		const int newLayer = id - ACTION_BGLAYER1;

		if(core->getAltState())
		{
			game->setElementLayerVisible(newLayer, !game->isElementLayerVisible(newLayer));
			return; // do not switch to the layer that was just hidden
		}

		if(editType == ET_ELEMENTS)
		{
			bool change = true;
			if(size_t N = selectedTiles.size())
			{
				TileStorage& ts = getCurrentLayerTiles();

				if (core->getCtrlState())
				{
					change = false;
					if (id == ACTION_BGLAYEREND)
					{
						ts.setEffect(dsq->tilemgr.tileEffects, -1, &selectedTiles[0], N);
					}
					else if (id >= ACTION_BGLAYER1 && id < ACTION_BGLAYER10)
					{
						ts.setEffect(dsq->tilemgr.tileEffects, id - ACTION_BGLAYER1, &selectedTiles[0], N);
					}
				}
				else
				{
					updateText();

					if (core->getShiftState())
					{
						if(newLayer < MAX_TILE_LAYERS)
						{
							TileStorage& dst = dsq->tilemgr.tilestore[newLayer];
							const size_t idx = ts.moveToOther(dst, &selectedTiles[0], N);
							setActiveLayer(newLayer); // this clears selected tiles
							// update selected tiles so that when we switch to the layer they are still selected
							assert(selectedTiles.empty());
							for(size_t i = 0; i < N; ++i)
								selectedTiles.push_back(idx + i);
							//ts.changeFlags(TILEFLAG_SELECTED, 0, &selectedTiles[0], N); // they still have that flag
							change = false;
						}
					}
				}
			}
			if(change)
				setActiveLayer(newLayer);
		}
	}

	if (id == ACTION_MULTISELECT && this->state == ES_SELECTING)
	{
		if (state)
		{

			if (!multiSelecting)
			{
				multiSelecting = true;
				clearSelection();
			}
			multiSelectPoint = dsq->getGameCursorPosition();
		}
		else
		{
			multiSelecting = false;
		}
	}
}

void SceneEditor::rotateElement()
{
	placer->rotation += Vector(0, 0, 45);
	if (placer->rotation.z > 360)
		placer->rotation.z -= 360;
}

void SceneEditor::rotateElement2()
{
	placer->rotation -= Vector(0, 0, 45);
	if (placer->rotation.z < 0)
		placer->rotation.z += 360;
}

bool SceneEditor::loadSceneByName()
{
	std::string s = dsq->getUserInputString("Enter Name of Map to Load");
	if (!s.empty())
	{
		game->noSceneTransitionFadeout = true;
		game->fullTilesetReload = true;
		game->transitionToScene(s);
		return true;
	}
	return false;
}

void SceneEditor::reloadScene()
{
	debugLog("reloadScene");
	game->noSceneTransitionFadeout = true;
	game->fullTilesetReload = true;
	game->positionToAvatar = game->avatar->position;
	game->transitionToScene(game->sceneName);
}


void SceneEditor::loadScene()
{
	bool reload;
	if (core->getShiftState())
	{
		reload = loadSceneByName();
	}
	else
	{
		reloadScene();
		reload = true;
	}

	if(reload)
	{
		// HACK: reload stuff when (re-) loading a map this way
		particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);
		Shot::loadShotBank(dsq->shotBank1, dsq->shotBank2);
		game->loadEntityTypeList();
		dsq->loadTileEffects();
		dsq->continuity.loadSongBank();
		dsq->loadStringBank();
	}
}

void SceneEditor::saveScene()
{
	if(game->saveScene(game->sceneName))
		dsq->screenMessage(game->sceneName + " Saved!");
	else
		dsq->screenMessage(game->sceneName + " FAILED to save!");
}


Quad *se_grad = 0;

void destroyEntityPage()
{
	if (se_label)
	{
		se_label->safeKill();
		se_label = 0;
	}
	if (se_grad)
	{

		se_grad->setLife(1);
		se_grad->setDecayRate(10);
		se_grad->fadeAlphaWithLife = 1;
		se_grad = 0;
	}
	for (size_t i = 0; i < qs.size(); i++)
	{
		qs[i]->safeKill();
	}
	qs.clear();
}

void createEntityPage()
{
	int c = 0;
	int rowSize = 12;
	int sizing = 64;
	Vector basePos = Vector(48, 164);

	destroyEntityPage();

	se_grad = new Quad();


	se_grad->scale = Vector(800, 500);
	se_grad->position = Vector(400,350);
	se_grad->followCamera = 1;
	se_grad->alpha = 0;
	se_grad->color = Vector(0,0,0);
	se_grad->alpha.interpolateTo(0.8f, 0.2f);
	game->addRenderObject(se_grad, LR_HUD);

	EntityGroup &group = game->entityGroups[game->sceneEditor.entityPageNum];

	for (size_t i = 0; i < group.entities.size(); i++)
	{
		EntityGroupEntity ent = group.entities[i];
		EntityClass *ec = game->getEntityClassForEntityType(ent.name);
		if (!ec && !dsq->mod.isActive())
		{
			errorLog("Entity Page List Error: Typo in Entities.txt?");
		}
		int type = -1;
		if (ec)
		{
			size_t j=0;
			for (j = 0; j < game->entityTypeList.size(); j++)
			{
				if (ec->idx == game->entityTypeList[j].idx)
					break;
			}
			type = j;
		}

		int bit = int(floorf(float(c/rowSize)));  // FIXME: Not float(c)/rowSize?  --achurch
		std::string prevGfx = ent.gfx;
		if (prevGfx.empty() && ec)
			prevGfx = ec->prevGfx;
		std::string ecName = ent.name;
		if (ec)
			ecName = ec->name;
		if (!ecName.empty())
		{
			SEQuad *q = new SEQuad(prevGfx, Vector((c-(bit*rowSize)), bit)*sizing, type, ecName);
			q->position += basePos;
			if (q->getWidth() > q->getHeight())
			{
				q->setWidthHeight(sizing, (q->getHeight()*sizing) / q->getWidth());
			}
			else
			{
				q->setWidthHeight((q->getWidth()*sizing) / q->getHeight(), sizing);
			}

			q->followCamera = 1;
			game->addRenderObject(q, LR_HUD);
			qs.push_back(q);
			c++;
		}
	}

	se_label = new DebugFont();
	se_label->setFontSize(10);
	se_label->setText(group.name);
	se_label->position = Vector(20,90);
	se_label->followCamera = 1;
	game->addRenderObject(se_label, LR_HUD);
}

void nextEntityPage()
{
	game->sceneEditor.entityPageNum++;
	if (game->sceneEditor.entityPageNum >= game->entityGroups.size())
		game->sceneEditor.entityPageNum = 0;

	createEntityPage();
}


void prevEntityPage()
{
	game->sceneEditor.entityPageNum--;
	if (game->sceneEditor.entityPageNum >= game->entityGroups.size())
		game->sceneEditor.entityPageNum = game->entityGroups.size()-1;

	createEntityPage();
}

void SceneEditor::selectEntityFromGroups()
{
	createEntityPage();

	placer->alpha = 0;
	se_changedEntityType = false;
	editType = ET_SELECTENTITY;

	bool mbld = false;
	bool ld = false, rd = false;
	ld = core->getKeyState(KEY_E);
	while (!se_changedEntityType)
	{
		if (core->mouse.buttons.left && !mbld)
			mbld = true;
		else if (!core->mouse.buttons.left && mbld)
		{
			mbld = false;
			if (core->mouse.position.y < 100)
				break;
		}

		if (core->getKeyState(KEY_ESCAPE))
			break;
		if (core->getKeyState(KEY_SPACE))
			break;

		if (!core->getKeyState(KEY_E))
			ld = false;
		else if (!ld)
		{
			ld=true;
			nextEntityPage();
		}

		if (!core->getKeyState(KEY_R))
			rd = !true;
		else if (!rd)
		{
			rd=!false;
			prevEntityPage();
		}

		core->run(FRAME_TIME);
	}

	destroyEntityPage();

	editType = ET_ENTITIES;
	updateEntityPlacer();
	placer->alpha = 1;
}


void SceneEditor::updateEntityPlacer()
{
	placer->setTexture(selectedEntity.prevGfx);
	placer->scale = Vector(selectedEntity.prevScale,selectedEntity.prevScale);
}

void SceneEditor::cycleSelectedTiles(int direction)
{
	size_t n = selectedTiles.size();
	if(!n)
		return;
	TileStorage& ts = getCurrentLayerTiles();
	const int maxn = (int)dsq->tilemgr.tileset.elementTemplates.size();
	if(!maxn)
		return;
	for(size_t i = 0; i < n; ++i)
	{
		TileData& t = ts.tiles[selectedTiles[i]];
		const ElementTemplate *adj = dsq->tilemgr.tileset.getAdjacent(t.et->idx, direction, true, 1024);
		if(adj)
			t.et = adj;
	}

	ts.refreshAll();
	checkForRebuild();
}

void SceneEditor::nextElement()
{
	if (state != ES_SELECTING) return;


	if (core->getCtrlState())
	{
		dsq->mod.recache();
		return;
	}

	if (core->getAltState())
	{
		debugLog("rebuilding level!");
		game->reconstructGrid(true);
		return;
	}

	if (editType == ET_ELEMENTS)
	{
		if (core->getCtrlState())
		{
			size_t n = selectedTiles.size();
			TileStorage& ts = getCurrentLayerTiles();
			for(size_t i = 0; i < n; ++i)
			{
				TileData& t = ts.tiles[selectedTiles[i]];
				t.rotation = 0;
			}
			checkForRebuild();
		}
		else if (!selectedTiles.empty())
		{
			cycleSelectedTiles(1);
		}
		else
		{
			cyclePlacer(1);
		}
	}
}

void SceneEditor::prevElement()
{
	if (state != ES_SELECTING) return;

	if (editType != ET_SELECTENTITY)
	{
		if (core->getCtrlState())
		{
			debugLog("SELECT ENTITY FROM GROUPS!");
			selectEntityFromGroups();
			return;
		}
	}

	if (editType == ET_ELEMENTS)
	{
		if (!selectedTiles.empty())
		{
			cycleSelectedTiles(-1);
		}
		else
		{
			cyclePlacer(-1);
		}
	}
}

void SceneEditor::cyclePlacer(int direction)
{
	const ElementTemplate *next = dsq->tilemgr.tileset.getAdjacent(curElementId, direction, true, 1024);

	if (next)
	{
		placer->setTexture(next->gfx);
		curElementId = next->idx;
	}
}

void SceneEditor::selectZero()
{
	if (state != ES_SELECTING) return;

	if (editType == ET_ELEMENTS)
	{
		curElementId = 0;
		const ElementTemplate *et = dsq->tilemgr.tileset.getIfExists(0);
		placer->setTexture(et ? et->gfx : "missingimage");
	}
}

void SceneEditor::selectEnd()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ELEMENTS)
	{
		const ElementTemplate *next = dsq->tilemgr.tileset.getAdjacent(0, -1, true, 1024);
		if(next)
		{
			curElementId = next->idx;
			placer->setTexture(next->gfx);
		}
	}
}

void SceneEditor::placeElement()
{
	if (editType == ET_ELEMENTS)
	{
		if (!core->getShiftState() && !core->getKeyState(KEY_LALT))
		{
			dsq->tilemgr.createOneTile(curElementId, bgLayer, placer->position.x, placer->position.y);
			// FIXME: need to update grid or no?
			updateText();
		}
	}
	else if (editType == ET_ENTITIES)
	{
		Entity *e = game->createEntityOnMap(selectedEntity.name.c_str(), dsq->getGameCursorPosition());
		if(e)
			e->postInit();
	}
	else if (editType == ET_PATHS)
	{
		if (core->getCtrlState())
		{
			// new path
			Path *p = new Path;
			p->name = "";
			PathNode n;
			n.position = dsq->getGameCursorPosition();
			p->nodes.push_back(n);
			game->addPath(p);
			selectedIdx = game->getNumPaths()-1;
		}
		else
		{
			Path *p = getSelectedPath();
			if (p)
			{
				p->addNode(selectedNode);
			}
		}
	}
}

void SceneEditor::cloneSelectedElement()
{
	if (editType == ET_ELEMENTS)
	{
		if (size_t n = selectedTiles.size())
		{
			TileStorage& ts = getCurrentLayerTiles();
			size_t newidx = ts.cloneSome(dsq->tilemgr.tileEffects, &selectedTiles[0], n);

			assert(!multi);
			unsigned allflags = 0;

			// select the clones
			clearSelection();
			for(size_t i = 0; i < n; ++i)
			{
				selectedTiles.push_back(newidx + i);
				TileData& t = ts.tiles[newidx + i];
				allflags |= t.flags;
				t.flags |= TILEFLAG_SELECTED;
				t.x += 40;
				t.y += 40;
			}

			if(allflags & TILEFLAG_SOLID)
				game->reconstructGrid(true);
		}
	}
	else if (editType == ET_ENTITIES)
	{
		// TODO: make this work
	}
	else if (editType == ET_PATHS)
	{
		if (editingPath)
		{
			Path *p = editingPath;
			Path *newp = new Path;
			newp->name = p->name;
			newp->nodes = p->nodes;
			newp->rect = p->rect;
			newp->refreshScript();
			newp->pathShape = p->pathShape;
			newp->nodes[0].position += Vector(64,64);

			game->addPath(newp);
			selectedIdx = game->getNumPaths()-1;
			newp->init();
		}
	}
}

void SceneEditor::shutdown()
{
	clearActions();
	text = 0;
	placer = 0;
}

void SceneEditor::toggle()
{
	toggle(!on);
}

void SceneEditor::toggle(bool on)
{
	if (core->getNestedMains() > 1) return;
	if (game->isInGameMenu()) return;
	if (!on && editType == ET_SELECTENTITY) return;
	if (game->worldMapRender && game->worldMapRender->isOn()) return;
	this->on = on;
	autoSaveTimer = 0;

	execID = -1;
	if (on)
	{
		btnMenu->alpha = 1;
		dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = true;
		dsq->tileRenders[bgLayer]->renderBorders = true;

		game->togglePause(on);
		if (game->avatar)
			game->avatar->disableInput();
		text->alpha.interpolateTo(1, 0.5);
		placer->alpha.interpolateTo(0.5, 0.5);
		movingEntity = 0;
		dsq->toggleCursor(true);
		dsq->setCursor(CURSOR_NORMAL);


		dsq->darkLayer.toggle(false);

		for (int i = LR_ELEMENTS1; i <= LR_ELEMENTS8; i++)
		{
			dsq->getRenderObjectLayer(i)->update = true;
		}

		oldGlobalScale = core->globalScale;
		const float cameraOffset = 1/oldGlobalScale.x - 1/zoom.x;
		core->cameraPos.x += cameraOffset * core->getVirtualWidth()/2;
		core->cameraPos.y += cameraOffset * core->getVirtualHeight()/2;
		core->globalScale = zoom;
		core->globalScaleChanged();
	}
	else
	{
		btnMenu->alpha = 0;

		destroyMultiTileHelper();
		clearSelection();
		for (int i = 0; i < 9; i++)
			dsq->getRenderObjectLayer(LR_ELEMENTS1+i)->visible = true;

		movingEntity = 0;

		dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = false;
		dsq->tileRenders[bgLayer]->renderBorders = false;

		game->togglePause(on);
		if (game->avatar)
			game->avatar->enableInput();
		text->alpha.interpolateTo(0, 0.2f);
		placer->alpha.interpolateTo(0, 0.2f);

		dsq->darkLayer.toggle(true);

		const float cameraOffset = 1/oldGlobalScale.x - 1/zoom.x;
		core->cameraPos.x -= cameraOffset * core->getVirtualWidth()/2;
		core->cameraPos.y -= cameraOffset * core->getVirtualHeight()/2;
		core->globalScale = oldGlobalScale;
	}
}

bool SceneEditor::isOn()
{
	return on;
}

void SceneEditor::updateText()
{
	btnMenu->position = Vector(20, 20 - core->getVirtualOffY());

	const Vector cursor = dsq->getGameCursorPosition();
	TileVector tv(cursor);

	std::ostringstream os;
	os << game->sceneName << " bgL[" << bgLayer << "] (" <<
		(int)dsq->cameraPos.x << "," << (int)dsq->cameraPos.y << ") ("
		<< (int)cursor.x << "," << (int)cursor.y << ") T("
		<< tv.x << "," << tv.y << ") ";
	switch(editType)
	{
	case ET_ELEMENTS:
		os << "elements (" << dsq->tilemgr.getNumTiles() << ")";
		if (selectedTiles.size() > 1)
		{
			os << " - " << selectedTiles.size() << " selected";
		}
		else if(selectedTiles.size() == 1)
		{
			const TileData& t = getCurrentLayerTiles().tiles[selectedTiles[0]];
			os << " id: " << t.et->idx;
			os << " efx: " << (t.eff ? (t.eff->efxidx + 1) : 0); // +1 so that it resembles the layout on numpad
			os << " tag: " << t.tag;
			os << " gfx: " << t.et->gfx;
			os << " F:" << ((t.flags & TILEFLAG_FH) ? "H" : "") << ((t.flags & TILEFLAG_FV) ? "V" : "");
		}
		else
		{
			os << " // id: " << curElementId;
			const ElementTemplate *et = dsq->tilemgr.tileset.getIfExists(curElementId);
			if(et)
				os << " gfx: " << et->gfx;
		}
	break;
	case ET_ENTITIES:
		os << "entities (" << dsq->entities.size() << ")";
		if (editingEntity)
		{
			os.precision(1);
			os << std::fixed;
			os << " id: " << editingEntity->getID()
				<< " name: " << editingEntity->name
				<< " flag:" << dsq->continuity.getEntityFlag(game->sceneName, editingEntity->getID())
				<< " fh:" << editingEntity->isfh()
				<< " fv:" << editingEntity->isfv()
				<< " state:" << editingEntity->getState()
				<< " et:" << editingEntity->getEntityType()
				<< " hp:" << editingEntity->health << "/" << editingEntity->maxHealth;
		}
	break;
	case ET_PATHS:
		os << "paths (" << game->getNumPaths() << ")";
		if (selectedIdx != size_t(-1))
			os << " si[" << selectedIdx << "]";
		if (getSelectedPath())
			os << " name: " << getSelectedPath()->name;
	break;
	case ET_SELECTENTITY:
	case ET_MAX:
		break;
	}
	text->setText(os.str());
}

void SceneEditor::update(float dt)
{
	if (on)
	{
		if (core->getNestedMains() > 1) return;

		autoSaveTimer += dt;
		if (autoSaveTimer > vars->autoSaveTime && state == ES_SELECTING)
		{
			autoSaveTimer = 0;
			std::ostringstream os;
			os << "auto/AUTO_" << autoSaveFile << "_" << game->sceneName;
			if(game->saveScene(os.str()))
			{
				std::string m = "Map AutoSaved to " + os.str();
				debugLog(m);
				dsq->screenMessage(m);
			}

			autoSaveFile++;
			if (autoSaveFile > vars->autoSaveFiles)
				autoSaveFile = 0;
		}

		updateMultiSelect();
		switch (editType)
		{
		case ET_ELEMENTS:
		{
			bool ismulti = isActing(ACTION_MULTISELECT, -1);
			int sel = -1;
			if (state == ES_SELECTING && !ismulti)
			{
				sel = this->getTileAtCursor();
				if(selectedTiles.empty() || (selectedTiles.size() == 1 && selectedTiles[0] != sel))
				{
					if(!selectedTiles.empty())
						clearSelection();
					if(sel >= 0)
					{
						selectedTiles.push_back(sel);
						const size_t idx = sel;
						getCurrentLayerTiles().select(&idx, 1);
					}
				}
			}

			if (sel >= 0 || ismulti)
				placer->alpha = 0;
			else
				placer->alpha = 0.5;
		}
		break;
		case ET_ENTITIES:
			if (state == ES_SELECTING)
				editingEntity = this->getEntityAtCursor();
			if (editingEntity)
				placer->alpha = 0;
			else
				placer->alpha = 0.5;
		break;
		case ET_PATHS:
		case ET_SELECTENTITY:
		case ET_MAX:
			break;
		}

		updateText();
		ActionMapper::onUpdate(dt);

		TileVector cursorTile(dsq->getGameCursorPosition());
		Vector p = Vector(cursorTile.x*TILE_SIZE+TILE_SIZE/2, cursorTile.y*TILE_SIZE+TILE_SIZE/2);
		placer->position = p;

		int camSpeed = 500/zoom.x;
		if (core->getShiftState())
			camSpeed = 5000/zoom.x;
		if (isActing(ACTION_CAMLEFT, -1))
			dsq->cameraPos.x -= dt*camSpeed;
		if (isActing(ACTION_CAMRIGHT, -1))
			dsq->cameraPos.x += dt*camSpeed;
		if (isActing(ACTION_CAMUP, -1))
			dsq->cameraPos.y -= dt*camSpeed;
		if (isActing(ACTION_CAMDOWN, -1))
			dsq->cameraPos.y += dt*camSpeed;
		if (core->mouse.buttons.middle && !core->mouse.change.isZero())
		{
			dsq->cameraPos += core->mouse.change*(4/zoom.x);
			core->setMousePosition(core->mouse.lastPosition);
		}

		float spd = 0.5;
		const Vector oldZoom = zoom;
		if (isActing(ACTION_ZOOMOUT, -1))
			zoom /= (1 + spd*dt);
		else if (isActing(ACTION_ZOOMIN, -1))
			zoom *= (1 + spd*dt);
		else if (core->mouse.scrollWheelChange < 0)
		{
			if (core->getShiftState() && !core->getCtrlState()) // hackish: to prevent accidental recache()
				nextElement();
			else
				zoom /= 1.12f;
		}
		else if (core->mouse.scrollWheelChange > 0)
		{
			if (core->getShiftState() && !core->getCtrlState()) // hackish: to prevent accidental entity selection
				prevElement();
			else
				zoom *= 1.12f;
		}
		if (zoom.x < 0.04f)
			zoom.x = zoom.y = 0.04f;
		core->globalScale = zoom;
		core->globalScaleChanged();
		if (zoom.x != oldZoom.x)
		{
			const float mouseX = core->mouse.position.x;
			const float mouseY = core->mouse.position.y;
			dsq->cameraPos.x += mouseX/oldZoom.x - mouseX/zoom.x;
			dsq->cameraPos.y += mouseY/oldZoom.y - mouseY/zoom.y;
		}

		if (this->editType == ET_PATHS)
		{
			switch(state)
			{
			case ES_SELECTING:
			{
				selectedIdx = -1;
				selectedNode = -1;
				float smallestDist = sqr(64);
				for (size_t i = 0; i < game->getNumPaths(); i++)
				{
					if(game->getPath(i)->nodes.size() == 0) {
						continue;
					}
					for (size_t n = game->getPath(i)->nodes.size(); n-- > 0; )
					{
						Vector v = game->getPath(i)->nodes[n].position - dsq->getGameCursorPosition();
						float dist = v.getSquaredLength2D();
						if (dist < smallestDist)
						{
							smallestDist = dist;
							selectedIdx = i;
							selectedNode = n;

						}
					}
				}
			}
			break;
			case ES_SCALING:
			{
				if (editingPath)
				{
					float factor = 1;
					Vector add = (dsq->getGameCursorPosition() - cursorOffset)*factor;
					Vector sz = oldScale + add;
					if (sz.x < 32)
						sz.x = 32;
					if (sz.y < 32)
						sz.y = 32;
					editingPath->rect.x1 = -sz.x/2;
					editingPath->rect.x2 = sz.x/2;
					editingPath->rect.y1 = -sz.y/2;
					editingPath->rect.y2 = sz.y/2;
				}
			}
			break;
			case ES_MOVING:
			{
				if(Path *p = game->getPath(selectedIdx))
					p->nodes[selectedNode].position = dsq->getGameCursorPosition() + cursorOffset;
			}
			break;
			case ES_ROTATING:
			case ES_MAX:
				break;
			}
		}
		else if (editType == ET_ENTITIES)
		{
			switch(state)
			{
			case ES_MOVING:
				if (editingEntity)
					editingEntity->position = dsq->getGameCursorPosition() + cursorOffset;
			break;
			case ES_ROTATING:
			{
				if (editingEntity)
				{
					float add = (dsq->getGameCursorPosition().x - cursorOffset.x)/2.4f;
					if (core->getCtrlState())
					{
						int a = (oldRotation + add)/45;
						add = a * 45;
						editingEntity->rotation.z = add;
					}
					else
					{
						editingEntity->rotation.z = oldRotation + add;
					}
				}
			}
				break;
			case ES_SELECTING:
			case ES_SCALING:
			case ES_MAX:
				break;
			}
		}
		else if (editType == ET_ELEMENTS)
		{
			switch(state)
			{
			case ES_SELECTING:
			{
			}
			break;
			case ES_MOVING:
				// FIXME: this should be relative? check this
				updateSelectedElementPosition(dsq->getGameCursorPosition() - cursorOffset);
			break;
			case ES_ROTATING:
			{
				if (!selectedTiles.empty())
				{
					assert(multi);
					float add = (dsq->getGameCursorPosition().x - cursorOffset.x)/2.4f;
					if (core->getCtrlState())
					{
						int a = (oldRotation + add)/45;
						add = a * 45;
						multi->rotation.z = add;
					}
					else
					{
						multi->rotation.z = oldRotation + add;
					}
				}
			}
			break;
			case ES_SCALING:
			{
				bool right=false, middle=false, down=false, uni=false, repeatScale=false;
				bool noSide = 0;
				if (cursorOffset.x > oldPosition.x+10)
					right = true;
				else if (cursorOffset.x < oldPosition.x-10)
					right = false;
				else
					noSide = true;
				if (cursorOffset.y > oldPosition.y+10)
					down = true;
				else if (cursorOffset.y < oldPosition.y-10)
					down = false;
				else if (noSide)
					middle = true;

				Vector add = Vector((dsq->getGameCursorPosition().x - cursorOffset.x)/100.0f,
					(dsq->getGameCursorPosition().y - cursorOffset.y)/100.0f);
				{
					if (core->getKeyState(KEY_C))
					{
						add.y = 0;
						if (!right && !middle)
							add.x *= -1;
					}
					else if (core->getKeyState(KEY_V))
					{
						add.x = 0;
						if (!down && !middle)
							add.y *= -1;
					}
					else
					{
						add.y = add.x;
						uni = true;
					}

					repeatScale = core->getKeyState(KEY_X);
					if(repeatScale)
						add *= 0.3f;
				}
				if (!selectedTiles.empty())
				{
					if (!core->getCtrlState())
					{
						if(repeatScale)
						{
							if(selectedTiles.size() == 1)
								multi->changeRepeatScale(oldRepeatScale + add);
						}
						else
						{
							multi->scale=oldScale + add;
							if (multi->scale.x < MIN_SIZE)
								multi->scale.x = MIN_SIZE;
							if (multi->scale.y < MIN_SIZE)
								multi->scale.y = MIN_SIZE;

							if(!middle && !uni && selectedTiles.size() == 1)
							{
								Vector offsetChange = (add*Vector(multi->getWidth(), multi->getHeight()))*0.5f;
								offsetChange.rotate2D360(multi->rotation.z);
								if (add.y == 0)
								{
									if (right)
										multi->offset = offsetChange;
									else
										multi->offset = -offsetChange;
								}
								else
								{
									if (down)
										multi->offset = offsetChange;
									else
										multi->offset = -offsetChange;
								}
							}
						}
					}
				}
			}
			break;
			case ES_MAX:
				break;
			}
		}
	}
}

void SceneEditor::nextEntityType()
{
	if (editType == ET_ELEMENTS && state == ES_SELECTING)
	{
		if (!selectedTiles.empty())
		{
			enterMoveState();
			updateSelectedElementPosition(Vector(1,0));
			exitMoveState();
		}
	}
	else if (editType == ET_SELECTENTITY)
	{
		nextEntityPage();
	}
}

void SceneEditor::prevEntityType()
{
	if (editType == ET_ELEMENTS && state == ES_SELECTING)
	{
		if (!selectedTiles.empty())
		{
			enterMoveState();
			updateSelectedElementPosition(Vector(-1,0));
			exitMoveState();
		}
	}
	else if (editType == ET_SELECTENTITY)
	{
		prevEntityPage();
	}
}

void SceneEditor::dumpObs()
{
	TileVector tv;
	unsigned char *data = new unsigned char[MAX_GRID * MAX_GRID * sizeof(unsigned)];
	unsigned *ptr = (unsigned*)data;
	for(tv.y = 0; tv.y < MAX_GRID; ++tv.y)
		for(tv.x = 0; tv.x < MAX_GRID; ++tv.x)
			*ptr++ = game->isObstructed(tv, OT_MASK_BLACK) ? 0xFF000000 : 0xFFFFFFFF;
	std::string outfn = dsq->getUserDataFolder() + "/griddump-" + game->sceneName + ".png";
	if(pngSaveRGBA(outfn.c_str(), MAX_GRID, MAX_GRID, data, 3))
		dsq->screenMessage("Saved grid image to " + outfn);
	else
		dsq->screenMessage("Failed to save grid dump: " + outfn);
	delete [] data;
}

void SceneEditor::moveEverythingBy(int dx, int dy)
{
	const Vector mv(dx, dy);

	FOR_ENTITIES(ei)
	{
		Entity *e = *ei;
		e->startPos += mv;
		e->position += mv;
	}

	const size_t nn = game->getNumPaths();
	for(size_t i = 0; i < nn; ++i)
	{
		Path *p = game->getPath(i);
		const size_t n = p->nodes.size();
		for(size_t ii = 0; ii < n; ++ii)
			p->nodes[ii].position += mv;
	}

	for(unsigned lr = 0; lr < MAX_TILE_LAYERS; ++lr)
	{
		TileStorage& ts = tilesForLayer(lr);
		const size_t ne = ts.tiles.size();
		for(size_t i = 0; i < ne; ++i)
		{
			TileData& t = ts.tiles[i];
			t.x += mv.x;
			t.y += mv.y;
		}
	}

	tinyxml2::XMLElement *sf = game->saveFile->FirstChildElement("SchoolFish");
	for( ; sf; sf = sf->NextSiblingElement("SchoolFish"))
	{
		int sx = sf->IntAttribute("x");
		int sy = sf->IntAttribute("y");
		sf->SetAttribute("x", sx + dx);
		sf->SetAttribute("y", sy + dy);
	}
}

TileStorage& SceneEditor::getCurrentLayerTiles()
{
	return tilesForLayer(this->bgLayer);
}

MultiTileHelper * SceneEditor::createMultiTileHelperFromSelection()
{
	assert(!multi);
	if(selectedTiles.empty())
		return NULL;
	return (multi = MultiTileHelper::New(bgLayer, &selectedTiles[0], selectedTiles.size()));
}

void SceneEditor::destroyMultiTileHelper()
{
	if(multi)
	{
		multi->finish();
		multi = NULL;
	}
}
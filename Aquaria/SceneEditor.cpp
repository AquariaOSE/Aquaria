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


#ifdef BBGE_BUILD_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <shellapi.h>
#endif

const int minSelectionSize = 64;
PathRender *pathRender;


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
		index = dsq->game->entityTypeList[idx].idx;
		prevGfx = dsq->game->entityTypeList[idx].prevGfx;
		prevScale = dsq->game->entityTypeList[idx].prevScale;
		name = dsq->game->entityTypeList[idx].name;
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
		return std::string(dsq->mod.getPath() + "maptemplates/" + dsq->game->sceneName + ".png");
	else
		return std::string("maptemplates/" + dsq->game->sceneName + ".png");
	return "";
}

SceneEditor::SceneEditor() : ActionMapper(), on(false)
{
	autoSaveFile = 0;
	selectedIdx = -1;
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

	if (dsq->game->grad)
	{
		dsq->game->grad->makeVertical(Vector(g1r, g1g, g1b), Vector(g2r, g2g, g2b));
	}
}

void SceneEditor::changeDepth()
{
	if (editingElement)
	{
		editingElement->followCamera = 0.9f;
		editingElement->cull = false;
	}
}

void SceneEditor::updateSelectedElementPosition(Vector dist)
{
	if (state == ES_MOVING)
	{
		if (!selectedElements.empty())
		{
			dummy.position = oldPosition + dist;
		}
		else if (editingElement)
		{
			editingElement->position = oldPosition + dist;
		}
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


		if (dsq->game->sceneEditor.selectedEntity.name == selectedEntity.name
			|| (entType != -1 && dsq->game->sceneEditor.selectedEntity.typeListIndex == entType))
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
					dsq->game->sceneEditor.selectedEntity.setIndex(entType);
				else
				{
					dsq->game->sceneEditor.selectedEntity.setName(selectedEntity.name, selectedEntity.prevGfx);
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
			dsq->game->sceneEditor.closeMainMenu();
		}
		else
		{
			dsq->game->sceneEditor.openMainMenu();
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
		dsq->game->reconstructGrid(true);
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
		os << dsq->game->gradTop.x << " " << dsq->game->gradTop.y << " " << dsq->game->gradTop.z;
		std::string read;
		read = dsq->getUserInputString("Enter Spaced R G B 0-1 Values for the Top Color (e.g. 1 0.5 0.5 for bright red)", os.str());

		if (!read.empty())
		{
			std::istringstream is(read);
			is >> dsq->game->gradTop.x >> dsq->game->gradTop.y >> dsq->game->gradTop.z;
		}

		std::ostringstream os2;
		os2 << dsq->game->gradBtm.x << " " << dsq->game->gradBtm.y << " " << dsq->game->gradBtm.z;
		read = dsq->getUserInputString("Enter Spaced R G B 0-1 Values for the Bottom Color (e.g. 0 0 0 for black)", os2.str());

		if (!read.empty())
		{
			std::istringstream is2(read);
			is2 >> dsq->game->gradBtm.x >> dsq->game->gradBtm.y >> dsq->game->gradBtm.z;
		}

		dsq->game->createGradient();
	}
	break;
	case 111:
	{
		std::string track = dsq->getUserInputString("Set Background Music Track", dsq->game->saveMusic);
		if (!track.empty())
		{
			dsq->game->setMusicToPlay(track);
			dsq->game->saveMusic = track;
			dsq->game->updateMusic();
		}
	}
	break;
	case 112:
	{
		selectEntityFromGroups();
	}
	break;
	case 113:
		dsq->game->toggleGridRender();
	break;
	case 114:
		dsq->screenshot();
	break;
	case 115:
	{
		dsq->returnToScene = dsq->game->sceneName;
		core->enqueueJumpState("AnimationEditor");
	}
	break;
	case 120:
	{
		dsq->returnToScene = dsq->game->sceneName;
		core->enqueueJumpState("ParticleEditor");
	}
	break;
	case 116:
		regenLevel();
	break;
	case 130:
		dsq->mod.recache();
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
	dsq->game->addRenderObject(b, LR_HUD);
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
	if (dsq->game->gridRender)
		addMainMenuItem(std::string("TOGGLE TILE COLLISION RENDER ") + ((dsq->game->gridRender->alpha!=0) ? "OFF" : "ON ") + std::string("       (F9)"),            113);
	addMainMenuItem("SCREENSHOT                                 ",            114);



	addMainMenuItem("PARTICLE VIEWER                            ",            120);
	addMainMenuItem("ANIMATION EDITOR                           ",            115);

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
	selectedElements.clear();
	autoSaveTimer = 0;
	skinMinX = skinMinY = skinMaxX = skinMaxY = -1;
	editingElement = 0;
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

	dsq->game->addRenderObject(btnMenu, LR_HUD);

	selectedEntityType = 0;
	zoom = Vector(0.2f,0.2f);
	bgLayer = 5;
	text = new DebugFont();
	text->setFontSize(6);

	text->followCamera = 1;
	text->position = Vector(125,20,4.5);

	dsq->game->addRenderObject(text, LR_HUD);
	text->alpha = 0;
	selectedVariation = -1;
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

	addAction(MakeFunctionEvent(SceneEditor, alignHorz), KEY_C, 1);
	addAction(MakeFunctionEvent(SceneEditor, alignVert), KEY_V, 1);



	addAction(MakeFunctionEvent(SceneEditor, placeElement), KEY_SPACE, 1);

	addAction(MakeFunctionEvent(SceneEditor, enterName), KEY_N, 0);
	addAction(MakeFunctionEvent(SceneEditor, changeShape), KEY_Y, 0);
	addAction(MakeFunctionEvent(SceneEditor, reversePath), KEY_T, 0);


	addAction(MakeFunctionEvent(SceneEditor, moveLayer), KEY_F10, 0);

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
	dsq->game->addRenderObject(placer, LR_HUD);
	placer->alpha = 0;
	curElement = 0;
	selectedEntity.clear();
	nextElement();


	if (curElement < dsq->game->elementTemplates.size())
	{
		placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
		placer->scale = Vector(1,1);
	}
	else
	{
		placer->setTexture("missingimage");
	}

	updateText();

	doPrevElement();
}

void SceneEditor::alignHorz()
{
	if (core->getShiftState()) return;
	if (editType == ET_ELEMENTS && state == ES_SELECTING && editingElement)
	{
		TileVector t(editingElement->position);
		int startOn = dsq->game->getGrid(t);
		TileVector c=t;
		bool found = false;
		int dir = -1;
		for (int i = 1; i < 5; i++)
		{
			// search down
			c.y = t.y + i;
			if (dsq->game->getGrid(c) != startOn)
			{
				found = true;
				dir = 1;
				break;
			}
			c.y = t.y - i;
			if (dsq->game->getGrid(c) != startOn)
			{
				found = true;
				dir = -1;
				break;
			}
		}
		if (found)
		{
			editingElement->position.y = c.worldVector().y + (editingElement->texture->getPixelHeight()/2)*(-dir);
		}
	}
}

void SceneEditor::alignVert()
{
	if (core->getShiftState()) return;
	if (editType == ET_ELEMENTS && state == ES_SELECTING && editingElement)
	{
		TileVector t(editingElement->position);
		int startOn = dsq->game->getGrid(t);
		TileVector c=t;
		bool found = false;
		int dir = -1;
		for (int i = 1; i < 5; i++)
		{
			// search down
			c.x = t.x + i;
			if (dsq->game->getGrid(c) != startOn)
			{
				found = true;
				dir = 1;
				break;
			}
			c.x = t.x - i;
			if (dsq->game->getGrid(c) != startOn)
			{
				found = true;
				dir = -1;
				break;
			}
		}
		if (found)
		{
			editingElement->position.x = c.worldVector().x + (editingElement->texture->getPixelWidth()/2)*(-dir);
		}
	}
}

void SceneEditor::createAquarian()
{

	static bool inCreateAqurian = false;
	if (inCreateAqurian) return;

	inCreateAqurian = true;
	std::string t = dsq->getUserInputString("Enter Aquarian:", "");
	stringToUpper(t);
	Vector startPos = dsq->getGameCursorPosition();
	for (size_t i = 0; i < t.size(); i++)
	{
		int v = 0;
		if (t[i] >= 'A' && t[i] <= 'Z')
		{
			v = 1024+int(t[i] - 'A');
		}
		if (t[i] == '.' || t[i] == ' ')
		{
			v = 1024+26;
		}

		dsq->game->createElement(v, startPos + Vector(64*i,0), this->bgLayer);
	}
	inCreateAqurian = false;
}

Path *SceneEditor::getSelectedPath()
{
	if (selectedIdx < dsq->game->getNumPaths())
	{
		return dsq->game->getPath(selectedIdx);
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
	if (selectedElements.size())
		for (size_t i = 0; i < selectedElements.size(); ++i)
			selectedElements[i]->setElementEffectByIndex(gi);
	else if (editingElement)
		editingElement->setElementEffectByIndex(gi);
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
	if (editingElement && !core->getShiftState())
	{
		std::vector<Element*> copy = dsq->getElementsCopy();
		dsq->clearElements();

		//  move to the foreground ... this means that the editing element should be last in the list (Added last)
		for (size_t i = 0; i < copy.size(); i++)
		{
			if (copy[i] != editingElement)
				dsq->addElement(copy[i]);
		}
		dsq->addElement(editingElement);

		editingElement->moveToFront();
	}
}

void SceneEditor::moveToBack()
{
	if (editingElement && !core->getShiftState())
	{
		std::vector<Element*> copy = dsq->getElementsCopy();
		dsq->clearElements();

		//  move to the background ... this means that the editing element should be first in the list (Added first)
		dsq->addElement(editingElement);
		for (size_t i = 0; i < copy.size(); i++)
		{
			if (copy[i] != editingElement)
				dsq->addElement(copy[i]);
		}

		editingElement->moveToBack();
	}
}

void SceneEditor::editModeElements()
{
	selectedIdx = -1;
	editType = ET_ELEMENTS;
	if (curElement < dsq->game->elementTemplates.size())
	{
		placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
		placer->scale = Vector(1,1);
	}
	placer->alpha = 0.5;
	pathRender->alpha = 0;
	editingEntity = NULL;
	editingPath = NULL;
}

void SceneEditor::editModeEntities()
{
	selectedIdx = -1;
	//HACK: methinks target is useless now
	//target->alpha.interpolateTo(0, 0.5);
	editType = ET_ENTITIES;


	placer->setTexture(selectedEntity.prevGfx);
	placer->alpha = 0.5;
	pathRender->alpha = 0;
	selectedElements.clear();
	editingElement = NULL;
	editingPath = NULL;
}

void SceneEditor::editModePaths()
{
	selectedIdx = -1;
	editType = ET_PATHS;
	placer->alpha = 0;
	pathRender->alpha = 0.5;
	selectedElements.clear();
	editingElement = NULL;
	editingEntity = NULL;
}

Element *SceneEditor::getElementAtCursor()
{
	int minDist = -1;
	Element *selected = 0;
	for (Element *e = dsq->getFirstElementOnLayer(this->bgLayer); e; e = e->bgLayerNext)
	{
		if (e->life == 1)
		{
			if (e->isCoordinateInside(dsq->getGameCursorPosition()))
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
		if (selectedElements.size()>0)
		{
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				selectedElements[i]->safeKill();
				dsq->removeElement(selectedElements[i]);
			}
			selectedElements.clear();
			dsq->game->reconstructGrid();
		}
		else if (editingElement)
		{
			editingElement->safeKill();
			dsq->removeElement(editingElement);
			editingElement = 0;
			dsq->game->reconstructGrid();
		}
	}
	else if (editType == ET_ENTITIES)
	{
		if (editingEntity)
		{
			if (editingEntity->getEntityType() != ET_AVATAR)
			{
				dsq->game->removeEntity(editingEntity);
				editingEntity = 0;
			}
		}
	}
	else if (editType == ET_PATHS)
	{
		if (core->getShiftState())
		{
			dsq->game->removePath(selectedIdx);
		}
		else
		{
			if (selectedIdx != -1)
			{
				Path *p = dsq->game->getPath(selectedIdx);
				if (p->nodes.size() == 1)
				{
					dsq->game->removePath(selectedIdx);
					selectedIdx = -1;
				}
				else
					p->removeNode(selectedNode);


			}
		}
	}
}

void SceneEditor::updateSaveFileEnemyPosition(Entity *ent)
{
	XMLElement *exml = dsq->game->saveFile->FirstChildElement("Enemy");
	while (exml)
	{
		int x = atoi(exml->Attribute("x"));
		int y = atoi(exml->Attribute("y"));
		if (ent->startPos.x == x && ent->startPos.y == y)
		{
			ent->startPos = Vector(int(ent->position.x), int(ent->position.y));
			exml->SetAttribute("x", int(ent->startPos.x));
			exml->SetAttribute("y", int(ent->startPos.y));
			return;
		}
		exml = exml->NextSiblingElement("Enemy");
	}
	exml = dsq->game->saveFile->FirstChildElement("Entity");
	while (exml)
	{
		int x = atoi(exml->Attribute("x"));
		int y = atoi(exml->Attribute("y"));
		if (ent->startPos.x == x && ent->startPos.y == y)
		{
			ent->startPos = Vector(int(ent->position.x), int(ent->position.y));
			exml->SetAttribute("x", int(ent->startPos.x));
			exml->SetAttribute("y", int(ent->startPos.y));
			return;
		}
		exml = exml->NextSiblingElement("Entity");
	}

}

void SceneEditor::checkForRebuild()
{
	if (editType == ET_ELEMENTS && state != ES_SELECTING && !selectedElements.empty())
	{
		bool rebuild = false;
		for (size_t i = 0; i < selectedElements.size(); i++)
		{
			if (selectedElements[i]->elementFlag == EF_SOLID || selectedElements[i]->elementFlag == EF_HURT)
			{
				rebuild = true;
				break;
			}
		}
		if (rebuild)
		{
			dsq->game->reconstructGrid();
		}
	}
	else if (editType == ET_ELEMENTS && state != ES_SELECTING && editingElement != 0 && (editingElement->elementFlag == EF_SOLID || editingElement->elementFlag == EF_HURT))
	{
		dsq->game->reconstructGrid();
	}
}

void SceneEditor::exitMoveState()
{
	if (!selectedElements.empty())
	{
		for (size_t i = 0; i < selectedElements.size(); i++)
		{
			selectedElements[i]->position = selectedElements[i]->getWorldPosition();
			dummy.removeChild(selectedElements[i]);
		}
		core->removeRenderObject(&dummy, Core::DO_NOT_DESTROY_RENDER_OBJECT);
	}
	checkForRebuild();
	state = ES_SELECTING;
}

void SceneEditor::enterMoveState()
{
	if (state != ES_SELECTING) return;
	state = ES_MOVING;
	if (editType == ET_ELEMENTS)
	{
		if (!selectedElements.empty())
		{
			dummy.rotation = Vector(0,0,0);
			cursorOffset = dsq->getGameCursorPosition();
			groupCenter = getSelectedElementsCenter();
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				selectedElements[i]->position -= groupCenter;
				dummy.addChild(selectedElements[i], PM_NONE);
			}
			core->addRenderObject(&dummy, selectedElements[0]->layer);
			dummy.cull = false;
			dummy.position = groupCenter;
			oldPosition = dummy.position;
			dummy.scale = Vector(1,1,1);
		}
		else if (editingElement)
		{
			oldPosition = editingElement->position;
			cursorOffset = dsq->getGameCursorPosition();
		}
	}
	else if (editType == ET_ENTITIES)
	{
		oldPosition = editingEntity->position;
		cursorOffset = editingEntity->position - dsq->getGameCursorPosition();
	}
	else if (editType == ET_PATHS)
	{
		oldPosition = dsq->game->getPath(selectedIdx)->nodes[selectedNode].position;
		cursorOffset = oldPosition - dsq->getGameCursorPosition();
	}
}

void SceneEditor::enterRotateState()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ENTITIES)
	{
		state = ES_ROTATING;
		oldRotation = editingEntity->rotation;
		oldPosition = editingEntity->position;
		cursorOffset = dsq->getGameCursorPosition();
	}
	if (editType == ET_ELEMENTS)
	{
		if (!selectedElements.empty())
		{
			state = ES_ROTATING;
			dummy.rotation = Vector(0,0,0);
			oldRotation = dummy.rotation;
			cursorOffset = dsq->getGameCursorPosition();
			groupCenter = getSelectedElementsCenter();
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				selectedElements[i]->position -= groupCenter;
				dummy.addChild(selectedElements[i], PM_NONE);
			}
			core->addRenderObject(&dummy, selectedElements[0]->layer);
			dummy.cull = false;
			dummy.position = groupCenter;
			dummy.scale = Vector(1,1,1);
		}
		else if (editingElement)
		{
			state = ES_ROTATING;
			oldRotation = editingElement->rotation;
			cursorOffset = dsq->getGameCursorPosition();
		}
	}
}

void SceneEditor::enterScaleState()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ELEMENTS)
	{
		if (!selectedElements.empty())
		{
			state = ES_SCALING;
			dummy.rotation = Vector(0,0,0);
			dummy.scale = Vector(1,1,1);
			oldScale = dummy.scale;
			oldRepeatScale = Vector(1, 1); // not handled for multi-selection
			cursorOffset = dsq->getGameCursorPosition();
			groupCenter = getSelectedElementsCenter();
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				selectedElements[i]->position -= groupCenter;
				dummy.addChild(selectedElements[i], PM_NONE);
			}
			core->addRenderObject(&dummy, selectedElements[0]->layer);
			dummy.cull = false;
			dummy.position = groupCenter;
		}
		else if (editingElement)
		{
			oldPosition = editingElement->position;
			state = ES_SCALING;
			oldScale = editingElement->scale;
			oldRepeatScale = editingElement->repeatToFillScale;
			cursorOffset = dsq->getGameCursorPosition();
		}
	}
	else if (editType == ET_PATHS)
	{
		state = ES_SCALING;
		oldScale = Vector(editingPath->rect.x2-editingPath->rect.x1,
			editingPath->rect.y2-editingPath->rect.y1);
		cursorOffset = dsq->getGameCursorPosition();
	}
}

void SceneEditor::updateEntitySaveData(Entity *editingEntity)
{
	if (editingEntity)
	{

		EntitySaveData *d = dsq->game->getEntitySaveDataForEntity(editingEntity, oldPosition);
		if (d)
		{
			std::ostringstream os;
			os << "idx1: " << d->idx << " ";
			os << "idx2: " << editingEntity->entityTypeIdx << " ";
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

		if (state == ES_ROTATING)
		{
			if (!selectedElements.empty())
			{
				for (size_t i = 0; i < selectedElements.size(); i++)
				{
					selectedElements[i]->position = selectedElements[i]->getWorldPosition();
					selectedElements[i]->rotation = selectedElements[i]->getAbsoluteRotation();
					dummy.removeChild(selectedElements[i]);
				}
				core->removeRenderObject(&dummy, Core::DO_NOT_DESTROY_RENDER_OBJECT);
			}
		}
		else if (state == ES_SCALING)
		{

			if (!selectedElements.empty())
			{
				for (size_t i = 0; i < selectedElements.size(); i++)
				{
					selectedElements[i]->position = selectedElements[i]->getWorldPosition();
					selectedElements[i]->scale = selectedElements[i]->scale * dummy.scale;
					selectedElements[i]->rotation = selectedElements[i]->getAbsoluteRotation();
					dummy.removeChild(selectedElements[i]);
				}
				core->removeRenderObject(&dummy, Core::DO_NOT_DESTROY_RENDER_OBJECT);
			}
			else if (editingElement)
			{
				Vector add = editingElement->beforeScaleOffset;
				Vector newScale = editingElement->scale;
				editingElement->beforeScaleOffset = Vector(0,0,0);
				editingElement->scale = Vector(1,1,1);
				editingElement->position = editingElement->getWorldCollidePosition(add);
				editingElement->scale = newScale;
			}
		}
		checkForRebuild();
	}
	state = ES_SELECTING;

}


void SceneEditor::toggleElementSolid()
{
	if (editingElement)
	{
		switch(editingElement->elementFlag)
		{
		default:
		case EF_NONE:
		{
			std::ostringstream os;
			os << "elementFlag: " << editingElement->elementFlag;
			debugLog(os.str());
			debugLog("Solid");
			editingElement->elementFlag = EF_SOLID;
		}
		break;
		case EF_SOLID:
			debugLog("Solid2");
			editingElement->elementFlag = EF_SOLID2;
		break;
		case EF_SOLID2:
			debugLog("Solid3");
			editingElement->elementFlag = EF_SOLID3;
		break;
		case EF_SOLID3:
			debugLog("None");
			editingElement->elementFlag = EF_NONE;
		break;
		}
		dsq->game->reconstructGrid(true);
	}
}

void SceneEditor::toggleElementHurt()
{
	if (editingElement)
	{
		if (editingElement->elementFlag == EF_HURT)
			editingElement->elementFlag = EF_NONE;
		else
			editingElement->elementFlag = EF_HURT;
		dsq->game->reconstructGrid(true);
	}
}

void SceneEditor::toggleElementRepeat()
{
	if (editingElement)
	{
		editingElement->repeatTextureToFill(!editingElement->isRepeatingTextureToFill());
	}
}

void SceneEditor::mouseButtonLeft()
{
	if (multiSelecting || state != ES_SELECTING || !dummy.children.empty() || core->mouse.buttons.right) return;
	if (editType == ET_ELEMENTS)
	{
		if (selectedElements.empty() || editingElement)
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
	if (multiSelecting || state != ES_SELECTING || !dummy.children.empty() || core->mouse.buttons.left) return;
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
			Path *p = dsq->game->getPath(selectedIdx);
			editingPath = p;
			if (core->getShiftState())
			{
				enterScaleState();
			}
		}
	}
	if (editType == ET_ELEMENTS)
	{
		if (selectedElements.empty() || editingElement)
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

void SceneEditor::startMoveEntity()
{
	movingEntity = dsq->game->getEntityAtCursor();
}

void SceneEditor::endMoveEntity()
{
	if (movingEntity)
	{
		updateSaveFileEnemyPosition(movingEntity);
		movingEntity = 0;
	}
}

void SceneEditor::up()
{
	if (editType == ET_ELEMENTS && state == ES_SELECTING)
	{
		if (editingElement || !selectedElements.empty())
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
		if (editingElement || !selectedElements.empty())
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
	std::vector<Element*> deleteElements;
	size_t i = 0;
	for (i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		if (e->bgLayer==4 && e->templateIdx >= 1 && e->templateIdx <= 4)
		{
			e->safeKill();
			deleteElements.push_back(e);
		}
	}
	for (i = 0; i < deleteElements.size(); i++)
	{
		dsq->removeElement(deleteElements[i]);
	}
	deleteElements.clear();

	int idx=1;
	int idxCount = 0;
	for (int x = minX; x < maxX-2; x++)
	{
		for (int y = minY; y < maxY; y++)
		{
			Vector offset, pOffset;
			Vector wallNormal;
			float rot=0;
			bool addTile = false;
			TileVector t(x,y);
			if (dsq->game->isObstructed(t, OT_MASK_BLACK)
				&& (
				!dsq->game->isObstructed(TileVector(x+1,y), OT_MASK_BLACK) ||
				!dsq->game->isObstructed(TileVector(x-1,y), OT_MASK_BLACK) ||
				!dsq->game->isObstructed(TileVector(x,y-1), OT_MASK_BLACK) ||
				!dsq->game->isObstructed(TileVector(x,y+1), OT_MASK_BLACK)
				)
				)
			{
				float dist=0;
				wallNormal = dsq->game->getWallNormal(t.worldVector(), 5, &dist, OT_MASK_BLACK);
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
				Quad q;
				q.rotation.z = rot;
				p += Vector(TILE_SIZE/2, TILE_SIZE/2,0);
				offset.z = 0;

				bool skip = false;
				for (size_t i = 0; i < dsq->getNumElements(); i++)
				{
					Element *e = dsq->getElement(i);
					if (e->templateIdx <= 4 && e->templateIdx >= 1)
					{
						if ((p - e->position).getSquaredLength2D() < sqr(50))
						{
							skip = true;
							break;
						}
					}
				}

				if (!skip)
				{
					std::vector<int> cantUse;
					cantUse.resize(4);
					size_t i = 0;
					for (i = 0; i < dsq->getNumElements(); i++)
					{
						Element *e = dsq->getElement(i);
						if (e->templateIdx <= 4 && e->templateIdx >= 1)
						{
							if ((p - e->position).getSquaredLength2D() < sqr(120))
							{
								cantUse[e->templateIdx-1]++;
							}
						}
					}
					size_t useIdx = rand()%cantUse.size()+1;
					for (i = 0; i < cantUse.size(); i++)
					{
						size_t check = i + idxCount;
						if (check >= cantUse.size())
							check -= cantUse.size();
						if (cantUse[check]<=0)
						{
							useIdx = check+1;
						}
					}
					idxCount = rand()%cantUse.size();

					Element *e = dsq->game->createElement(useIdx, p, 4, &q);
					e->offset = offset;



					idx++;
					if(idx > 4)
						idx = 1;
				}
			}
		}
		for (size_t i = 0; i < dsq->getNumElements(); i++)
		{
			Element *e = dsq->getElement(i);
			if (e->bgLayer == 4 && e->templateIdx >= 1 && e->templateIdx <= 4)
			{
				e->position += e->offset;
				e->offset = Vector(0,0,0);
			}
		}
	}
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
		dsq->game->clearObsRows();

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
						dsq->game->addObsRow(xobs, y, x - xobs); // just changed from obs to not-obs, emit row
					}
				}

				c += img.channels;
			}
			if(isobs) // right side ends with obs, add final row
			{
				dsq->game->addObsRow(xobs, y, img.w - xobs);
				if (img.w > maxX)
					maxX = img.w;
				if (y > maxY)
					maxY = y;
			}
		}

		dsq->game->reconstructGrid(true);

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

void SceneEditor::removeEntity()
{
	debugLog("remove entity at cursor");
	dsq->game->removeEntityAtCursor();
}

void SceneEditor::placeAvatar()
{
	dsq->game->avatar->position = dsq->getGameCursorPosition();
	dsq->game->action(ACTION_PLACE_AVATAR, 0, -1, INPUT_NODEVICE);
}

void SceneEditor::scaleElementUp()
{
	placer->scale += Vector(0.25, 0.25,0);
}

void SceneEditor::scaleElementDown()
{
	placer->scale -= Vector(0.25, 0.25,0);
}

void SceneEditor::scaleElement1()
{
	placer->scale = Vector(1,1,1);
}

void SceneEditor::flipElementHorz()
{
	if (editType != ET_ELEMENTS)
		return;

	if (editingElement)
		editingElement->flipHorizontal();
	else
		this->placer->flipHorizontal();
}

void SceneEditor::flipElementVert()
{
	if (editType != ET_ELEMENTS)
		return;

	if (editingElement)
		editingElement->flipVertical();
	else
		this->placer->flipVertical();
}

void SceneEditor::moveElementToLayer(Element *e, int bgLayer)
{
	if (!selectedElements.empty())
	{
		for (size_t i = 0; i < selectedElements.size(); i++)
		{
			Element *e = selectedElements[i];
			core->removeRenderObject(e, Core::DO_NOT_DESTROY_RENDER_OBJECT);
			dsq->removeElement(e);
			e->bgLayer = bgLayer;
			dsq->addElement(e);
			core->addRenderObject(e, LR_ELEMENTS1+bgLayer);
		}
	}
	else if (e)
	{
		core->removeRenderObject(e, Core::DO_NOT_DESTROY_RENDER_OBJECT);
		dsq->removeElement(e);
		e->bgLayer = bgLayer;
		dsq->addElement(e);
		core->addRenderObject(e, LR_ELEMENTS1+bgLayer);
	}
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

		selectedElements.clear();

		for (size_t i = 0; i < dsq->getNumElements(); i++)
		{
			Element *e = dsq->getElement(i);
			if (e->bgLayer == bgLayer && e->position.x >= p1.x && e->position.y >= p1.y && e->position.x <= p2.x && e->position.y <= p2.y)
			{
				selectedElements.push_back(e);
			}
		}
	}
}

void SceneEditor::action(int id, int state, int source, InputDevice device)
{
	if (core->getCtrlState() && editingElement)
	{
		if (id == ACTION_BGLAYEREND)
		{
			editingElement->setElementEffectByIndex(-1);
		}
		else if (id >= ACTION_BGLAYER1 && id < ACTION_BGLAYER10)
		{
			editingElement->setElementEffectByIndex(id - ACTION_BGLAYER1);
		}
	}
	else if (editType == ET_ELEMENTS && state && id >= ACTION_BGLAYER1 && id < ACTION_BGLAYEREND)
	{
		int newLayer = id - ACTION_BGLAYER1;

		updateText();

		if (core->getAltState())
		{
			dsq->game->setElementLayerVisible(newLayer, !dsq->game->isElementLayerVisible(newLayer));
			return; // do not switch to the layer that was just hidden
		}
		else if (core->getShiftState() && (editingElement || !selectedElements.empty()))
		{
			moveElementToLayer(editingElement, newLayer);
		}
		else
		{
			selectedElements.clear();
		}

		this->bgLayer = newLayer;

	}

	if (id == ACTION_MULTISELECT && this->state == ES_SELECTING)
	{
		if (state)
		{
			if (!multiSelecting)
				selectedElements.clear();
			multiSelecting = true;
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

void SceneEditor::loadSceneByName()
{
	std::string s = dsq->getUserInputString("Enter Name of Map to Load");
	if (!s.empty())
		dsq->game->transitionToScene(s);
}

void SceneEditor::reloadScene()
{
	debugLog("reloadScene");
	dsq->game->positionToAvatar = dsq->game->avatar->position;
	dsq->game->transitionToScene(dsq->game->sceneName);
}


void SceneEditor::loadScene()
{
	if (core->getShiftState())
	{
		loadSceneByName();
	}
	else
	{
		reloadScene();
	}

	// HACK: reload stuff when (re-) loading a map this way
	particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);
	Shot::loadShotBank(dsq->shotBank1, dsq->shotBank2);
	dsq->game->loadEntityTypeList();
	dsq->loadElementEffects();
	dsq->continuity.loadSongBank();
	dsq->loadStringBank();
}

void SceneEditor::saveScene()
{
	if(dsq->game->saveScene(dsq->game->sceneName))
		dsq->screenMessage(dsq->game->sceneName + " Saved!");
	else
		dsq->screenMessage(dsq->game->sceneName + " FAILED to save!");
}

void SceneEditor::deleteSelectedElement()
{
	deleteElement(selectedIdx);
	selectedIdx = -1;
}

void SceneEditor::deleteElement(int selectedIdx)
{
	if (selectedIdx == -1) return;
	Element *e = dsq->getElement(selectedIdx);
	e->setLife(0.5);
	e->setDecayRate(1);
	e->fadeAlphaWithLife = true;
	dsq->removeElement(selectedIdx);
	dsq->game->reconstructGrid();
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
	dsq->game->addRenderObject(se_grad, LR_HUD);

	EntityGroup &group = game->entityGroups[game->sceneEditor.entityPageNum];

	for (size_t i = 0; i < group.entities.size(); i++)
	{
		EntityGroupEntity ent = group.entities[i];
		EntityClass *ec = dsq->game->getEntityClassForEntityType(ent.name);
		if (!ec && !dsq->mod.isActive())
		{
			errorLog("Entity Page List Error: Typo in Entities.txt?");
		}
		int type = -1;
		if (ec)
		{
			size_t j=0;
			for (j = 0; j < dsq->game->entityTypeList.size(); j++)
			{
				if (ec->idx == dsq->game->entityTypeList[j].idx)
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
			dsq->game->addRenderObject(q, LR_HUD);
			qs.push_back(q);
			c++;
		}
	}

	se_label = new DebugFont();
	se_label->setFontSize(10);
	se_label->setText(group.name);
	se_label->position = Vector(20,90);
	se_label->followCamera = 1;
	dsq->game->addRenderObject(se_label, LR_HUD);
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

Element* SceneEditor::cycleElementNext(Element *e1)
{
	size_t ce = e1->templateIdx;
	int idx=0;
	for (size_t i = 0; i < dsq->game->elementTemplates.size(); i++)
	{
		if (dsq->game->elementTemplates[i].idx == ce)
			idx = i;
	}
	ce = idx;
	ce++;
	if (ce >= dsq->game->elementTemplates.size())
		ce = 0;
	idx = dsq->game->elementTemplates[ce].idx;
	if (idx < 1024)
	{
		Element *e = dsq->game->createElement(idx, e1->position, e1->bgLayer, e1);
		e1->safeKill();
		dsq->removeElement(e1);
		return e;
	}
	return e1;
}

Element* SceneEditor::cycleElementPrev(Element *e1)
{
	size_t ce = e1->templateIdx;
	size_t idx=0;
	for (size_t i = 0; i < dsq->game->elementTemplates.size(); i++)
	{
		if (dsq->game->elementTemplates[i].idx == ce)
		{
			idx = i;
			break;
		}
	}
	ce = idx;
	ce--;
	if (ce == -1)
		ce = dsq->game->elementTemplates.size()-1;
	idx = dsq->game->elementTemplates[ce].idx;
	if (idx < 1024)
	{
		Element *e = dsq->game->createElement(idx, e1->position, e1->bgLayer, e1);
		e1->safeKill();
		dsq->removeElement(e1);
		return e;
	}
	return e1;
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
		dsq->game->reconstructGrid(true);
		return;
	}

	if (editType == ET_ELEMENTS)
	{
		if (dsq->game->elementTemplates.empty()) return;
		if (core->getCtrlState())
		{
			if (!selectedElements.empty())
			{
				for (size_t i = 0; i < selectedElements.size(); i++)
				{
					selectedElements[i]->rotation.z = 0;
				}
			}
			else if (editingElement)
			{
				editingElement->rotation.z = 0;
			}
		}
		else if (!selectedElements.empty())
		{
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				selectedElements[i] = cycleElementNext(selectedElements[i]);
			}
		}
		else if (editingElement)
		{
			cycleElementNext(editingElement);
			editingElement = 0;
		}
		else
		{
			int oldCur = curElement;
			curElement++;
			if (curElement >= dsq->game->elementTemplates.size())
				curElement = 0;

			if (dsq->game->elementTemplates[curElement].idx < 1024)
			{
				placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
			}
			else
			{
				curElement = oldCur;
			}
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
		if (dsq->game->elementTemplates.empty()) return;
		if (!selectedElements.empty())
		{
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				selectedElements[i] = cycleElementPrev(selectedElements[i]);
			}
		}
		else if (editingElement)
		{
			cycleElementPrev(editingElement);
			editingElement = 0;
		}
		else
		{
			doPrevElement();
		}
	}
}

void SceneEditor::doPrevElement()
{
	size_t oldCur = curElement;
	size_t maxn = dsq->game->elementTemplates.size();

	if(curElement)
		curElement--;
	else if(maxn)
		curElement = maxn-1;

	if (maxn && curElement >= maxn)
		curElement = maxn-1;

	if (maxn && dsq->game->elementTemplates[curElement].idx < 1024)
	{
		placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
	}
	else
	{
		curElement = oldCur;
	}
}

void SceneEditor::moveLayer()
{
	std::string s = dsq->getUserInputString("Enter 'fromLayer toLayer' (space inbetween, ESC/m-ty to cancel)");
	if (!s.empty())
	{
		std::istringstream is(s);
		int fromLayer, toLayer;
		is >> fromLayer >> toLayer;
		toLayer--;
		fromLayer--;
		for (size_t i = 0; i < dsq->getNumElements(); i++)
		{
			Element *e = dsq->getElement(i);
			if (e)
			{
				if (e->bgLayer == fromLayer)
				{
					moveElementToLayer(e, toLayer);
				}
			}
		}
	}
}

void SceneEditor::selectZero()
{
	if (state != ES_SELECTING) return;

	if (editType == ET_ELEMENTS)
	{
		if (dsq->game->elementTemplates.empty()) return;
		if (editingElement)
		{
		}
		else
		{
			curElement = 0;
			placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
		}
	}
}

void SceneEditor::selectEnd()
{
	if (state != ES_SELECTING) return;
	if (editType == ET_ELEMENTS)
	{
		if (dsq->game->elementTemplates.empty()) return;
		if (!editingElement)
		{
			size_t largest = 0;
			for (size_t i = 0; i < dsq->game->elementTemplates.size(); i++)
			{
				ElementTemplate et = dsq->game->elementTemplates[i];
				if (et.idx < 1024 && i > largest)
				{
					largest = i;
				}
			}
			curElement = largest;
			placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
		}
	}
}

void SceneEditor::placeElement()
{
	if (editType == ET_ELEMENTS)
	{
		if (!core->getShiftState() && !core->getKeyState(KEY_LALT))
		{
			dsq->game->createElement(dsq->game->elementTemplates[curElement].idx, placer->position, bgLayer, placer);
			updateText();
			dsq->game->reconstructGrid();
		}
	}
	else if (editType == ET_ENTITIES)
	{
		if (!selectedEntity.nameBased)
			dsq->game->createEntity(selectedEntity.index, 0, dsq->getGameCursorPosition(), 0, true, "", ET_ENEMY, true);
		else
			dsq->game->createEntity(selectedEntity.name, 0, dsq->getGameCursorPosition(), 0, true, "", ET_ENEMY, true);
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
			dsq->game->addPath(p);
			selectedIdx = dsq->game->getNumPaths()-1;
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
		if (!selectedElements.empty())
		{
			std::vector<Element*>copy;
			Vector groupCenter = this->getSelectedElementsCenter();
			for (size_t i = 0; i < selectedElements.size(); i++)
			{
				Element *e1 = selectedElements[i];
				Vector dist = e1->position - groupCenter;
				Element *e = dsq->game->createElement(e1->templateIdx, placer->position + Vector(40,40) + dist, e1->bgLayer, e1);
				e->elementFlag = e1->elementFlag;
				e->setElementEffectByIndex(e1->getElementEffectIndex());
				copy.push_back(e);
			}
			selectedElements.clear();
			selectedElements = copy;
			copy.clear();
		}
		else if (editingElement)
		{
			Element *e1 = editingElement;
			Element *e = dsq->game->createElement(e1->templateIdx, placer->position + Vector(40,40), e1->bgLayer, e1);
			e->elementFlag = e1->elementFlag;
			e->setElementEffectByIndex(e1->getElementEffectIndex());

		}
		dsq->game->reconstructGrid();
	}
	else if (editType == ET_ENTITIES)
	{

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

			dsq->game->addPath(newp);
			selectedIdx = dsq->game->getNumPaths()-1;
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
	if (dsq->game->isInGameMenu()) return;
	if (!on && editType == ET_SELECTENTITY) return;
	if (dsq->game->worldMapRender && dsq->game->worldMapRender->isOn()) return;
	this->on = on;
	autoSaveTimer = 0;

	execID = -1;
	if (on)
	{
		btnMenu->alpha = 1;
		dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = true;

		dsq->game->togglePause(on);
		if (dsq->game->avatar)
			dsq->game->avatar->disableInput();
		text->alpha.interpolateTo(1, 0.5);
		placer->alpha.interpolateTo(0.5, 0.5);
		movingEntity = 0;
		dsq->toggleCursor(true);
		dsq->setCursor(CURSOR_NORMAL);


		dsq->darkLayer.toggle(false);

		dsq->game->rebuildElementUpdateList();

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

		selectedElements.clear();
		for (int i = 0; i < 9; i++)
			dsq->getRenderObjectLayer(LR_ELEMENTS1+i)->visible = true;

		if (movingEntity)
			endMoveEntity();
		movingEntity = 0;

		dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = false;

		dsq->game->togglePause(on);
		if (dsq->game->avatar)
			dsq->game->avatar->enableInput();
		text->alpha.interpolateTo(0, 0.2f);
		placer->alpha.interpolateTo(0, 0.2f);

		dsq->darkLayer.toggle(true);

		dsq->game->rebuildElementUpdateList();

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
	const Vector cursor = dsq->getGameCursorPosition();
	TileVector tv(cursor);

	std::ostringstream os;
	os << dsq->game->sceneName << " bgL[" << bgLayer << "] (" <<
		(int)dsq->cameraPos.x << "," << (int)dsq->cameraPos.y << ") ("
		<< (int)cursor.x << "," << (int)cursor.y << ") T("
		<< tv.x << "," << tv.y << ") ";
	switch(editType)
	{
	case ET_ELEMENTS:
		os << "elements (" << dsq->getNumElements() << ")";
		if (selectedElements.size() > 1)
		{
			os << " - " << selectedElements.size() << " selected";
		}
		else
		{
			Element *e;
			if (!selectedElements.empty())
				e = selectedElements[0];
			else
				e = editingElement;
			if (e)
			{
				os << " id: " << e->templateIdx;
				ElementTemplate *et = game->getElementTemplateByIdx(e->templateIdx);
				if (et)
					os << " gfx: " << et->gfx;
				os << " efx: " << (e->getElementEffectIndex() + 1); // +1 so that it resembles the layout on numpad
			}
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
				<< " flag:" << dsq->continuity.getEntityFlag(dsq->game->sceneName, editingEntity->getID())
				<< " fh:" << editingEntity->isfh()
				<< " fv:" << editingEntity->isfv()
				<< " state:" << editingEntity->getState()
				<< " et:" << editingEntity->getEntityType()
				<< " hp:" << editingEntity->health << "/" << editingEntity->maxHealth;
		}
	break;
	case ET_PATHS:
		os << "paths (" << dsq->game->getNumPaths() << ")";
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

Vector SceneEditor::getSelectedElementsCenter()
{
	Vector c;
	for (size_t i = 0; i < selectedElements.size(); i++)
	{
		c += selectedElements[i]->position;
	}
	c /= float(selectedElements.size());
	return c;
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
			os << "auto/AUTO_" << autoSaveFile << "_" << dsq->game->sceneName;
			if(dsq->game->saveScene(os.str()))
			{
				std::string m = "Map AutoSaved to " + os.str();
				dsq->debugLog(m);
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
			editingEntity = 0;
			if (isActing(ACTION_MULTISELECT, -1) || !selectedElements.empty())
			{
				editingElement = 0;
			}
			if (state == ES_SELECTING && !isActing(ACTION_MULTISELECT, -1))
				editingElement = this->getElementAtCursor();

			if (editingElement)
				placer->alpha = 0;
			else
				placer->alpha = 0.5;
		break;
		case ET_ENTITIES:
			editingElement = 0;
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
				for (size_t i = 0; i < dsq->game->getNumPaths(); i++)
				{
					if(dsq->game->getPath(i)->nodes.size() == 0) {
						continue;
					}
					for (size_t n = dsq->game->getPath(i)->nodes.size(); n-- > 0; )
					{
						Vector v = dsq->game->getPath(i)->nodes[n].position - dsq->getGameCursorPosition();
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
					Vector add = Vector((dsq->getGameCursorPosition().x - cursorOffset.x)*factor,
						(dsq->getGameCursorPosition().y - cursorOffset.y)*factor);
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
				dsq->game->getPath(selectedIdx)->nodes[selectedNode].position = dsq->getGameCursorPosition() + cursorOffset;
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
						int a = (oldRotation.z + add)/45;
						add = a * 45;
						editingEntity->rotation.z = add;
					}
					else
					{
						editingEntity->rotation.z = oldRotation.z + add;
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
				float closest = sqr(800);
				size_t i = 0;
				for (i = 0; i < dsq->getNumElements(); i++)
				{
					Vector dist = dsq->getElement(i)->getFollowCameraPosition() - dsq->getGameCursorPosition();
					float len = dist.getSquaredLength2D();
					if (len < closest)
					{
						closest = len;
					}
				}
			}
			break;
			case ES_MOVING:
				updateSelectedElementPosition(dsq->getGameCursorPosition() - cursorOffset);
			break;
			case ES_ROTATING:
			{
				if (!selectedElements.empty())
				{

					float add = (dsq->getGameCursorPosition().x - cursorOffset.x)/2.4f;
					if (core->getCtrlState())
					{
						int a = (oldRotation.z + add)/45;
						add = a * 45;
						dummy.rotation.z = add;
					}
					else
					{
						dummy.rotation.z = oldRotation.z + add;
					}
				}
				else if (editingElement)
				{
					float add = (dsq->getGameCursorPosition().x - cursorOffset.x)/2.4f;
					if (core->getCtrlState())
					{
						int a = (oldRotation.z + add)/45;
						add = a * 45;
						editingElement->rotation.z = add;
					}
					else
					{
						editingElement->rotation.z = oldRotation.z + add;
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
					if (!selectedElements.empty())
						add.y = add.x;
					else
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
							add *= 0.1f;
					}
				}
				if (!selectedElements.empty())
				{
					if (core->getCtrlState())
					{
						dummy.scale = Vector(1,1);
					}
					else
					{
						dummy.scale=oldScale + add;
						if (dummy.scale.x < MIN_SIZE)
							dummy.scale.x  = MIN_SIZE;
						if (dummy.scale.y < MIN_SIZE)
							dummy.scale.y  = MIN_SIZE;
					}

					for (size_t i = 0; i < selectedElements.size(); i++)
						selectedElements[i]->refreshRepeatTextureToFill();
				}
				else if (editingElement)
				{
					Vector& editVec = repeatScale ? editingElement->repeatToFillScale : editingElement->scale;
					if (core->getCtrlState())
					{
						editVec = Vector(1,1);
					}
					else
					{

						editVec = (repeatScale ? oldRepeatScale : oldScale) + add;
						if (!uni && !repeatScale)
						{
							if (!middle)
							{
								Vector offsetChange = (add*Vector(editingElement->getWidth(), editingElement->getHeight()))*0.5f;
								if (add.y == 0)
								{
									if (right)
										editingElement->beforeScaleOffset = offsetChange;
									else
										editingElement->beforeScaleOffset = -offsetChange;
								}
								else
								{
									if (down)
										editingElement->beforeScaleOffset = offsetChange;
									else
										editingElement->beforeScaleOffset = -offsetChange;
								}
							}
						}
						if (editVec.x < MIN_SIZE)
							editVec.x  = MIN_SIZE;
						if (editVec.y < MIN_SIZE)
							editVec.y  = MIN_SIZE;
					}

					editingElement->refreshRepeatTextureToFill();
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
		if (editingElement || !selectedElements.empty())
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
		if (editingElement || !selectedElements.empty())
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
	for(tv.y = MAX_GRID - 1; ; --tv.y)
	{
		for(tv.x = 0; tv.x < MAX_GRID; ++tv.x)
			*ptr++ = game->isObstructed(tv, OT_MASK_BLACK) ? 0xFF000000 : 0xFFFFFFFF;
		if(tv.y == 0)
			break;
	}
	std::string outfn = dsq->getUserDataFolder() + "/griddump-" + game->sceneName + ".tga";
	tgaSaveRGBA(outfn.c_str(), MAX_GRID, MAX_GRID, data);
	dsq->screenMessage("Saved grid image to " + outfn);
}

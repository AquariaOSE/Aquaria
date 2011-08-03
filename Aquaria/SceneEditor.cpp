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
#include "../BBGE/MathFunctions.h"
#include "../ExternalLibs/glpng.h"
#include "../BBGE/Gradient.h"
#include "../BBGE/DebugFont.h"

#include "Game.h"
#include "DSQ.h"
#include "Avatar.h"
#include "GridRender.h"


#ifdef AQUARIA_BUILD_SCENEEDITOR  // Through end of file


#ifdef BBGE_BUILD_WINDOWS
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

void WarpAreaRender::onRender()
{
#ifdef BBGE_BUILD_OPENGL
	for (int i = 0; i < dsq->game->warpAreas.size(); i++)
	{
		WarpArea *a = &dsq->game->warpAreas[i];
		glTranslatef(a->position.x, a->position.y,0);

		if (a->warpAreaType == "Brown")
			glColor4f(0.5, 0.25, 0, alpha.getValue());
		else
		{
			switch (a->warpAreaType[0])
			{
			case 'B':
				glColor4f(0,0,1,alpha.getValue());
			break;
			case 'R':
				glColor4f(1,0,0,alpha.getValue());
			break;
			case 'G':
				glColor4f(0, 1, 0,alpha.getValue());
			break;
			case 'Y':
				glColor4f(1,1,0,alpha.getValue());
			break;
			case 'P':
				glColor4f(1,0,1,alpha.getValue());
			break;
			case 'O':
				glColor4f(1,0.5,0,alpha.getValue());
			break;
			}
		}

		if (a->radius)
			drawCircle(a->radius);
		else
		{
			glBegin(GL_QUADS);
			{
				glVertex2f(-a->w,-a->h);
				glVertex2f(-a->w,a->h);
				glVertex2f(a->w,a->h);
				glVertex2f(a->w,-a->h);
			}
			glEnd();
		}
		glTranslatef(-a->position.x, -a->position.y,0);
	}
#endif
#ifdef BBGE_BUILD_DIRECTX
	for (int i = 0; i < dsq->game->warpAreas.size(); i++)
	{
		WarpArea *a = &dsq->game->warpAreas[i];
		core->translateMatrixStack(a->position.x, a->position.y);
		switch (a->warpAreaType[0])
		{
		case 'B':
			core->setColor(0, 0, 1, alpha.x);
		break;
		case 'R':
			core->setColor(1,0,0,alpha.x);
		break;
		case 'G':
			core->setColor(0,1,0,alpha.x);
		break;
		case 'Y':
			core->setColor(1,1,0,alpha.x);
		break;
		case 'P':
			core->setColor(1,0,1,alpha.x);
		break;
		}
		if (a->radius)
		{
		//	drawCircle(a->radius);
		}
		else
		{
			core->applyMatrixStackToWorld();
			core->blitD3D(0, a->w*2, a->h*2);
			/*
			glBegin(GL_QUADS);
			{
				glVertex2f(-a->w,-a->h);
				glVertex2f(-a->w,a->h);
				glVertex2f(a->w,a->h);
				glVertex2f(a->w,-a->h);
			}
			glEnd();
			*/
		}
		core->translateMatrixStack(-a->position.x, -a->position.y);
		//glTranslatef(-a->position.x, -a->position.y,0);
	}
#endif
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
		//editingElement->parallax = 0.9;
		//editingElement->followCamera = 0.001;
		editingElement->followCamera = 0.9;
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
		glow->setBlendType(RenderObject::BLEND_ADD);
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
		//if ()

		if (dsq->game->sceneEditor.selectedEntity.name == selectedEntity.name
			|| (entType != -1 && dsq->game->sceneEditor.selectedEntity.typeListIndex == entType))
		{
			glow->alpha.interpolateTo(0.2, 0.2);
		}
		else
		{
			glow->alpha.interpolateTo(0, 0.1);
		}
		if ((core->mouse.position - position).isLength2DIn(32))
		{
			label->alpha.interpolateTo(1, 0.1);
			alpha.interpolateTo(1, 0.2);
			scale.interpolateTo(Vector(1.5, 1.5), 0.3);

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
				//se_changedEntityType = true;
				if (doubleClickTimer > 0)
				{
					doubleClickTimer = 0;
					if (!dsq->isFullscreen())
					{
						std::string fn = "scripts/entities/" + entName + ".lua";
						#ifdef BBGE_BUILD_WINDOWS
						debugLog("SHELL EXECUTE!");
						ShellExecute(dsq->hWnd, "open", fn.c_str(), NULL, NULL, SW_SHOWNORMAL);
						#endif
					}
				}
				else
				{
					doubleClickTimer = 0.4;
				}
			}
		}
		else
		{
			label->alpha.interpolateTo(0, 0.1);
			alpha.interpolateTo(0.7, 0.2);
			scale.interpolateTo(Vector(1.0, 1.0), 0.3);
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
	/*
	core->clearDebugMenu();
	core->addDebugMenuItem("", bid);
	core->doModalDebugMenu();
	*/

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
		core->main(FRAME_TIME);
	}

	addMainMenuItem("LOAD LEVEL...                    (SHIFT-F1)",			100);
	addMainMenuItem("RELOAD LEVEL                           (F1)",			101);
	addMainMenuItem("SAVE LEVEL                             (F2)",			102);
	addMainMenuItem("EDIT TILES                             (F5)",			106);
	addMainMenuItem("EDIT ENTITIES                          (F6)",			107);
	addMainMenuItem("EDIT NODES                             (F7)",			108);
	addMainMenuItem("REGEN COLLISIONS                  (SHIFT-R)",		    103);
	addMainMenuItem("RECACHE TEXTURES					(CTRL-R)",			130);
//	addMainMenuItem("REFRESH DATAFILES					   (F11)",			117);
	addMainMenuItem("REGEN ROCK FROM MAPTEMPLATE       (F11+F12)",			116);
	/*
	addMainMenuItem("RE-TEMPLATE                           (F11)",			104);
	addMainMenuItem("RE-SKIN                               (F12)",			105);
	*/
	addMainMenuItem("SET BG GRADIENT",										110);
	addMainMenuItem("SET MUSIC",											111);
	addMainMenuItem("ENTITY GROUPS                      (CTRL-E)",			112);
	if (dsq->game->gridRender)
		addMainMenuItem(std::string("TOGGLE TILE COLLISION RENDER ") + ((dsq->game->gridRender->alpha!=0) ? "OFF" : "ON ") + std::string("       (F9)"),			113);
	addMainMenuItem("SCREENSHOT                                 ",	        114);



	addMainMenuItem("PARTICLE VIEWER                            ",	        120);
	addMainMenuItem("ANIMATION EDITOR                           ",	        115);

	while (1 && !core->getKeyState(KEY_TAB))
	{
		core->main(FRAME_TIME);
		if (execID != -1)
			break;
	}

	closeMainMenu();

	executeButtonID(execID);
}

void SceneEditor::closeMainMenu()
{
	inMainMenu = false;
	for (int i = 0; i < mainMenu.size(); i++)
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
	selectionType = ST_SINGLE;
	editingElement = 0;
	editingEntity = 0;
	pathRender = new PathRender();
	core->getTopStateData()->addRenderObject(pathRender, LR_DEBUG_TEXT);
	pathRender->alpha = 0;

	selectedType = -1;
	possibleSelectedType = -1;

	target = new Quad;
	//target->setTexture("target");
	core->getTopStateData()->addRenderObject(target, LR_HUD);
	target->alpha = 0;
	editType = ET_ELEMENTS;
	state = ES_SELECTING;
	drawingWarpArea = 'N';

	btnMenu = new DebugButton(200, &menuReceiver, 700);
	btnMenu->position = Vector(20, 20);
	btnMenu->label->setText("Menu");
	btnMenu->followCamera = 1;
	btnMenu->alpha = 0;
	//btnMenu->event.set(MakeFunctionEvent(SceneEditor, openMainMenu));
	dsq->game->addRenderObject(btnMenu, LR_HUD);

	selectedEntityType = 0;
	zoom = Vector(0.2,0.2);
	drawingBox = false;
	bgLayer = 5;
	text = new DebugFont();
	text->setFontSize(6);

	text->followCamera = 1;
	text->position = Vector(200,20,4.5);
	//text->setAlign(ALIGN_CENTER);
	dsq->game->addRenderObject(text, LR_HUD);
	text->alpha = 0;
	selectedVariation = -1;

	boxPromo = new Quad;
	boxPromo->color = 0;
	boxPromo->alpha =0;
	boxPromo->cull = false;
	dsq->game->addRenderObject(boxPromo, LR_HUD);
	on = false;

	//addAction(MakeFunctionEvent(SceneEditor, addSpringPlant), KEY_K, 0);

	addAction(MakeFunctionEvent(SceneEditor, loadScene), KEY_F1, 0);
	addAction(MakeFunctionEvent(SceneEditor, saveScene), KEY_F2, 0);

	// removed in fc3
	//addAction(MakeFunctionEvent(SceneEditor, setGroup), KEY_G, 0);

	addAction(MakeFunctionEvent(SceneEditor, moveToBack), KEY_Z, 0);
	addAction(MakeFunctionEvent(SceneEditor, moveToFront), KEY_X, 0);

	addAction(MakeFunctionEvent(SceneEditor, toggleWarpAreaRender), KEY_F4, 0);

	addAction(MakeFunctionEvent(SceneEditor, editModeElements), KEY_F5, 0);
	addAction(MakeFunctionEvent(SceneEditor, editModeEntities), KEY_F6, 0);
	addAction(MakeFunctionEvent(SceneEditor, editModePaths), KEY_F7, 0);

	addAction(MakeFunctionEvent(SceneEditor, mouseButtonLeft), MOUSE_BUTTON_LEFT, 1);
	addAction(MakeFunctionEvent(SceneEditor, mouseButtonRight), MOUSE_BUTTON_RIGHT, 1);
	addAction(MakeFunctionEvent(SceneEditor, mouseButtonLeftUp), MOUSE_BUTTON_LEFT, 0);
	addAction(MakeFunctionEvent(SceneEditor, mouseButtonRightUp), MOUSE_BUTTON_RIGHT, 0);

	// removed in fc3
	//addAction(MakeFunctionEvent(SceneEditor, bindNodeToEntity), KEY_B, 0);

	addAction(MakeFunctionEvent(SceneEditor, alignHorz), KEY_C, 1);
	addAction(MakeFunctionEvent(SceneEditor, alignVert), KEY_V, 1);



/*
	addAction(MakeFunctionEvent(SceneEditor, placeEntity), KEY_U, 0);
	addAction(MakeFunctionEvent(SceneEditor, removeEntity), KEY_I, 0);
	*/
	//addAction(MakeFunctionEvent(SceneEditor, changeDepth), KEY_N, 0);

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

	//addAction(MakeFunctionEvent(SceneEditor, regenLevel), KEY_F12, 0);

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


	/*
	// OLD CRAP
	addAction(MakeFunctionEvent(SceneEditor, rotateElement), KEY_MULTIPLY, 0);
	addAction(MakeFunctionEvent(SceneEditor, rotateElement2), KEY_DIVIDE, 0);
	addAction(MakeFunctionEvent(SceneEditor, scaleElementUp), KEY_NUMPAD7, 0);
	addAction(MakeFunctionEvent(SceneEditor, scaleElementDown), KEY_NUMPAD1, 0);
	addAction(MakeFunctionEvent(SceneEditor, scaleElement1), KEY_NUMPAD0, 0);
	addAction(MakeFunctionEvent(SceneEditor, nextVariation), KEY_UP, 0);
	addAction(MakeFunctionEvent(SceneEditor, prevVariation), KEY_DOWN, 0);
	*/


	addAction(ACTION_ZOOMIN,		KEY_PGUP);
	addAction(ACTION_ZOOMOUT,		KEY_PGDN);

	addAction(ACTION_CAMLEFT,		KEY_A);
	addAction(ACTION_CAMRIGHT,		KEY_D);
	addAction(ACTION_CAMUP,			KEY_W);
	addAction(ACTION_CAMDOWN,		KEY_S);

	addAction(ACTION_BGLAYEREND,	KEY_0);

	addAction(ACTION_BGLAYER1,		KEY_1);
	addAction(ACTION_BGLAYER2,		KEY_2);
	addAction(ACTION_BGLAYER3,		KEY_3);
	addAction(ACTION_BGLAYER4,		KEY_4);
	addAction(ACTION_BGLAYER5,		KEY_5);
	addAction(ACTION_BGLAYER6,		KEY_6);
	addAction(ACTION_BGLAYER7,		KEY_7);
	addAction(ACTION_BGLAYER8,		KEY_8);
	addAction(ACTION_BGLAYER9,		KEY_9);

	addAction(ACTION_BGLAYER10,		KEY_B);
	addAction(ACTION_BGLAYER11,		KEY_N);
	addAction(ACTION_BGLAYER12,		KEY_M);

	addAction(ACTION_BGLAYER13,		KEY_J);

	addAction(ACTION_BGLAYER14,		KEY_COMMA);
	addAction(ACTION_BGLAYER15,		KEY_PERIOD);
	addAction(ACTION_BGLAYER16,		KEY_SLASH);

	addAction(ACTION_MULTISELECT,	KEY_LALT);

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

	warpAreaRender = new WarpAreaRender;
	warpAreaRender->alpha = 0;
	warpAreaRender->toggleCull(false);
	dsq->game->addRenderObject(warpAreaRender, LR_HUD);

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
	//if (dsq->mod.isActive())	return;
	static bool inCreateAqurian = false;
	if (inCreateAqurian) return;
	//if (dsq->game->isPaused()) return;
	inCreateAqurian = true;
	std::string t = dsq->getUserInputString("Enter Aquarian:", "");
	stringToUpper(t);
	Vector startPos = dsq->getGameCursorPosition();
	for (int i = 0; i < t.size(); i++)
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
		//ElementTemplate et = dsq->game->getElementTemplateForLetter(v);
		dsq->game->createElement(v, startPos + Vector(64*i,0), this->bgLayer);
	}
	inCreateAqurian = false;
}

void SceneEditor::bindNodeToEntity()
{
	if (editType == ET_PATHS)
	{
		Path *p = getSelectedPath();
		if (p)
		{
			std::istringstream is(dsq->getUserInputString("Enter group number"));
			int group = 0;
			is >> group;
			Entity *e = getEntityAtCursor();
			if (e)
			{
				e->removeNodeFromAllNodeGroups(p);
				e->addNodeToNodeGroup(group, p);
			}
			else
			{
				debugLog("no entity at cursor");
			}
		}
	}
}

void SceneEditor::addSpringPlant()
{
	/*
	SpringPlant *s = new SpringPlant(dsq->getGameCursorPosition());
	dsq->game->addRenderObject(s, LR_ENTITIES);
	*/
}

Path *SceneEditor::getSelectedPath()
{
	if (selectedIdx >= 0 && selectedIdx < dsq->game->getNumPaths())
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
			p->name = dsq->getUserInputString("PathName", p->name);
			p->refreshScript();
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

void SceneEditor::toggleWarpAreaRender()
{
	if (warpAreaRender->alpha.x == 0)
		warpAreaRender->alpha.x = 0.5;
	else if (warpAreaRender->alpha.x >= 0.5f)
		warpAreaRender->alpha.x = 0;
		//warpAreaRender->alpha.interpolateTo(1, 0.2);
}

void SceneEditor::setGridPattern0()
{	if (editingElement)	editingElement->setElementEffectByIndex(-1); }

void SceneEditor::setGridPattern1()
{	if (editingElement)	editingElement->setElementEffectByIndex(0);	}

void SceneEditor::setGridPattern2()
{	if (editingElement)	editingElement->setElementEffectByIndex(1);	}

void SceneEditor::setGridPattern3()
{	if (editingElement)	editingElement->setElementEffectByIndex(2);	}

void SceneEditor::setGridPattern4()
{	if (editingElement)	editingElement->setElementEffectByIndex(3);	}

void SceneEditor::setGridPattern5()
{	if (editingElement)	editingElement->setElementEffectByIndex(4);	}

void SceneEditor::setGridPattern6()
{	if (editingElement)	editingElement->setElementEffectByIndex(5);	}

void SceneEditor::setGridPattern7()
{	if (editingElement)	editingElement->setElementEffectByIndex(6);	}

void SceneEditor::setGridPattern8()
{	if (editingElement)	editingElement->setElementEffectByIndex(7);	}

void SceneEditor::setGridPattern9()
{	if (editingElement)	editingElement->setElementEffectByIndex(8);	}

void SceneEditor::moveToFront()
{
	if (editingElement)
	{
		std::vector<Element*> copy = dsq->getElementsCopy();
		dsq->clearElements();

		//  move to the foreground ... this means that the editing element should be last in the list (Added last)
		for (int i = 0; i < copy.size(); i++)
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
	if (editingElement)
	{
		std::vector<Element*> copy = dsq->getElementsCopy();
		dsq->clearElements();

		//  move to the background ... this means that the editing element should be first in the list (Added first)
		dsq->addElement(editingElement);
		for (int i = 0; i < copy.size(); i++)
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
	target->alpha.interpolateTo(0, 0.5);
	editType = ET_ELEMENTS;
	if (curElement < dsq->game->elementTemplates.size())
	{
		placer->setTexture(dsq->game->elementTemplates[curElement].gfx);
		placer->scale = Vector(1,1);
	}
	placer->alpha = 0.5;
	pathRender->alpha = 0;
}

void SceneEditor::editModeEntities()
{
	selectedIdx = -1;
	//HACK: methinks target is useless now
	//target->alpha.interpolateTo(0, 0.5);
	editType = ET_ENTITIES;

	//dsq->game->entityTypeList[curEntity].prevGfx
	placer->setTexture(selectedEntity.prevGfx);
	placer->alpha = 0.5;
	pathRender->alpha = 0;
}

void SceneEditor::editModePaths()
{
	selectedIdx = -1;
	target->alpha.interpolateTo(0, 0.5);
	editType = ET_PATHS;
	placer->alpha = 0;
	pathRender->alpha = 0.5;
}

Element *SceneEditor::getElementAtCursor()
{
	int minDist = -1;
	Element *selected = 0;
	for (Element *e = dsq->getFirstElementOnLayer(this->bgLayer); e; e = e->bgLayerNext)
	{
		if (e->life == 1)
		{
			if (e->isCoordinateInside(dsq->getGameCursorPosition()))//, minSelectionSize
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
			for (int i = 0; i < selectedElements.size(); i++)
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
		/*
		RenderObject *r = getSelectedRenderObject();
		if (r)
		{
			if (dynamic_cast<Element*>(r))
			{
				deleteSelectedElement();
			}
			Entity *ent = 0;
			if (ent=dynamic_cast<Entity*>(r))
			{
				dsq->game->removeEntity(ent);
				//removeEntity();
			}
			selectedIdx = -1;
		}
		*/
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
				/*
				if (p->nodes.size() > 1)
					p->nodes.resize(p->nodes.size()-1);
				*/
				//selectedIdx = -1;
			}
		}
	}
}

void SceneEditor::updateSaveFileEnemyPosition(Entity *ent)
{
	TiXmlElement *exml = dsq->game->saveFile->FirstChildElement("Enemy");
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

RenderObject *SceneEditor::getSelectedRenderObject()
{
	if (editType == ET_ELEMENTS)
	{
		if (selectedIdx > -1)
		{
			if (selectedType == 0)
				return dsq->getElement(selectedIdx);
			/*
			else if (selectedType == 1)
				return dsq->entities[selectedIdx];
			*/
		}
	}
	return 0;
}

void SceneEditor::checkForRebuild()
{
	if (editType == ET_ELEMENTS && state != ES_SELECTING && !selectedElements.empty())
	{
		bool rebuild = false;
		for (int i = 0; i < selectedElements.size(); i++)
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
		for (int i = 0; i < selectedElements.size(); i++)
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
	/*
	if (editType == ET_ELEMENTS && state == ES_SELECTING && selectedIdx > -1)
	{
		state = ES_MOVING;
		oldPosition = getSelectedRenderObject()->position;
		cursorOffset = getSelectedRenderObject()->position - dsq->getGameCursorPosition();
	}
	*/
	if (state != ES_SELECTING) return;
	state = ES_MOVING;
	if (editType == ET_ELEMENTS)
	{
		if (!selectedElements.empty())
		{
			dummy.rotation = Vector(0,0,0);
			cursorOffset = dsq->getGameCursorPosition();
			groupCenter = getSelectedElementsCenter();
			for (int i = 0; i < selectedElements.size(); i++)
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
	/*
	if (editType == ET_ELEMENTS && state == ES_SELECTING && selectedIdx > -1)
	{
		state = ES_ROTATING;
		oldRotation = getSelectedRenderObject()->rotation;
		//cursorOffset = getSelectedRenderObject()->position - dsq->getGameCursorPosition();
		//cursorOffset = Vector(0,0);
		cursorOffset = dsq->getGameCursorPosition();
	}
	*/
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
			for (int i = 0; i < selectedElements.size(); i++)
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
			//cursorOffset = getSelectedRenderObject()->position - dsq->getGameCursorPosition();
			//cursorOffset = Vector(0,0);
			cursorOffset = dsq->getGameCursorPosition();
		}
	}
}

void SceneEditor::enterScaleState()
{
	/*
	if (editType == ET_ELEMENTS && state == ES_SELECTING && selectedIdx > -1)
	{
		state = ES_SCALING;
		oldScale = getSelectedRenderObject()->scale;
		//cursorOffset = getSelectedRenderObject()->position - dsq->getGameCursorPosition();
		cursorOffset = dsq->getGameCursorPosition();
	}
	*/
	if (state != ES_SELECTING) return;
	if (editType == ET_ELEMENTS)
	{
		if (!selectedElements.empty())
		{
			state = ES_SCALING;
			dummy.rotation = Vector(0,0,0);
			dummy.scale = Vector(1,1,1);
			oldScale = dummy.scale;
			cursorOffset = dsq->getGameCursorPosition();
			groupCenter = getSelectedElementsCenter();
			for (int i = 0; i < selectedElements.size(); i++)
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
		/*
		std::ostringstream os;
		os << "oldPos (" << oldPosition.x << ", " << oldPosition.y << ")";
		debugLog(os.str());
		*/
		EntitySaveData *d = dsq->game->getEntitySaveDataForEntity(editingEntity, oldPosition);
		if (d)
		{
			std::ostringstream os;
			os << "idx1: " << d->idx << " ";
			os << "idx2: " << editingEntity->entityTypeIdx << " ";
			os << "name: " << editingEntity->name;
			//os << "state: " << editingEntity->getState();
			os << "groupID: " << editingEntity->getGroupID();
			debugLog(os.str());
			//debugLog("changing entity save data");
			d->x = editingEntity->position.x;
			d->y = editingEntity->position.y;
			editingEntity->startPos = Vector(d->x, d->y);
			/*
			std::ostringstream os2;
			os2 << "setting savedata rot to: " << d->rot;
			debugLog(os2.str());
			*/
			d->rot = editingEntity->rotation.z;
			d->group = editingEntity->getGroupID();
		}
		else
		{
			//debugLog("didn't get entity save data");
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
				for (int i = 0; i < selectedElements.size(); i++)
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
				for (int i = 0; i < selectedElements.size(); i++)
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
	//dsq->game->reconstructGrid();
}

/*
void SceneEditor::toggleElementFlag1()
{
	if (editingElement)
	{
		editingElement->setFlag("");
	}
}

void SceneEditor::toggleElementFlag2()
{
}

void SceneEditor::toggleElementFlag3()
{
}

void SceneEditor::toggleElementFlag4()
{
}

void SceneEditor::toggleElementFlag5()
{
}
*/

void SceneEditor::toggleElementSolid()
{
	if (editingElement)
	{
		/*
		TileVector t(editingElement->position);
		editingElement->position = t.worldVector();
		*/

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

void SceneEditor::setGroup()
{
	if (editingEntity)
	{
		std::ostringstream os;
		os << editingEntity->getGroupID();
		Entity *backup = editingEntity;
		std::string value = dsq->getUserInputString("Enter Group", os.str());
		int group = 0;
		if (!value.empty())
		{
			std::istringstream is(value);
			is >> group;
		}
		backup->setGroupID(group);
		updateEntitySaveData(backup);
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
		/*
		if (!selectedElements.empty())
		{
			enterMoveState();
		}
		*/
		/*
		if (state == ES_MOVING || state == ES_ROTATING || state == ES_SCALING)
		{
			if (selectedIdx != -1)
			{
				if (selectedType == 1)
				{
					RenderObject *r = getSelectedRenderObject();
					Entity *e = dynamic_cast<Entity*>(r);
					if (e)
					{
						updateSaveFileEnemyPosition(e);
					}
				}
			}
			state = ES_SELECTING;
			//selectedIdx = -1;
		}
		else if (state == ES_SELECTING)
		{
			placeElement();
		}
		*/
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
			if (p && selectedNode >= 0 && selectedNode < p->nodes.size())
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
		/*
		else if (selectedIdx != -1)
		{
			if (selectedIdx >= 0 && selectedIdx < dsq->game->getNumPaths())
			{
				Path *p = dsq->game->getPath(selectedIdx);
				PathNode n;
				n.position = dsq->getGameCursorPosition();
				p->nodes.push_back(n);
			}
		}
				else if (selectedIdx != -1)
		{
			if (selectedIdx >= 0 && selectedIdx < dsq->game->getNumPaths())
			{
				Path *p = dsq->game->getPath(selectedIdx);
				p->nodes[selectedNode] =
				PathNode n;
				n.position = dsq->getGameCursorPosition();
				p->nodes.push_back(n);
			}
		}
		*/
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
		/*
		switch(state)
		{
		case ES_SELECTING:
			selectedType = possibleSelectedType;
			selectedIdx = possibleSelectedIdx;
		break;
		case ES_MOVING:
			getSelectedRenderObject()->position = oldPosition;
			state = ES_SELECTING;
		break;
		case ES_ROTATING:
			getSelectedRenderObject()->rotation = oldRotation;
			state = ES_SELECTING;
		break;
		case ES_SCALING:
			getSelectedRenderObject()->scale = oldScale;
			state = ES_SELECTING;
		break;
		}
		*/
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

class Row
{
public:
	Row()
	{
		x1=x2=y=0;
		rows=1;
	}
	int x1, x2, y;
	int rows;
};

bool getGrassPixel(pngRawInfo *png, int x, int y)
{
	if (x >= png->Width || y >= png->Height || x < 0 || y < 0) return false;

	//int c = ((x*png->Width)*3)+y*3;
	int c = (y*png->Width)*png->Components + x*png->Components;
	if (png->Data[c] == 128 &&
		png->Data[c+1] == 255 &&
		png->Data[c+2] == 128)
	{
		return true;
	}
	return false;
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
	pngRawInfo rawinfo;
	std::string file = getMapTemplateFilename();
	bool success = pngLoadRaw(file.c_str(), &rawinfo);
	if (success)
	{
		skinLevel(&rawinfo, skinMinX, skinMinY, skinMaxX, skinMaxY);
		if (rawinfo.Data != NULL)
			free(rawinfo.Data);
	}
}

void SceneEditor::skinLevel(pngRawInfo *png, int minX, int minY, int maxX, int maxY)
{
	std::vector<Element*> deleteElements;
	int i = 0;
	for (i = 0; i < dsq->getNumElements(); i++)
	{
		Element *e = dsq->getElement(i);
		if (e->bgLayer==4 && e->templateIdx >= 1 && e->templateIdx <= 4)
		{
			e->safeKill();
			deleteElements.push_back(e);
			/*
			e->setLife(0.1);
			e->setDecayRate(1);
			deleteElements.push_back(e);
			*/
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
			if (dsq->game->isObstructed(t,OT_BLACK)
				&& (
				!dsq->game->isObstructed(TileVector(x+1,y),OT_BLACK) ||
				!dsq->game->isObstructed(TileVector(x-1,y),OT_BLACK) ||
				!dsq->game->isObstructed(TileVector(x,y-1),OT_BLACK) ||
				!dsq->game->isObstructed(TileVector(x,y+1),OT_BLACK)
				)
				)
			{
				// do color check
				/*
				int ci = x+(y*png->Height);
				if (png->data[ci] < pixelColor.x &&
					png->data[ci+1] < pixelColor.y &&
					png->data[ci+2] < pixelColor.z)
				{
				*/
				float dist=0;
				wallNormal = dsq->game->getWallNormal(t.worldVector(), 5, &dist, OT_BLACK);
				offset = wallNormal*(-TILE_SIZE*0.6f);
				MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), wallNormal, rot);
				rot = 180-(360-rot);
				addTile = true;
				//}
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
				for (int i = 0; i < dsq->getNumElements(); i++)
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
					int i = 0;
					for (i = 0; i < dsq->getNumElements(); i++)
					{
						Element *e = dsq->getElement(i);
						if (e->templateIdx <= 4 && e->templateIdx >= 1)
						{
							if ((p - e->position).getSquaredLength2D() < sqr(120))//sqr(60*3+10)) // 120
							{
								cantUse[e->templateIdx-1]++;
							}
						}
					}
					int useIdx = rand()%cantUse.size()+1;
					for (i = 0; i < cantUse.size(); i++)
					{
						int check = i + idxCount;
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


					/*
					bool addGrass = false;
					int search = 2;
					for (int dx = -search; dx < search; dx++)
					{
						for (int dy = -search; dy < search; dy++)
						{
							if (getGrassPixel(png, x+dx, y+dy))
							{
								//std::ostringstream os;
								//os << "found grass pixel at (" << x+dx << ", " << y+dy << ")";
								//debugLog(os.str());
								//errorLog ("add grass");
								addGrass = true;
								break;
							}
						}
					}

					if (addGrass)
					{
						//Vector detailPos = p + wallNormal*48;
						Element *grassE = dsq->game->createElement(5, p, 0, &q);
							//dsq->game->createElement(5, detailPos, 6, &q);
						//grassE->offset = offset;
					}
					*/


					/*
					float sz = ((rand()%1000)/4000.0f);
					e->scale = Vector(1+sz, 1+sz, 1);
					*/


					idx++;
					if(idx > 4)
						idx = 1;
				}
			}
		}
		for (int i = 0; i < dsq->getNumElements(); i++)
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

void SceneEditor::fixEntityIDs()
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		e->assignUniqueID();
	}
}

void SceneEditor::generateLevel()
{
	//pngSetStandardOrientation(0);
	std::string file=getMapTemplateFilename();
	//pngInfo info;
	//PNG_ALPHA

	//errorLog("generate level");
	// Y R G B P
		int maxX=0, maxY=0;
	const int YELLOW=0, RED=1, GREEN=2, BLUE=3, PURPLE=4, ORANGE=5, BROWN=6, MAX=7;
	int firstColorX[MAX], firstColorY[MAX];
	int lastColorX[MAX], lastColorY[MAX];
	Vector colorVects[MAX];
	colorVects[YELLOW] = Vector(1,1,0);
	colorVects[RED]	= Vector(1,0,0);
	colorVects[GREEN] = Vector(0,1,0);
	colorVects[BLUE] = Vector(0,0,1);
	colorVects[PURPLE] = Vector(1,0,1);
	colorVects[ORANGE] = Vector(1,0.5,0);
	colorVects[BROWN] = Vector(0.5,0.25,0);
	for (int i = 0; i < MAX; i++)
	{
		firstColorX[i] = firstColorY[i] = -1;
		lastColorX[i] = lastColorY[i] = -1;
	}
	pngRawInfo rawinfo;
	bool success = pngLoadRaw(file.c_str(), &rawinfo);
	if (success)
	{
		//dsq->elements.clear();
		std::vector<Row> rows;
		std::vector<Vector> positions;
		const int maxRowCount = 9999;//9999;//9999;
		int rowCount = 0;
		if (rawinfo.Components < 3)
		{
			errorLog("png color depth ( < 3 bit) not supported by generate level");
		}
		int scale = TILE_SIZE;
		int c = 0;
		//for (int y = rawinfo.Height-1; y >= 0; y--)
		for (int y = 0; y < rawinfo.Height; y++)
		{
			Vector lastElement;
			lastElement = Vector(0,0,0);
			bool hasLastElement = false;
			Vector *firstRowElement = 0;
			Row row;
			rowCount = 0;
			positions.clear();
			for (int x = 0; x < rawinfo.Width; x++)
			{
				Vector *e = 0;
				if
				(
					(
						rawinfo.Data[c] < 48 &&
						rawinfo.Data[c+1] < 48 &&
						rawinfo.Data[c+2] < 48
					)
					||
					(
						rawinfo.Data[c] == 128
						&&
						rawinfo.Data[c+1] == 255
						&&
						rawinfo.Data[c+2] == 128
					)
				)
				{
					if (x > maxX)
						maxX = x;
					if (y > maxY)
						maxY = y;
					positions.push_back(Vector(x*scale+(scale/2.0f),y*scale+(scale/2.0f)));
					e = &positions[positions.size()-1];
				}
				if (rawinfo.Data[c] < 32 &&
					rawinfo.Data[c+1] > 200 &&
					rawinfo.Data[c+2] > 200)
				{
					dsq->game->saveWaterLevel = dsq->game->waterLevel.x = y*TILE_SIZE;
				}
				for (int i = 0; i < MAX; i++)
				{
					//if (checkWarpPixel(rawinfo.Data, c, colorVects[i]))
					bool p1, p2, p3;
					p1=p2=p3=false;
					int diff;
					diff = fabsf((colorVects[i].x*255) - rawinfo.Data[c]);
					p1 = (diff < 5);
					diff = fabsf((colorVects[i].y*255) - rawinfo.Data[c+1]);
					p2 = (diff < 5);
					diff = fabsf((colorVects[i].z*255) - rawinfo.Data[c+2]);
					p3 = (diff < 5);
					/*
					p1 = (colorVects[i].x == 1 && rawinfo.Data[c] > 200);
					if (!p1)
					{
						p1 = (colorVects[i].x == 0 && rawinfo.Data[c] < 32);
					}
					p2 = (colorVects[i].y == 1 && rawinfo.Data[c+1] > 200);
					if (!p2)
					{
						p2 = (colorVects[i].y == 0 && rawinfo.Data[c+1] < 32);
						if (!p2)
						{
							p2 = (colorVects[i].y == 0.5f && rawinfo.Data[c+1] > 96 && rawinfo.Data[c+1] < 164);
						}
					}
					p3 = (colorVects[i].z == 1 && rawinfo.Data[c+2] > 200);
					if (!p3)
						p3 = (colorVects[i].z == 0 && rawinfo.Data[c+2] < 32);
					*/
					if (p1 && p2 && p3)
					{
						lastColorX[i] = x;
						lastColorY[i] = y;
						if (firstColorX[i] == -1)
						{
							firstColorX[i] = x;
							firstColorY[i] = y;
						}
					}
				}
				/*
				else if (checkPixel(1, 0, 0))
				{
					lastColorX[RED] = x;
					lastColorY[RED] = y;
					if (firstColorX[RED] == -1)
					{
						firstColorX[RED] = x;
						firstColorY[RED] = y;
					}
				}
				*/
				/*
				else if (	rawinfo.Data[c]		> 200	&&
							rawinfo.Data[c+1]	< 32	&&
							rawinfo.Data[c+2]	< 32)
				{

				}
				else if (rawinfo.
				*/

				c += rawinfo.Components;
				if ((e==0 && firstRowElement) || (firstRowElement && rowCount >= maxRowCount && hasLastElement)
					|| (firstRowElement && x == rawinfo.Width-1))
				{
					/*
					if (x == rawinfo.Width-1)
						row.x2 = rawinfo.Width-1;
					else
					{
					*/
					// HACK: it crashes here:
					// because lastElement is garbage data
					// fixed!
					if (hasLastElement)
						row.x2 = lastElement.x;
					//}

					hasLastElement = false;
					firstRowElement = 0;

					bool add = true;
					/*
					for (int i = 0; i < rows.size(); i++)
					{
						if (rows[i].x1 == row.x1 && rows[i].x2 == row.x2)
						{
							if (abs(rows[i].y - row.y) <= TILE_SIZE+1)
							{
								rows[i].rows++;
								add = false;
								break;
							}
						}
					}
					*/
					if (add)
						rows.push_back(row);
				}
				if (!firstRowElement && e)
				{
					row.x1 = e->x;
					row.y = e->y;
					firstRowElement = e;
					rowCount = 1;
				}
				if (e)
				{
					lastElement = *e;
					hasLastElement = true;
				}
				else
					hasLastElement = false;
				rowCount ++ ;
			}
		}

		dsq->game->clearObsRows();
		int i = 0;
		for (i = 0; i < rows.size(); i++)
		{
			int w = rows[i].x2 - rows[i].x1;
			//int h = scale * rows[i].rows;
			int useY = rows[i].y;
			if (rows[i].rows > 1)
			{
				useY += (rows[i].rows-1)*TILE_SIZE/2;
			}

			dsq->game->addObsRow(rows[i].x1/TILE_SIZE, rows[i].y/TILE_SIZE, w/TILE_SIZE);
			/*
			BoxElement *box = new BoxElement(w,h);

			box->position = Vector(rows[i].x1 + int(w/2), useY);
			box->position.z = boxElementZ;
			dsq->game->addRenderObject(box, BLACKGROUND);
			dsq->elements.push_back(box);
			*/
		}

		dsq->game->reconstructGrid(true);

		// create warp areas
		dsq->game->warpAreas.clear();
		for (i = 0; i < MAX; i++)
		{
			if (firstColorX[i] != -1)
			{
				WarpArea warpArea;
				int w = lastColorX[i] - firstColorX[i];
				int h = lastColorY[i] - firstColorY[i];

				warpArea.w = w*TILE_SIZE;
				warpArea.h = h*TILE_SIZE;
				warpArea.position = Vector(firstColorX[i]*TILE_SIZE + warpArea.w/2, firstColorY[i]*TILE_SIZE + warpArea.h/2);
				warpArea.w += TILE_SIZE*4;
				warpArea.h += TILE_SIZE*4;

				switch (i)
				{
				case RED:		warpArea.warpAreaType = "Red";			break;
				case BLUE:		warpArea.warpAreaType = "Blue";			break;
				case GREEN:		warpArea.warpAreaType = "Green";		break;
				case YELLOW:	warpArea.warpAreaType = "Yellow";		break;
				case PURPLE:	warpArea.warpAreaType = "Purple";		break;
				case ORANGE:	warpArea.warpAreaType = "Orange";		break;
				case BROWN:		warpArea.warpAreaType = "Brown";		break;
				}

				dsq->game->setWarpAreaSceneName(warpArea);

				warpArea.generated = true;

				std::ostringstream os;
				os << "WarpArea (" << warpArea.w << ", " << warpArea.h << ") - pos(" << warpArea.position.x << ", " <<
					warpArea.position.y << ")";
				debugLog(os.str());

				dsq->game->warpAreas.push_back(warpArea);
			}
		}

		maxX--;
		maxY--;
		this->skinMinX = 4;
		this->skinMinY = 4;
		this->skinMaxX = maxX;
		this->skinMaxY = maxY;
		//skinLevel(&rawinfo, 4, 4, maxX, maxY);
		if (rawinfo.Data != NULL)
			free(rawinfo.Data);
	}
	else
	{
		debugLog("generateLevel: Failed to load mapTemplate");
	}
//	pngSetStandardOrientation(1);
}

void SceneEditor::removeEntity()
{
	debugLog("remove entity at cursor");
	dsq->game->removeEntityAtCursor();
}

void SceneEditor::placeAvatar()
{
	dsq->game->avatar->position = dsq->getGameCursorPosition();
	QuadLight::clearQuadLights();
	QuadLight::addQuadLight(QuadLight(Vector(dsq->game->avatar->position), Vector(1, 0, 0), 1200));
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
	if (editingElement)
		editingElement->flipHorizontal();
	else
		this->placer->flipHorizontal();
}

void SceneEditor::flipElementVert()
{
	if (editingElement)
		editingElement->flipVertical();
	else
		this->placer->flipVertical();
}

void SceneEditor::moveElementToLayer(Element *e, int bgLayer)
{
	if (!selectedElements.empty())
	{
		for (int i = 0; i < selectedElements.size(); i++)
		{
			Element *e = selectedElements[i];
			core->removeRenderObject(e, Core::DO_NOT_DESTROY_RENDER_OBJECT);
			core->addRenderObject(e, LR_ELEMENTS1+bgLayer);
			e->bgLayer = bgLayer;
		}
	}
	else if (e)
	{
		core->removeRenderObject(e, Core::DO_NOT_DESTROY_RENDER_OBJECT);
		core->addRenderObject(e, LR_ELEMENTS1+bgLayer);
		e->bgLayer = bgLayer;
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

		for (int i = 0; i < dsq->getNumElements(); i++)
		{
			Element *e = dsq->getElement(i);
			if (e->bgLayer == bgLayer && e->position.x >= p1.x && e->position.y >= p1.y && e->position.x <= p2.x && e->position.y <= p2.y)
			{
				selectedElements.push_back(e);
			}
		}
	}
}

void SceneEditor::action(int id, int state)
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
		/*
		std::string copy = action;
		copy = copy.substr(std::string("bgLayer").length(), action.length());
		std::istringstream is(copy);
		int v;
		is >> v;
		this->bgLayer = v-1;
		*/

		int v = id - ACTION_BGLAYER1;
		this->bgLayer = v;

		updateText();

		if (core->getAltState())
		{
			dsq->game->setElementLayerVisible(bgLayer, !dsq->game->isElementLayerVisible(bgLayer));
			//core->getRenderObjectLayer(LR_ELEMENTS1+bgLayer)->visible = !core->getRenderObjectLayer(LR_ELEMENTS1+bgLayer)->visible;
		}
		else if (core->getShiftState() && (editingElement || !selectedElements.empty()))
		{
			moveElementToLayer(editingElement, bgLayer);
			//editingElement->bgLayer
			//editingElement->bgLayer = this->bgLayer;
		}
		else
		{
			selectedElements.clear();
		}

	}
	/*
	Vector multiSelectPoint, secondMultiSelectPoint;
	*/
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
}

void SceneEditor::saveScene()
{
	dsq->screenMessage(dsq->game->sceneName + " Saved!");
	dsq->game->saveScene(dsq->game->sceneName);
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
		//se_grad->safeKill();
		se_grad->setLife(1);
		se_grad->setDecayRate(10);
		se_grad->fadeAlphaWithLife = 1;
		se_grad = 0;
	}
	for (int i = 0; i < qs.size(); i++)
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
	//Gradient()
	//se_grad->makeHorizontal(Vector(0,0,0.3), Vector(0,0,0.1));
	se_grad->scale = Vector(800, 500);
	se_grad->position = Vector(400,350);
	se_grad->followCamera = 1;
	se_grad->alpha = 0;
	se_grad->color = Vector(0,0,0);
	se_grad->alpha.interpolateTo(0.8, 0.2);
	dsq->game->addRenderObject(se_grad, LR_HUD);

	EntityGroup &group = game->entityGroups[game->sceneEditor.entityPageNum];

	for (int i = 0; i < group.entities.size(); i++)
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
			int j=0;
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
			//q->setWidthHeight(sizing, sizing);
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
	if (game->sceneEditor.entityPageNum < 0)
		game->sceneEditor.entityPageNum = game->entityGroups.size()-1;

	createEntityPage();
}

//page = game->entityGroups.begin();

void SceneEditor::selectEntityFromGroups()
{
	createEntityPage();

	placer->alpha = 0;
	se_changedEntityType = false;
	editType = ET_SELECTENTITY;


	//bool done = false;
	//bool mbrd = false;
	bool mbld = false;
	bool ld = false, rd = false;
	ld = core->getKeyState(KEY_E);
	while (!se_changedEntityType)
	{
		/*
		if (core->mouse.buttons.right && !mbrd)
			mbrd = true;
		else if (!core->mouse.buttons.right && mbrd)
		{
			mbrd = false;
			nextEntityPage();
		}
		*/
		
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

		core->main(FRAME_TIME);
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
	int ce = e1->templateIdx;
	int idx=0;
	for (int i = 0; i < dsq->game->elementTemplates.size(); i++)
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
	int ce = e1->templateIdx;
	int idx=0;
	for (int i = 0; i < dsq->game->elementTemplates.size(); i++)
	{
		if (dsq->game->elementTemplates[i].idx == ce)
		{
			idx = i;
			break;
		}
	}
	ce = idx;
	ce--;
	if (ce < 0)
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

	if (core->getShiftState())
	{
		debugLog("rebuilding level!");
		dsq->game->reconstructGrid(true);
		return;
	}

	if (editType == ET_ENTITIES)
	{
		/*
		if (editingEntity)
		{
			// swap entity type up (somehow)

		}
		else
		{
			curEntity++;

			if (curEntity >= dsq->game->entityTypeList.size())
				curEntity = 0;

			updateEntityPlacer();
		}
		*/
	}
	else if (editType == ET_ELEMENTS)
	{
		if (dsq->game->elementTemplates.empty()) return;
		if (core->getCtrlState())
		{
			if (!selectedElements.empty())
			{
				for (int i = 0; i < selectedElements.size(); i++)
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
			for (int i = 0; i < selectedElements.size(); i++)
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
				//int idx = dsq->game->elementTemplates[curElement].idx;
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
	if (editType == ET_ENTITIES)
	{
		/*
		if (editingEntity)
		{
			// swap entity type up (somehow)
		}
		else
		{
			curEntity--;

			if (curEntity < 0)
				curEntity = dsq->game->entityTypeList.size()-1;

			updateEntityPlacer();


			//dsq->game->entityTypeList
		}
		*/
	}
	else if (editType == ET_ELEMENTS)
	{
		if (dsq->game->elementTemplates.empty()) return;
		if (!selectedElements.empty())
		{
			for (int i = 0; i < selectedElements.size(); i++)
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
	int oldCur = curElement;
	curElement--;
	if (curElement < 0)
		curElement = dsq->game->elementTemplates.size()-1;

	if (dsq->game->elementTemplates[curElement].idx < 1024)
	{
		//int idx = dsq->game->elementTemplates[curElement].idx;
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
		for (int i = 0; i < dsq->getNumElements(); i++)
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
		/*
		int realLayer1 = LR_ELEMENTS1 + fromLayer, realLayer2 = LR_ELEMENTS1 + toLayer;
		std::vector<RenderObject*> move;
		for (int i = 0; i < dsq->renderObjectLayers[realLayer1].renderObjects.size(); i++)
		{
			RenderObject *r = dsq->renderObjectLayers[realLayer1].renderObjects[i];
			if (r)
			{
				move.push_back(r);
				core->removeRenderObject(r, Core::DO_NOT_DESTROY_RENDER_OBJECT);
				//dsq->renderObjectLayers[realLayer1].renderObjects[i] = 0;
			}
		}
		for (int i = 0; i < move.size(); i++)
		{
			RenderObject *r = move[i];
			if (r)
			{
				r->layer = realLayer2;
				dsq->addRenderObject(r, realLayer2);
			}
		}
		*/
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
			int largest = 0;
			for (int i = 0; i < dsq->game->elementTemplates.size(); i++)
			{
				ElementTemplate et = dsq->game->elementTemplates[i];
				if (et.idx < 1024 && et.idx > largest)
				{
					largest = et.idx;
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
		if (!core->getShiftState() && !core->getKeyState(KEY_LALT) && !drawingBox)
		{
			//errorLog("placeElement");

			dsq->game->createElement(dsq->game->elementTemplates[curElement].idx, placer->position, bgLayer, placer);
			updateText();
			dsq->game->reconstructGrid();
		}
	}
	else if (editType == ET_ENTITIES)
	{
		if (!selectedEntity.nameBased)
			dsq->game->createEntity(selectedEntity.index, 0, dsq->getGameCursorPosition(), 0, true, "", ET_ENEMY, BT_NORMAL, 0, 0, true);
		else
			dsq->game->createEntity(selectedEntity.name, 0, dsq->getGameCursorPosition(), 0, true, "", ET_ENEMY, BT_NORMAL, 0, 0, true);
	}
	else if (editType == ET_PATHS)
	{
		if (core->getCtrlState())
		{
			// new path
			Path *p = new Path;
			p->name = "NewPath";
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
			/*
			if (selectedIdx >= 0 && selectedIdx < dsq->game->getNumPaths())
			{
				Path *p = dsq->game->getPath(selectedIdx);
				PathNode n;
				n.position = dsq->getGameCursorPosition();
				p->nodes.push_back(n);
			}
			*/
		}
	}
}

void SceneEditor::cloneSelectedElementInput()
{
	/*
	if (core->getShiftState())
	{
		cloneSelectedElement();
	}
	*/
}

void SceneEditor::cloneSelectedElement()
{
	if (editType == ET_ELEMENTS)
	{
		if (!selectedElements.empty())
		{
			std::vector<Element*>copy;
			Vector groupCenter = this->getSelectedElementsCenter();
			for (int i = 0; i < selectedElements.size(); i++)
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
			//e->repeatTextureToFill(e1->isRepeatingTextureToFill());
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
		}
	}

	/*
	if (selectedIdx > -1)
	{
		Element *e1 = dsq->elements[selectedIdx];
		Element *e = dsq->game->createElement(e1->templateIdx, placer->position, e1->layer-LR_ELEMENTS1, getSelectedRenderObject());
		selectedIdx = dsq->elements.size()-1;
	}
	*/
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

		warpAreaRender->alpha = 0.5;
		dsq->game->togglePause(on);
		if (dsq->game->avatar)
			dsq->game->avatar->disableInput();
		text->alpha.interpolateTo(1, 0.5);
		placer->alpha.interpolateTo(0.5, 0.5);
		movingEntity = 0;
		dsq->toggleCursor(true);
		dsq->setCursor(CURSOR_NORMAL);
		//core->flags.set(CF_CLEARBUFFERS);

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
	}
	else
	{
		btnMenu->alpha = 0;
		//dsq->game->reconstructGrid();
		selectedElements.clear();
		for (int i = 0; i < 9; i++)
			dsq->getRenderObjectLayer(LR_ELEMENTS1+i)->visible = true;

		if (movingEntity)
			endMoveEntity();
		movingEntity = 0;

		dsq->getRenderObjectLayer(LR_BLACKGROUND)->update = false;

		warpAreaRender->alpha = 0;
		dsq->game->togglePause(on);
		if (dsq->game->avatar)
			dsq->game->avatar->enableInput();
		text->alpha.interpolateTo(0, 0.2);
		placer->alpha.interpolateTo(0, 0.2);
		//core->flags.unset(CF_CLEARBUFFERS);
		target->alpha = 0;
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
	std::ostringstream os;
	os << dsq->game->sceneName << " bgL[" << bgLayer << "] (" <<
		(int)dsq->cameraPos.x << "," << (int)dsq->cameraPos.y << ") ("
		//<< (int)dsq->game->avatar->position.x
		//<< "," << (int)dsq->game->avatar->position.y << "," << (int)dsq->game->avatar->position.z << ")" << " ("
		<< (int)dsq->getGameCursorPosition().x << "," << (int)dsq->getGameCursorPosition().y << ")" << " ";
	switch(editType)
	{
	case ET_ELEMENTS:
		os << "elements";
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
			}
		}
	break;
	case ET_ENTITIES:
		os << "entities";
		if (editingEntity)
		{
			os << " id: " << editingEntity->getID() << " name: " << editingEntity->name << " flag: " << dsq->continuity.getEntityFlag(dsq->game->sceneName, editingEntity->getID());
			os << " groupID: " << editingEntity->getGroupID() << " ";
			os << " state: " << editingEntity->getState();
		}
	break;
	case ET_PATHS:
		os << "paths si[" << selectedIdx << "]";
		if (getSelectedPath())
			os << " name: " << getSelectedPath()->name;
	break;
	}
	text->setText(os.str());
}

void SceneEditor::startDrawingWarpArea(char c)
{
	if (drawingWarpArea == 'N')
	{
		boxPos = dsq->getGameCursorPosition();
		boxPromo->alpha = 1;
		switch(c)
		{
		case 'Y': boxPromo->color = Vector(1,1,0); break;
		case 'U': boxPromo->color = Vector(1,0,0); break;
		case 'I': boxPromo->color = Vector(0,1,0); break;
		case 'O': boxPromo->color = Vector(0,0,1); break;
		case 'P': boxPromo->color = Vector(1,0,1); break;
		}
		drawingWarpArea = c;
	}
	if (drawingWarpArea == c)
	{
		Vector diff = dsq->getGameCursorPosition() - boxPos;
		boxPromo->setWidthHeight(diff.x, diff.y);
		boxPromo->position = boxPos + diff/2;
	}
}

void SceneEditor::updateDrawingWarpArea(char c, int k)
{
	if (core->getKeyState(k))
	{
		startDrawingWarpArea(c);
	}
	else
	{
		endDrawingWarpArea(c);
	}
}

void SceneEditor::endDrawingWarpArea(char c)
{
	if (drawingWarpArea == c)
	{
		drawingWarpArea = 'N';
		boxPromo->alpha = 0;
		if (boxPromo->getWidth() > TILE_SIZE && boxPromo->getHeight() > TILE_SIZE)
		{
			WarpArea a;
			a.position = boxPromo->position;
			a.w = boxPromo->getWidth()/2;
			a.h = boxPromo->getHeight()/2;
			switch (c)
			{
			case 'Y': a.warpAreaType = "Yellow"; break;
			case 'U': a.warpAreaType = "Red"; break;
			case 'I': a.warpAreaType = "Green"; break;
			case 'O': a.warpAreaType = "Blue"; break;
			case 'P': a.warpAreaType = "Purple"; break;
			}

			a.sceneName = dsq->getUserInputString("Enter map to warp to");
			a.spawnOffset = dsq->getUserInputDirection("Enter warp direction");
			dsq->game->warpAreas.push_back(a);
			/*
			BoxElement *boxElement = new BoxElement(boxPromo->width.getValue(), boxPromo->height.getValue());
			boxElement->position = boxPromo->position;
			boxElement->position.z = boxElementZ;
			dsq->game->addRenderObject(boxElement, BLACKGROUND);
			dsq->elements.push_back(boxElement);
			dsq->game->reconstructGrid();
			*/
		}
	}
}

Vector SceneEditor::getSelectedElementsCenter()
{
	Vector c;
	for (int i = 0; i < selectedElements.size(); i++)
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
			std::string m = "Map AutoSaved to " + os.str();
			dsq->game->saveScene(os.str());
			dsq->debugLog(m);
			dsq->screenMessage(m);

			autoSaveFile++;
			if (autoSaveFile > vars->autoSaveFiles)
				autoSaveFile = 0;
		}

		updateMultiSelect();
		switch (editType)
		{
		case ET_ELEMENTS:
			editingEntity = 0;
			if (isActing(ACTION_MULTISELECT) || !selectedElements.empty())
			{
				editingElement = 0;
			}
			if (state == ES_SELECTING && !isActing(ACTION_MULTISELECT))
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
		}

		updateText();
		ActionMapper::onUpdate(dt);

		TileVector cursorTile(dsq->getGameCursorPosition());
		Vector p = Vector(cursorTile.x*TILE_SIZE+TILE_SIZE/2, cursorTile.y*TILE_SIZE+TILE_SIZE/2);
		placer->position = p;


		//selectedIdx = idx;

		int camSpeed = 500/zoom.x;
		if (core->getShiftState())
			camSpeed = 5000/zoom.x;
		if (isActing(ACTION_CAMLEFT))
			dsq->cameraPos.x -= dt*camSpeed;
		if (isActing(ACTION_CAMRIGHT))
			dsq->cameraPos.x += dt*camSpeed;
		if (isActing(ACTION_CAMUP))
			dsq->cameraPos.y -= dt*camSpeed;
		if (isActing(ACTION_CAMDOWN))
			dsq->cameraPos.y += dt*camSpeed;
		if (core->mouse.buttons.middle && !core->mouse.change.isZero())
		{
			dsq->cameraPos += core->mouse.change*(4/zoom.x);
			core->setMousePosition(core->mouse.lastPosition);
		}

		float spd = 0.5;
		const Vector oldZoom = zoom;
		if (isActing(ACTION_ZOOMOUT))
			zoom /= (1 + spd*dt);
		else if (isActing(ACTION_ZOOMIN))
			zoom *= (1 + spd*dt);
		else if (core->mouse.scrollWheelChange < 0)
			zoom /= 1.05f;
		else if (core->mouse.scrollWheelChange > 0)
			zoom *= 1.05f;
		if (zoom.x < 0.04f)
			zoom.x = zoom.y = 0.04f;
		core->globalScale = zoom;
		if (zoom.x != oldZoom.x)
		{
			const float mouseX = core->mouse.position.x;
			const float mouseY = core->mouse.position.y;
			dsq->cameraPos.x += mouseX/oldZoom.x - mouseX/zoom.x;
			dsq->cameraPos.y += mouseY/oldZoom.y - mouseY/zoom.y;
		}

		/*
		for (int i = 0; i < dsq->elements.size(); i++)
		{
			//dsq->elements[i]->flags.unset(RO_RENDERBORDERS);
			dsq->elements[i]->renderBorders = false;
		}
		RenderObject *r;
		if (r = getSelectedRenderObject())
		{
			//r->flags.set(RO_RENDERBORDERS);
			r->renderBorders = true;
		}
		*/
		if (this->editType == ET_PATHS)
		{
			switch(state)
			{
			case ES_SELECTING:
			{
				selectedIdx = -1;
				selectedNode = -1;
				float smallestDist = sqr(64);
				for (int i = 0; i < dsq->game->getNumPaths(); i++)
				{
					for (int n = dsq->game->getPath(i)->nodes.size()-1; n >=0; n--)
					{
						Vector v = dsq->game->getPath(i)->nodes[n].position - dsq->getGameCursorPosition();
						float dist = v.getSquaredLength2D();
						if (dist < smallestDist)
						{
							smallestDist = dist;
							selectedIdx = i;
							selectedNode = n;
							//return;
						}
					}
				}
			}
			break;
			case ES_SCALING:
			{
				float factor = 1;
				Vector add = Vector((dsq->getGameCursorPosition().x - cursorOffset.x)*factor,
					(dsq->getGameCursorPosition().y - cursorOffset.y)*factor);
				//editingElement->scale=oldScale + add;
				Vector sz = oldScale + add;
				if (sz.x < 64)
					sz.x = 64;
				if (sz.y < 64)
					sz.y = 64;
				editingPath->rect.x1 = -sz.x/2;
				editingPath->rect.x2 = sz.x/2;
				editingPath->rect.y1 = -sz.y/2;
				editingPath->rect.y2 = sz.y/2;
			}
			break;
			case ES_MOVING:
				dsq->game->getPath(selectedIdx)->nodes[selectedNode].position = dsq->getGameCursorPosition() + cursorOffset;
			break;
			}
		}
		else if (editType == ET_ENTITIES)
		{
			switch(state)
			{
			case ES_MOVING:
				editingEntity->position = dsq->getGameCursorPosition() + cursorOffset;
				/*
				if (core->getShiftState())
				{
					TileVector t(getSelectedRenderObject()->position);
					getSelectedRenderObject()->position = t.worldVector();
				}
				*/
			break;
			case ES_ROTATING:
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
		}
		else if (editType == ET_ELEMENTS)
		{
			switch(state)
			{
			case ES_SELECTING:
			{
				float closest = sqr(800);
				int idx = -1, i = 0;
				for (i = 0; i < dsq->getNumElements(); i++)
				{
					Vector dist = dsq->getElement(i)->getFollowCameraPosition() - dsq->getGameCursorPosition();
					float len = dist.getSquaredLength2D();
					if (len < closest)
					{
						closest = len;
						idx = i;
						possibleSelectedType = 0;
					}
				}

				/*
				FOR_ENTITIES(i)
				{
					Entity *e = *i;
					Vector dist = e->position - dsq->getGameCursorPosition();
					int len = dist.getSquaredLength2D();
					if ((len < closest || closest == -1))
					{
						closest = len;
						idx = i;
						possibleSelectedType = 1;
					}
				}

				possibleSelectedIdx = idx;
				*/
			}
			break;
			case ES_MOVING:
				updateSelectedElementPosition(dsq->getGameCursorPosition() - cursorOffset);
				/*
				if (core->getShiftState())
				{
					TileVector t(getSelectedRenderObject()->position);
					getSelectedRenderObject()->position = t.worldVector();
				}
				*/
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
				bool right=false, middle=false, down=false, uni=false;
				bool noSide = 0;
				if (cursorOffset.x > oldPosition.x+10)
					right = true;
				else if (cursorOffset.x < oldPosition.x-10)
					right = false;
				else
					noSide++;
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

					for (int i = 0; i < selectedElements.size(); i++)
						selectedElements[i]->refreshRepeatTextureToFill();
				}
				else if (editingElement)
				{
					if (core->getCtrlState())
					{
						editingElement->scale = Vector(1,1);
					}
					else
					{
						editingElement->scale=oldScale + add;
						if (!uni)
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
						if (editingElement->scale.x < MIN_SIZE)
							editingElement->scale.x  = MIN_SIZE;
						if (editingElement->scale.y < MIN_SIZE)
							editingElement->scale.y  = MIN_SIZE;
					}

					editingElement->refreshRepeatTextureToFill();
				}
			}
			break;
			}
		}
		RenderObject *r = getSelectedRenderObject();
		if (r)
		{
			target->position = r->position;
			target->alpha = 1;
		}
		else
			target->alpha = 0;
		/*
		if (movingEntity)
		{
			movingEntity->position = dsq->getGameCursorPosition();
		}
		*/
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
	else if (editType == ET_ENTITIES)
	{
		/*
		selectedEntityType++;
		if (selectedEntityType>=dsq->game->entityTypeList.size())
		{
			selectedEntityType = 0;
		}
		*/
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
	else if (editType == ET_ENTITIES)
	{
		/*
		selectedEntityType --;
		if (selectedEntityType < 0)
		{
			selectedEntityType = dsq->game->entityTypeList.size()-1;
		}
		*/
	}
	else if (editType == ET_SELECTENTITY)
	{
		prevEntityPage();
	}
}


#endif  // AQUARIA_BUILD_SCENEEDITOR

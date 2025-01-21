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

#include "AnimationEditor.h"
#include "DSQ.h"
#include "AquariaMenuItem.h"
#include "../BBGE/Gradient.h"
#include "../BBGE/DebugFont.h"
#include "RenderBase.h"
#include "Game.h"
#include "SplineGrid.h"
#include "RenderGrid.h"


int TIMELINE_GRIDSIZE		= 10;
float TIMELINE_UNIT			= 0.1f;
float TIMELINE_UNIT_STEP	= 0.01f;
const float TIMELINE_HEIGHT = 120;
const int KEYFRAME_POS_Y	= 480;
const int KEYFRAME_HEIGHT_Y	= 12;
const float TIMELINE_X_OFFS = 5;
const float TIMELINE_CENTER_Y = 535;


enum { NumPages = 9 }; // one for each number key 1-9

const Vector ScreenMsgPos(210, 55);


class TimelineRender;


class KeyframeWidget : public Quad
{
public:
	KeyframeWidget(TimelineRender *tr, int key, int page);
	float t;
	int key, page;
	static KeyframeWidget *movingWidget;
	TimelineRender * const timeline;

	void shiftLeft();
	void shiftRight();
protected:
	void onUpdate(float dt);
};

AnimationEditor *ae = 0;

KeyframeWidget *KeyframeWidget::movingWidget = 0;

static void notify(const std::string& s)
{
	dsq->screenMessage(s, ScreenMsgPos);
}

class TimelineTickRender : public RenderObject
{
public:
	TimelineTickRender()
		: bg("black", Vector(400, 0))
	{
		addChild(&bg, PM_STATIC, RBP_ON);
		bg.setWidthHeight(800, TIMELINE_HEIGHT);
		bg.alphaMod = 0.2f;
	}

	virtual ~TimelineTickRender()
	{
	}

private:
	Quad bg;
	void onRender(const RenderState& rs) const OVERRIDE
	{
		glLineWidth(1);
		glBegin(GL_LINES);
		glColor4f(1, 1, 1, 1);
		const float h2 = TIMELINE_HEIGHT * 0.5f;
		for (int x = 0; x < 800; x += TIMELINE_GRIDSIZE)
		{
			glVertex3f(x + TIMELINE_X_OFFS, -h2, 0);
			glVertex3f(x + TIMELINE_X_OFFS, h2, 0);
		}
		glColor4f(0, 1, 0, 1);
		float tx = ae->getAnimTime() * (TIMELINE_GRIDSIZE / TIMELINE_UNIT);
		glVertex3f(tx + TIMELINE_X_OFFS, -h2, 0);
		glVertex3f(tx + TIMELINE_X_OFFS, h2, 0);
		glEnd();
	}
};

class TimelineRender : public Quad
{
public:
	const int page;
	DebugFont label;

	TimelineRender(int page)
		: page(page), label(6)
	{
		setWidthHeight(800, 10);
		addChild(&label, PM_STATIC);
		label.setAlign(ALIGN_LEFT);
		label.offset = Vector(20, 0);
	}

	virtual ~TimelineRender()
	{
	}

	void setLabel(const std::string& s)
	{
		label.setText(s);
	}

	KeyframeWidget *addKeyframe()
	{
		KeyframeWidget *w = new KeyframeWidget(this, keyframes.size(), page);
		keyframes.push_back(w);
		return w;
	}

	inline const std::vector<KeyframeWidget*>& getKeyframes() const { return keyframes; }

	void resizeKeyframes(size_t n)
	{
		while(keyframes.size() > n)
		{
			KeyframeWidget *k = keyframes.back();
			k->setLife(0.03f);
			k->setDecayRate(1);
			keyframes.pop_back();
		}

		while(keyframes.size() < n)
		{
			addKeyframe();
		}
	}

private:

	void onRender(const RenderState& rs) const OVERRIDE
	{
		SkeletalSprite *spr = ae->getPageSprite(page);
		if(!spr->isLoaded())
			return;

		if(ae->curPage == page)
			glColor4f(0.6f, 0.8f, 1, 0.5f);
		else
			glColor4f(0.5f, 0.3f, 0.3f, 0.5f);

		const float h2 = height * 0.5f;
		glPushMatrix();
		glTranslatef(width * 0.5f, h2, 0);
		Quad::onRender(rs);
		glPopMatrix();

		glColor4f(0, 1, 0, 1);
		glLineWidth(3);
		glBegin(GL_LINES);
			float tx = spr->getAnimationLayer(0)->timer * (TIMELINE_GRIDSIZE / TIMELINE_UNIT);
			glVertex3f(tx + TIMELINE_X_OFFS, h2 - 5, 0);
			glVertex3f(tx + TIMELINE_X_OFFS, h2 + 5, 0);
		glEnd();
	}

	std::vector<KeyframeWidget*> keyframes;
};

struct AnimationEditorPage
{
	SkeletalSprite editSprite;
	Quad *center;
	TimelineRender *timeline;
	std::string editingFile;
	std::deque<SkeletalSprite> undoHistory;
	size_t undoEntry;

	AnimationEditorPage()
		: center(NULL), timeline(NULL), undoEntry(0)
	{
		editSprite.cull = false;
	}

	~AnimationEditorPage()
	{
		//timeline->safeKill(); // is registered to global state, not necessary
		editSprite.destroy();
	}

	void showCenter(bool on)
	{
		if(center)
			center->setHidden(!on);
	}

	bool load(const char *fn) // pass NULL to unload
	{
		if(center)
		{
			center->safeKill();
			center = NULL;
		}
		if(!fn)
			fn = "";
		clearUndoHistory();
		editingFile = fn;
		editSprite.loadSkeletal(editingFile);
		timeline->setLabel(editingFile);
		bool ok = editSprite.isLoaded();
		if(ok)
		{
			center = new Quad("missingimage", Vector(0,0));
			center->alpha.x = 0.2f;
			center->scale = Vector(2,2);
			editSprite.addChild(center, PM_POINTER);
			center->moveToFront();
		}
		return ok;
	}

	void unload()
	{
		load(NULL);
	}

	void clearUndoHistory()
	{
		undoHistory.clear();
		undoEntry = 0;
	}
};

bool AnimationEditor::isMouseInRect() const
{
	Vector mp=core->mouse.position;
	const float w2 = rect->width * 0.5f;
	const float h2 = rect->height * 0.5f;
	const float left = rect->position.x - w2;
	const float right = rect->position.x + w2;
	const float top = rect->position.y - h2;
	const float btm = rect->position.y + h2;
	return mp.x >= left && mp.x <= right && mp.y >= top && mp.y <= btm;
}

void AnimationEditor::constrainMouse()
{
	Vector mp=core->mouse.position;
	const float w2 = rect->width * 0.5f;
	const float h2 = rect->height * 0.5f;
	const float left = rect->position.x - w2;
	const float right = rect->position.x + w2;
	const float top = rect->position.y - h2;
	const float btm = rect->position.y + h2;
	bool doit = false;
	if (mp.x < left )	{ mp.x = left; doit = true; }
	if (mp.x > right)	{ mp.x = right; doit = true; }
	if (mp.y < top  )	{ mp.y = top; doit = true; }
	if (mp.y > btm  )	{ mp.y = btm; doit = true; }

	if(doit)
		core->setMousePosition(mp);
}

KeyframeWidget::KeyframeWidget(TimelineRender *tr, int key, int page)
	: Quad(), key(key), page(page), timeline(tr)
{
	setTexture("keyframe");
	setWidthHeight(15, 30);
	tr->addChild(this, PM_POINTER);
}

void KeyframeWidget::shiftLeft()
{
	if (!offset.isInterpolating())
		offset.interpolateTo(Vector(offset.x-80, 0), 0.1f, 0, 0, 0);
}

void KeyframeWidget::shiftRight()
{
	if (!offset.isInterpolating())
		offset.interpolateTo(Vector(offset.x+80, 0), 0.1f, 0, 0, 0);
}

void KeyframeWidget::onUpdate(float dt)
{
	Quad::onUpdate(dt);
	if (life != 1 || ae->isAnimating()) return;

	Animation *ani = ae->getPageAnimation(page);

	switch(ani->getKeyframe(this->key)->lerpType)
	{
	case 1:
			color = Vector(0,0,1);
	break;
	case 2:
			color = Vector(1,0,0);
	break;
	case 3:
			color = Vector(1,1,0);
	break;
	default:
			color = Vector(1,1,1);
	break;
	}

	scale.x = scale.y = 1; // Always do selection rectangle checking with full scale, otherwise they are too tiny to grab

	if (!movingWidget && isCoordinateInside(core->mouse.position))
	{
		if (core->mouse.buttons.left)
		{
			movingWidget = this;
			ae->currentKey = this->key;
			ae->selectPage(this->page);
		}
	}
	if (movingWidget == this)
	{
		float lastT = ani->getKeyframe(this->key)->t;
		this->position.x = int((core->mouse.position.x-offset.x)/TIMELINE_GRIDSIZE)*TIMELINE_GRIDSIZE+TIMELINE_GRIDSIZE/2;
		float newT = int(this->position.x/TIMELINE_GRIDSIZE)*TIMELINE_UNIT;
		ani->getKeyframe(this->key)->t = newT;
		if (core->getShiftState())
		{
			ae->moveNextWidgets(newT-lastT);
		}
	}
	else
	{
		this->position.x = ani->getKeyframe(this->key)->t*TIMELINE_GRIDSIZE*(1/TIMELINE_UNIT) + TIMELINE_GRIDSIZE/2;
	}

	if (movingWidget == this && !core->mouse.buttons.left)
	{
		movingWidget = 0;
		ae->reorderKeys();
		return;
	}

	if(!(this->key == ae->currentKey && this->page == ae->curPage))
		scale.x = scale.y = 0.6f;
}

void AnimationEditor::cycleLerpType()
{
	if (dsq->isNested()) return;
	if (editMode != AE_SELECT) return;

	Animation *a = getCurrentPageAnimation();

	if (core->getCtrlState())
	{
		if (a->getNumKeyframes() >= 2)
		{
			pushUndo();

			SkeletalKeyframe *k1 = a->getFirstKeyframe();
			SkeletalKeyframe *k2 = a->getLastKeyframe();
			if (k1 && k2)
			{
				k2->copyAllButTime(k1);
			}
			notify("Copied Loop Key");

		}
	}
	else
	{
		pushUndo();
		int& lt = a->getKeyframe(this->currentKey)->lerpType;
		lt++;
		if (lt > 3)
			lt = 0;
	}
}

AnimationEditor::AnimationEditor() : StateObject()
{
	registerState(this, "AnimationEditor");
}

void AnimationEditor::resetScaleOrSave()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
		saveFile();
	else if(core->getAltState() && editingBone)
	{
		Vector scale(1,1);
		Bone *b = editingBone;
		do
			scale *= b->scale;
		while( (b = dynamic_cast<Bone*>(b->getParent())) ); // don't want to get entity scale; that's what the anim editor uses for zooming
		std::ostringstream os;
		os << scale.x;
		if(!SDL_SetClipboardText(os.str().c_str()))
			notify("Scale copied to clipboard");
	}
	else
		getSelectedPageSprite()->scale = Vector(1,1);
}

void AnimationEditor::applyState()
{
	dsq->toggleCursor(true, 0.1f);
	core->cameraPos = Vector(0,0);
	selectedStripPoint = 0;
	mouseSelection = true;
	renderBorderMode = RENDER_BORDER_MINIMAL;
	ae = this;
	StateObject::applyState();
	editMode = AE_SELECT;
	editingBone = 0;
	editingBoneSprite = 0;
	editingBonePage = -1;
	editingBoneIdx = -1;
	currentKey = 0;
	splinegrid = 0;
	assistedSplineEdit = true;
	curPage = 0;

	spriteRoot = new Quad("missingimage", Vector(400, 300));
	spriteRoot->renderQuad = false;
	addRenderObject(spriteRoot, LR_ENTITIES);

	pages = new AnimationEditorPage[NumPages];
	for(size_t i = 0; i < NumPages; ++i)
	{
		TimelineRender *tr = new TimelineRender(i);
		tr->position = Vector(0, KEYFRAME_POS_Y + i * KEYFRAME_HEIGHT_Y);
		addRenderObject(tr, LR_BLACKGROUND);
		spriteRoot->addChild(&pages[i].editSprite, PM_STATIC);
		pages[i].timeline = tr;
	}
	pages[0].load("naija");


	addAction(MakeFunctionEvent(AnimationEditor, lmbu), MOUSE_BUTTON_LEFT, 0);
	addAction(MakeFunctionEvent(AnimationEditor, lmbd), MOUSE_BUTTON_LEFT, 1);
	addAction(MakeFunctionEvent(AnimationEditor, rmbu), MOUSE_BUTTON_RIGHT, 0);
	addAction(MakeFunctionEvent(AnimationEditor, rmbd), MOUSE_BUTTON_RIGHT, 1);
	addAction(MakeFunctionEvent(AnimationEditor, mmbd), MOUSE_BUTTON_MIDDLE, 1);


	addAction(MakeFunctionEvent(AnimationEditor, cloneBoneAhead), KEY_SPACE, 0);

	addAction(MakeFunctionEvent(AnimationEditor, prevKey), KEY_LEFT, 0);
	addAction(MakeFunctionEvent(AnimationEditor, nextKey), KEY_RIGHT, 0);

	addAction(MakeFunctionEvent(AnimationEditor, deleteKey), KEY_DELETE, 0);

	addAction(MakeFunctionEvent(AnimationEditor, resetScaleOrSave), KEY_S, 0);

	addAction(MakeFunctionEvent(AnimationEditor, quit), KEY_F12, 0);

	addAction(MakeFunctionEvent(AnimationEditor, goToTitle), KEY_ESCAPE, 0);

	addAction(MakeFunctionEvent(AnimationEditor, load), KEY_F1, 0);
	addAction(MakeFunctionEvent(AnimationEditor, loadSkin), KEY_F5, 0);

	addAction(MakeFunctionEvent(AnimationEditor, clearRot), KEY_R, 0);
	addAction(MakeFunctionEvent(AnimationEditor, clearPos), KEY_P, 0);
	addAction(MakeFunctionEvent(AnimationEditor, flipRot), KEY_D, 0);
	addAction(MakeFunctionEvent(AnimationEditor, toggleHideBone), KEY_N, 0);
	addAction(MakeFunctionEvent(AnimationEditor, _copyKey), KEY_C, 0);
	addAction(MakeFunctionEvent(AnimationEditor, _pasteKey), KEY_V, 0);

	addAction(MakeFunctionEvent(AnimationEditor, undo), KEY_Z, 0);
	addAction(MakeFunctionEvent(AnimationEditor, redo), KEY_Y, 0);


	addAction(MakeFunctionEvent(AnimationEditor, cycleLerpType), KEY_L, 0);

	addAction(MakeFunctionEvent(AnimationEditor, selectPrevBone), KEY_UP, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectNextBone), KEY_DOWN, 0);

	addAction(MakeFunctionEvent(AnimationEditor, editStripKey), KEY_E, 0);

	addAction(MakeFunctionEvent(AnimationEditor, prevAnim), KEY_PGUP, 0);
	addAction(MakeFunctionEvent(AnimationEditor, nextAnim), KEY_PGDN, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectAnim), KEY_F9, 0);
	addAction(MakeFunctionEvent(AnimationEditor, animateOrStop), KEY_RETURN, 0);

	addAction(MakeFunctionEvent(AnimationEditor, toggleRenderBorders), KEY_B, 0);
	addAction(MakeFunctionEvent(AnimationEditor, toggleMouseSelection), KEY_M, 0);
	addAction(MakeFunctionEvent(AnimationEditor, showAllBones), KEY_A, 0);
	addAction(MakeFunctionEvent(AnimationEditor, toggleGradient), KEY_G, 0);

	/*addAction(MakeFunctionEvent(AnimationEditor, decrTimelineUnit), KEY_U, 0);
	addAction(MakeFunctionEvent(AnimationEditor, incrTimelineUnit), KEY_I, 0);
	addAction(MakeFunctionEvent(AnimationEditor, decrTimelineGrid), KEY_O, 0);
	addAction(MakeFunctionEvent(AnimationEditor, incrTimelineGrid), KEY_P, 0);*/

	addAction(MakeFunctionEvent(AnimationEditor, toggleSplineMode), KEY_W, 0);
	addAction(MakeFunctionEvent(AnimationEditor, flipH), KEY_T, 0);

	addAction(MakeFunctionEvent(AnimationEditor, selectPage0), KEY_1, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage1), KEY_2, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage2), KEY_3, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage3), KEY_4, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage4), KEY_5, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage5), KEY_6, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage6), KEY_7, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage7), KEY_8, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectPage8), KEY_9, 0);



	addAction(ACTION_SWIMLEFT,	KEY_J, -1);
	addAction(ACTION_SWIMRIGHT, KEY_K, -1);
	addAction(ACTION_SWIMUP,	KEY_UP, -1);
	addAction(ACTION_SWIMDOWN,	KEY_DOWN, -1);

	const float yoffs = -55;
	const float smallw = 50;
	const float gap = 5;

	Quad *back = new Quad;
	{
		back->color = 0;
		back->setWidthHeight(800, 600);
		back->position = Vector(400,300, -0.2f);
	}
	addRenderObject(back, LR_BACKDROP);

	timelineTicks = new TimelineTickRender;
	timelineTicks->position = Vector(0, TIMELINE_CENTER_Y);
	addRenderObject(timelineTicks, LR_BACKGROUND);

	bgGrad = new Gradient;
	bgGrad->scale = Vector(800, 600);
	bgGrad->position = Vector(400,300);
	bgGrad->makeVertical(Vector(0.4f, 0.4f, 0.4f), Vector(0.8f, 0.8f, 0.8f));
	addRenderObject(bgGrad, LR_BACKDROP);

	DebugButton *a = new DebugButton(0, 0, 150);
	a->position = Vector(10, 70+yoffs);
	a->label->setText("prevKey  (LEFT)");
	a->event.set(MakeFunctionEvent(AnimationEditor, prevKey));
	addRenderObject(a, LR_HUD);

	DebugButton *a2 = new DebugButton(0, 0, 150);
	a2->position = Vector(10, 100+yoffs);
	a2->label->setText("nextKey (RIGHT)");
	a2->event.set(MakeFunctionEvent(AnimationEditor, nextKey));
	addRenderObject(a2, LR_HUD);

	DebugButton *a3 = new DebugButton(0, 0, 150);
	a3->position = Vector(10, 130+yoffs);
	a3->label->setText("cloneKey");
	a3->event.set(MakeFunctionEvent(AnimationEditor, newKey));
	addRenderObject(a3, LR_HUD);

	DebugButton *animate = new DebugButton(0, 0, 150);
	animate->position = Vector(10, 200+yoffs);
	animate->label->setText("animate (ENTER)");
	animate->event.set(MakeFunctionEvent(AnimationEditor, animate));
	addRenderObject(animate, LR_HUD);

	DebugButton *stop = new DebugButton(0, 0, 150);
	stop->position = Vector(10, 230+yoffs);
	stop->label->setText("stop  (S-ENTER)");
	stop->event.set(MakeFunctionEvent(AnimationEditor, stop));
	addRenderObject(stop, LR_HUD);

	DebugButton *prevAnimation = new DebugButton(0, 0, 150);
	prevAnimation->label->setText("prevAnim (PGUP)");
	prevAnimation->position = Vector(10, 330+yoffs);
	prevAnimation->event.set(MakeFunctionEvent(AnimationEditor, prevAnim));
	addRenderObject(prevAnimation, LR_MENU);

	DebugButton *nextAnimation = new DebugButton(0, 0, 150);
	nextAnimation->label->setText("nextAnim (PGDN)");
	nextAnimation->position = Vector(10, 360+yoffs);
	nextAnimation->event.set(MakeFunctionEvent(AnimationEditor, nextAnim));
	addRenderObject(nextAnimation, LR_MENU);

	DebugButton *copyKey = new DebugButton(0, 0, 150);
	copyKey->label->setText("copyKey");
	copyKey->position = Vector(10, 390+yoffs);
	copyKey->event.set(MakeFunctionEvent(AnimationEditor, copy));
	addRenderObject(copyKey, LR_MENU);

	DebugButton *pasteKey = new DebugButton(0, 0, 150);
	pasteKey->label->setText("pasteKey");
	pasteKey->position = Vector(10, 420+yoffs);
	pasteKey->event.set(MakeFunctionEvent(AnimationEditor, paste));
	addRenderObject(pasteKey, LR_MENU);


	DebugButton *newAnim = new DebugButton(0, 0, 150);
	newAnim->label->setText("NewAnim");
	newAnim->position = Vector(640, 150+yoffs);
	newAnim->event.set(MakeFunctionEvent(AnimationEditor, newAnim));
	addRenderObject(newAnim, LR_MENU);

	DebugButton *tm = new DebugButton(0, 0, 150);
	tm->label->setText("SelMode (M)");
	tm->position = Vector(640, 210+yoffs);
	tm->event.set(MakeFunctionEvent(AnimationEditor, toggleMouseSelection));
	addRenderObject(tm, LR_MENU);

	DebugButton *rb = new DebugButton(0, 0, 150);
	rb->label->setText("ShowJoints (B)");
	rb->position = Vector(640, 240+yoffs);
	rb->event.set(MakeFunctionEvent(AnimationEditor, toggleRenderBorders));
	addRenderObject(rb, LR_MENU);

	DebugButton *sa = new DebugButton(0, 0, 150);
	sa->label->setText("ShowAll (A)");
	sa->position = Vector(640, 270+yoffs);
	sa->event.set(MakeFunctionEvent(AnimationEditor, showAllBones));
	addRenderObject(sa, LR_MENU);

	DebugButton *a4 = new DebugButton(0, 0, 150);
	a4->label->setText("deleteKey");
	a4->position = Vector(640, 340+yoffs);
	a4->event.set(MakeFunctionEvent(AnimationEditor, deleteKey));
	addRenderObject(a4, LR_MENU);

	unitsize = new DebugFont(10, "Unitsize");
	unitsize->position = Vector(650, 400+yoffs);
	addRenderObject(unitsize, LR_MENU);

	DebugButton *unitdown = new DebugButton(0, 0, 70);
	unitdown->label->setText("Down");
	unitdown->position = Vector(640, 420+yoffs);
	unitdown->event.set(MakeFunctionEvent(AnimationEditor, decrTimelineUnit));
	addRenderObject(unitdown, LR_MENU);

	DebugButton *unitup = new DebugButton(0, 0, 70);
	unitup->label->setText("Up");
	unitup->position = Vector(640+80, 420+yoffs);
	unitup->event.set(MakeFunctionEvent(AnimationEditor, incrTimelineUnit));
	addRenderObject(unitup, LR_MENU);

	gridsize = new DebugFont(10, "Gridsize");
	gridsize->position = Vector(650, 450+yoffs);
	addRenderObject(gridsize, LR_MENU);

	DebugButton *griddown = new DebugButton(0, 0, 70);
	griddown->label->setText("Down");
	griddown->position = Vector(640, 470+yoffs);
	griddown->event.set(MakeFunctionEvent(AnimationEditor, decrTimelineGrid));
	addRenderObject(griddown, LR_MENU);

	DebugButton *gridup = new DebugButton(0, 0, 70);
	gridup->label->setText("Up");
	gridup->position = Vector(640+80, 470+yoffs);
	gridup->event.set(MakeFunctionEvent(AnimationEditor, incrTimelineGrid));
	addRenderObject(gridup, LR_MENU);

	DebugButton *save = new DebugButton(0, 0, 150 - smallw - gap);
	save->label->setText("Save");
	save->position = Vector(640, 100+yoffs);
	save->event.set(MakeFunctionEvent(AnimationEditor, saveFile));
	addRenderObject(save, LR_MENU);

	DebugButton *saveall = new DebugButton(0, 0, smallw);
	saveall->label->setText("All");
	saveall->position = save->position + 150/2 + smallw/2 + gap;
	saveall->event.set(MakeFunctionEvent(AnimationEditor, saveAll));
	addRenderObject(saveall, LR_MENU);

	DebugButton *load = new DebugButton(0, 0, 150 - smallw - gap);
	load->label->setText("Reload");
	load->position = Vector(640, 70+yoffs);
	load->event.set(MakeFunctionEvent(AnimationEditor, reloadFile));
	addRenderObject(load, LR_MENU);

	DebugButton *loadall = new DebugButton(0, 0, smallw);
	loadall->label->setText("All");
	loadall->position = load->position + 150/2 + smallw/2 + gap;
	loadall->event.set(MakeFunctionEvent(AnimationEditor, reloadAll));
	addRenderObject(loadall, LR_MENU);

	DebugButton *reverseAnim = new DebugButton(0, 0, 150);
	reverseAnim->label->setText("reverseAnim");
	reverseAnim->position = Vector(10, 480+yoffs);
	reverseAnim->event.set(MakeFunctionEvent(AnimationEditor, reverseAnim));
	addRenderObject(reverseAnim, LR_MENU);

	DebugButton *bAssist = new DebugButton(0, 0, 150);
	bAssist->position = Vector(10, 510+yoffs);
	bAssist->event.set(MakeFunctionEvent(AnimationEditor, toggleSplineMode));
	addRenderObject(bAssist, LR_MENU);
	bSplineAssist = bAssist;

	rect = new Quad("black", Vector(400,300+yoffs));
	rect->alphaMod = 0.2f;
	rect->renderBorder = true;
	rect->renderCenter = false;
	rect->borderAlpha = 5; // HACK to compensate alphaMod
	rect->renderBorderColor = Vector(1,1,1);
	rect->setWidthHeight(400,400);
	addRenderObject(rect, LR_MENU);

	text = new DebugFont();
	text->position = Vector(200,90+yoffs);
	text->setFontSize(6);
	addRenderObject(text, LR_HUD);

	text2 = new DebugFont();
	text2->position = Vector(200,505+yoffs);
	text2->setFontSize(6);
	addRenderObject(text2, LR_HUD);

	toptext = new DebugFont();
	toptext->position = Vector(200,90-20+yoffs);
	toptext->setFontSize(6);
	addRenderObject(toptext, LR_HUD);

	btmtext = new DebugFont();
	btmtext->position = Vector(200,515+yoffs);
	btmtext->setFontSize(6);
	addRenderObject(btmtext, LR_HUD);

	dsq->overlay->alpha.interpolateTo(0, 0.5f);

	rebuildKeyframeWidgets();

	dsq->resetTimer();

	dsq->toggleCursor(true, 0.1f);

	updateTimelineGrid();
	updateTimelineUnit();
	updateButtonLabels();
}

// FIXME: undo should be global?

void AnimationEditor::pushUndo()
{
	std::deque<SkeletalSprite>& undoHistory = pages[curPage].undoHistory;
	undoHistory.push_back(SkeletalSprite());
	undoHistory.back().animations = pages[curPage].editSprite.animations;

	if(undoHistory.size() > 50)
		undoHistory.pop_front();

	pages[curPage].undoEntry = undoHistory.size()-1;
}

void AnimationEditor::undo()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		std::deque<SkeletalSprite>& undoHistory = pages[curPage].undoHistory;
		size_t& undoEntry = pages[curPage].undoEntry;
		if (undoEntry < undoHistory.size())
		{
			getCurrentPageSprite()->animations = undoHistory[undoEntry].animations;
			if(undoEntry > 0) {
				undoEntry--;
			}
		}
	}
}

void AnimationEditor::redo()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		std::deque<SkeletalSprite>& undoHistory = pages[curPage].undoHistory;
		size_t& undoEntry = pages[curPage].undoEntry;

		undoEntry++;
		if (undoEntry < undoHistory.size())
		{
			getCurrentPageSprite()->animations = undoHistory[undoEntry].animations;
		}
		else
		{
			undoEntry --;
		}
	}
}

void AnimationEditor::action(int id, int state, int source, InputDevice device)
{
	if (editingBone && state)
	{
		if (dsq->isNested()) return;

		if (id == ACTION_BONELEFT)
		{
			editingBone->position.x--;
			applyTranslation();
		}
		if (id == ACTION_BONERIGHT)
		{
			editingBone->position.x++;
			applyTranslation();
		}
		if (id == ACTION_BONEUP)
		{
			editingBone->position.y--;
			applyTranslation();
		}
		if (id == ACTION_BONEDOWN)
		{
			editingBone->position.y++;
			applyTranslation();
		}
	}
}

const float ANIM_EDIT_ZOOM = 0.5;
void AnimationEditor::zoomOut()
{
	if (dsq->isNested()) return;

	core->globalScale -= Vector(ANIM_EDIT_ZOOM, ANIM_EDIT_ZOOM);
	core->globalScaleChanged();
}

void AnimationEditor::zoomIn()
{
	if (dsq->isNested()) return;

	core->globalScale += Vector(ANIM_EDIT_ZOOM, ANIM_EDIT_ZOOM);
	core->globalScaleChanged();
}

void AnimationEditor::reorderKeys()
{
	getCurrentPageAnimation()->reorderKeyframes();
	rebuildKeyframeWidgets();
}

void AnimationEditor::rebuildKeyframeWidgets()
{
	Animation *a = getCurrentPageAnimation();
	size_t n = a ? a->getNumKeyframes() : 0;
	pages[curPage].timeline->resizeKeyframes(n);
}

void AnimationEditor::removeState()
{
	delete [] pages;
	pages = NULL;
	StateObject::removeState();
	core->cameraPos = Vector(0,0);
}

void AnimationEditor::toggleMouseSelection()
{
	if (dsq->isNested()) return;

	if (editingBone)
	{
		editingBone->color = Vector(1,1,1);
	}

	mouseSelection = !mouseSelection;
}

void AnimationEditor::moveBoneStripPoint(const Vector &mov)
{
	if (dsq->isNested()) return;

	Bone *sel = editingBone;
	if (sel)
	{
		BoneKeyframe *b = editingBoneSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(sel->boneIdx);
		if (b)
		{
			if (!sel->changeStrip.empty())
			{
				if (b->grid.size() < sel->changeStrip.size())
				{
					b->grid.resize(sel->changeStrip.size());
				}

				Vector v = sel->changeStrip[selectedStripPoint] + mov*0.006f;
				sel->changeStrip[selectedStripPoint] = v;
				b->grid[selectedStripPoint] = v;
			}
		}
	}
}

void AnimationEditor::update(float dt)
{
	StateObject::update(dt);

	const float tltime = getMouseTimelineTime();

	{
		{
			std::ostringstream os;
			for(int i = 0; i < NumPages; ++i)
			{
				if(curPage == i)
					os << "[";
				else
					os << " ";
				os << (i + 1);
				if(curPage == i)
					os << "]";
				else
					os << " ";
			}
			toptext->setText(os.str());
		}


		int pg = editingBonePage >= 0 ? editingBonePage : curPage;

		std::ostringstream os;
		os << "page " << (pg + 1) << " " << pages[pg].editingFile;
		if(Animation *a = getPageSprite(pg)->getCurrentAnimationOrNull())
		{
			os << " anim[" << a->name << "] ";
			os << "currentKey " << currentKey;
			SkeletalKeyframe *k = a->getKeyframe(currentKey);
			if (k)
			{
				os << " keyTime: " << k->t;
			}
		}

		Vector ebdata;
		int pass = 0;
		int origpass = 0;

		if (editingBone)
		{
			os << " bone: " << editingBone->name << " [idx " << editingBone->boneIdx << "]";
			ebdata.x = editingBone->position.x;
			ebdata.y = editingBone->position.y;
			ebdata.z = editingBone->rotation.z;
			pass = editingBone->getRenderPass();
			origpass = editingBone->originalRenderPass;
		}
		text->setText(os.str());

		char t2buf[128];
		sprintf(t2buf, "Bone x: %.3f, y: %.3f, rot: %.3f  strip: %u pass: %d (%d)", ebdata.x, ebdata.y, ebdata.z, (unsigned)selectedStripPoint, pass, origpass);
		text2->setText(t2buf);


		const float t = getAnimTime();
		if(tltime >= 0)
			sprintf(t2buf, "t: %.4f, mouse at %.4f", t, tltime);
		else
			sprintf(t2buf, "t: %.4f", t);
		btmtext->setText(t2buf);
	}

	if (editMode == AE_STRIP)
	{
		if (isActing(ACTION_SWIMLEFT, -1))
			moveBoneStripPoint(Vector(-dt, 0));
		if (isActing(ACTION_SWIMRIGHT, -1))
			moveBoneStripPoint(Vector(dt, 0));

		if (isActing(ACTION_SWIMUP, -1))
			moveBoneStripPoint(Vector(0, -dt));
		if (isActing(ACTION_SWIMDOWN, -1))
			moveBoneStripPoint(Vector(0, dt));
	}

	const bool ctrlPressed = core->getCtrlState();

	for(size_t i = 0; i < NumPages; ++i)
		pages[i].showCenter(ctrlPressed);

	RenderObject *ctrlSprite; // what moving the mouse is currently controlling
	if(splinegrid)
		ctrlSprite = splinegrid;
	else
		ctrlSprite = ctrlPressed ? (RenderObject*)getCurrentPageSprite() : (RenderObject*)spriteRoot;

	if (core->mouse.buttons.middle)
	{
		ctrlSprite->position += core->mouse.change;
	}

	float spd = 1.0f;
	if (core->mouse.scrollWheelChange < 0)
	{
		if(splinegrid && core->getShiftState())
			splinegrid->setPointScale(std::max(splinegrid->getPointScale() / 1.12f, 0.05f));
		else
			ctrlSprite->scale.x /= 1.12f;
	}
	else if (core->mouse.scrollWheelChange > 0)
	{
		if(splinegrid && core->getShiftState())
			splinegrid->setPointScale(splinegrid->getPointScale() * 1.12f);
		else
			ctrlSprite->scale.x *= 1.12f;
	}
	if (core->getKeyState(KEY_PGDN) && core->getShiftState())
	{
		ctrlSprite->scale.x /= (1 + spd*dt);
	}
	if (core->getKeyState(KEY_PGUP) && core->getShiftState())
	{
		ctrlSprite->scale.x *= (1 + spd*dt);
	}
	if (ctrlSprite->scale.x < 0.05f)
	{
		ctrlSprite->scale.x = 0.05f;
	}
	ctrlSprite->scale.y = ctrlSprite->scale.x;

	if (editMode == AE_SELECT)
	{
		updateEditingBone();
	}
	if (editingBone)
	{
		float m = 0.2f;
		if(core->getKeyState(KEY_NUMPADSLASH))
		{
			editingBone->originalScale /= (1 + m*dt);
			editingBone->scale = editingBone->originalScale;
		}
		if(core->getKeyState(KEY_NUMPADSTAR))
		{
			editingBone->originalScale *= (1 + m*dt);
			editingBone->scale = editingBone->originalScale;
		}

		if (editMode == AE_EDITING_MOVE)
		{
			Vector add = core->mouse.change;
			// adjust relative mouse movement to absolute bone rotation
			if (editingBone->getParent())
			{
				float rot = editingBone->getParent()->getAbsoluteRotation().z;
				if (editingBone->getParent()->isfhr())
				{
					add.x = -add.x;
					add.rotate2D360(rot);
				}
				else
					add.rotate2D360(360 - rot);
			}
			editingBone->position += add;
			constrainMouse();
		}
		else if (editMode == AE_EDITING_ROT)
		{
			if (editingBone->getParent() && editingBone->getParent()->isfhr())
				editingBone->rotation.z = rotOffset + (cursorOffset.x - core->mouse.position.x)/2;
			else
				editingBone->rotation.z = rotOffset + (core->mouse.position.x - cursorOffset.x)/2;
			constrainMouse();
		}
	}

	bool doUpdateBones = false;
	if (editMode == AE_SELECT)
	{
		float t = 0;
		bool hastime = false;
		bool mod = false;
		if(core->mouse.buttons.right)
		{
			t = tltime;
			hastime = t >= 0;
			mod = true;
		}
		if (!hastime && !isAnimating())
		{
			SkeletalSprite *editSprite = getCurrentPageSprite();
			if(Animation *a = editSprite->getCurrentAnimationOrNull())
			{
				SkeletalKeyframe *k = a->getKeyframe(currentKey);
				if (k)
				{
					t = k->t;
					hastime = true;
				}
			}
		}

		if(hastime)
		{
			for(size_t i = 0; i < NumPages; ++i)
			{
				SkeletalSprite *spr = getPageSprite(i);
				if(Animation *a = spr->getCurrentAnimationOrNull())
				{
					float len = a->getAnimationLength();
					float tt = t;
					if(len && mod)
						tt = fmodf(tt, len);
					spr->setTime(tt);
				}
			}
			doUpdateBones = true;
		}
	}

	if(splinegrid && editingBone && editMode == AE_SPLINE && splinegrid->wasModified)
	{
		applySplineGridToBone();
		splinegrid->wasModified = false;
	}

	if(doUpdateBones)
	{
		for(size_t i = 0; i < NumPages; ++i)
		{
			SkeletalSprite *spr = getPageSprite(i);
			if(spr->isLoaded() && !spr->isAnimating())
				spr->updateBones();
		}
	}

}

void AnimationEditor::_copyKey()
{
	if (core->getCtrlState())
		copy();
}

void AnimationEditor::copy()
{
 	if (dsq->isNested()) return;

	copyBuffer = *getCurrentPageAnimation()->getKeyframe(currentKey);
}

void AnimationEditor::_pasteKey()
{
	if (core->getCtrlState())
		paste();
}

void AnimationEditor::paste()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		SkeletalKeyframe *k = getCurrentPageAnimation()->getKeyframe(currentKey);
		float time = k->t;
		*k = copyBuffer;
		k->t = time;
	}
}

void AnimationEditor::nextKey()
{
	if (dsq->isNested()) return;

	if (editMode == AE_STRIP)
	{
		selectedStripPoint++;
		if (selectedStripPoint >= editingBone->changeStrip.size()
				&& selectedStripPoint > 0)
			selectedStripPoint --;
	}
	else
	{
		SkeletalSprite *editSprite = getCurrentPageSprite();

		if (core->getCtrlState())
		{
			const std::vector<KeyframeWidget*>& keyframeWidgets = pages[curPage].timeline->getKeyframes();
			for (size_t i = 0; i < keyframeWidgets.size(); i++)
			{
				keyframeWidgets[i]->shiftLeft();
			}
		}
		else if(Animation *a = editSprite->getCurrentAnimation())
		{
			currentKey++;
			SkeletalKeyframe *k = a->getKeyframe(currentKey);
			if (k)
				editSprite->setTime(k->t);
			else
				currentKey --;

			onKeyframeChanged();
		}
	}
}

void AnimationEditor::prevKey()
{
	if (dsq->isNested()) return;

	if (editMode == AE_STRIP)
	{
		if(selectedStripPoint > 0)
			selectedStripPoint--;
	}
	else
	{
		SkeletalSprite *editSprite = getCurrentPageSprite();

		if (core->getCtrlState())
		{
			const std::vector<KeyframeWidget*>& keyframeWidgets = pages[curPage].timeline->getKeyframes();
			for (size_t i = 0; i < keyframeWidgets.size(); i++)
			{
				keyframeWidgets[i]->shiftRight();
			}
		}
		else if(Animation *a = editSprite->getCurrentAnimation())
		{
			if (currentKey > 0)
			{
				currentKey --;
				SkeletalKeyframe *k = a->getKeyframe(currentKey);
				if (k)
					editSprite->setTime(k->t);

				onKeyframeChanged();
			}
		}
	}
}


void AnimationEditor::newAnim()
{
	if (dsq->isNested()) return;

	std::string name = dsq->getUserInputString("NewAnimName", "");
	if (!name.empty())
	{
		Animation anim = *getCurrentPageAnimation();
		anim.name = name;
		SkeletalSprite *spr = getCurrentPageSprite();
		spr->animations.push_back(anim);
		spr->lastAnimation();
	}
}

void AnimationEditor::newKey()
{
	if (dsq->isNested()) return;

	getCurrentPageAnimation()->cloneKey(currentKey, TIMELINE_UNIT);
	currentKey++;
	rebuildKeyframeWidgets();
}

void AnimationEditor::_stopExtraEditModes()
{
	selectedStripPoint = 0;
	editMode = AE_SELECT;
	bgGrad->makeVertical(Vector(0.4f, 0.4f, 0.4f), Vector(0.8f, 0.8f, 0.8f));

	if(splinegrid)
	{
		splinegrid->safeKill();
		splinegrid = NULL;
	}
}

void AnimationEditor::editStripKey()
{
	if (dsq->isNested()) return;

	if (editMode == AE_SPLINE || editMode == AE_STRIP)
	{
		_stopExtraEditModes();
	}
	else
	{
		if(editingBone && editingBone->getGrid())
		{
			DynamicRenderGrid *grid = editingBone->getGrid();
			Animation *a = editingBoneSprite->getCurrentAnimation();
			BoneGridInterpolator *interp = a->getBoneGridInterpolator(editingBone->boneIdx);

			if(interp)
			{
				editMode = AE_SPLINE;
				bgGrad->makeVertical(Vector(0.4f, 0.6f, 0.4f), Vector(0.8f, 1, 0.8f));

				BoneKeyframe *bk = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
				const size_t totalcp = interp->bsp.ctrlX() * interp->bsp.ctrlY();
				const bool reset = bk->controlpoints.empty();
				bk->controlpoints.resize(totalcp);
				assert(!splinegrid);

				splinegrid = new SplineGrid();
				DynamicRenderGrid *rgrid = splinegrid->resize(interp->bsp.ctrlX(), interp->bsp.ctrlY(), grid->width(), grid->height(), interp->bsp.degX(), interp->bsp.degY());
				rgrid->setDrawOrder(grid->getDrawOrder());
				splinegrid->setTexture(editingBone->texture->name);
				splinegrid->setWidthHeight(editingBone->width, editingBone->height);
				splinegrid->position = Vector(400, 300);
				splinegrid->setAssist(assistedSplineEdit);

				if(reset)
					splinegrid->resetControlPoints();
				else
					splinegrid->importKeyframe(bk);

				addRenderObject(splinegrid, LR_PARTICLES_TOP);
			}
			else
			{
				editMode = AE_STRIP;
				bgGrad->makeVertical(Vector(0.4f, 0.4f, 0.6f), Vector(0.8f, 0.8f, 1));
			}

		}
		else if(editingBone)
		{
			notify("Bone has no grid, cannot edit grid");
			dsq->sound->playSfx("denied");
		}
		else
		{
			notify("No bone selected for grid edit mode");
			dsq->sound->playSfx("denied");
		}
	}
}

void AnimationEditor::deleteKey()
{
	if (dsq->isNested()) return;

	if (currentKey > 0)
	{
		getCurrentPageAnimation()->deleteKey(currentKey);
		currentKey --;
	}
	rebuildKeyframeWidgets();
}

void AnimationEditor::animate()
{
	if (dsq->isNested()) return;

	for(size_t i = 0; i < NumPages; ++i)
		if(pages[i].editSprite.isLoaded())
			pages[i].editSprite.playCurrentAnimation(-1);
}

void AnimationEditor::stop()
{
	if (dsq->isNested()) return;

	for(size_t i = 0; i < NumPages; ++i)
		if(pages[i].editSprite.isLoaded())
			pages[i].editSprite.stopAnimation();
}

void AnimationEditor::animateOrStop()
{
	if (dsq->isNested()) return;

	if (core->getShiftState())
		stop();
	else
		animate();
}

void AnimationEditor::lmbd()
{
	if(editMode == AE_SELECT)
	{
		pushUndo();
		updateEditingBone();
		if (editingBone && isMouseInRect() && !editingBoneSprite->isAnimating())
		{
			cursorOffset = editingBone->getWorldPosition() - core->mouse.position;
			editMode = AE_EDITING_MOVE;
		}
	}
}

void AnimationEditor::applyTranslation()
{
	if (editingBone)
	{
		Animation *a = editingBoneSprite->getCurrentAnimation();
		if (!core->getShiftState())
		{
			// one bone mode
			BoneKeyframe *b = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (b)
			{
				b->x = editingBone->position.x;
				b->y = editingBone->position.y;
			}
		}
		else
		{
			BoneKeyframe *bcur = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (bcur)
			{
				int xdiff = editingBone->position.x - bcur->x;
				int ydiff = editingBone->position.y - bcur->y;
				if(!core->getCtrlState())
				{
					// all bones in one anim mode
					for (size_t i = 0; i < a->getNumKeyframes(); ++i)
					{
						BoneKeyframe *b = a->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
						if (b)
						{
							b->x += xdiff;
							b->y += ydiff;
						}
					}
				}
				else
				{
					// all bones in all anims mode
					for (size_t a = 0; a < editingBoneSprite->animations.size(); ++a)
					{
						for (size_t i = 0; i < editingBoneSprite->animations[a].getNumKeyframes(); ++i)
						{
							BoneKeyframe *b = editingBoneSprite->animations[a].getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
							if (b)
							{
								b->x += xdiff;
								b->y += ydiff;
							}
						}
					}
				}
			}
		}
	}
}

void AnimationEditor::applyRotation()
{
	if(editingBone)
	{
		BoneKeyframe *b = editingBoneSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
		if (b)
		{
			b->rot = editingBone->rotation.z = 0;
		}
	}
}

void AnimationEditor::lmbu()
{
	if(editMode == AE_EDITING_MOVE)
	{
		applyTranslation();
		editMode = AE_SELECT;
	}
}

void AnimationEditor::rmbd()
{
	if(editMode == AE_SELECT)
	{
		updateEditingBone();
		if (editingBone && !editingBoneSprite->isAnimating())
		{
			pushUndo();
			cursorOffset = core->mouse.position;
			rotOffset = editingBone->rotation.z;
			editMode = AE_EDITING_ROT;
		}
	}
}

void AnimationEditor::clearRot()
{
	if (dsq->isNested()) return;

	updateEditingBone();
	if (editingBone)
	{
		if(core->getCtrlState())
			core->texmgr.load(editingBone->texture->name, TextureMgr::OVERWRITE);
		else if(splinegrid)
			splinegrid->resetControlPoints();
		else
			applyRotation();
	}
}

void AnimationEditor::flipRot()
{
	if (dsq->isNested()) return;

	updateEditingBone();
	if (editingBone)
	{
		Animation *a = editingBoneSprite->getCurrentAnimation();
		if (!core->getShiftState())
		{
			BoneKeyframe *b = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (b)
			{
				b->rot = -b->rot;
			}
		}
		else
		{
			BoneKeyframe *bcur = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (bcur)
			{
				if (!core->getCtrlState())
				{
					for (size_t i = 0; i < a->getNumKeyframes(); ++i)
					{
						BoneKeyframe *b = a->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
						if (b)
						{
							b->rot = -b->rot;
						}
					}
				}
				else
				{
					// all bones in all anims mode
					for (size_t a = 0; a < editingBoneSprite->animations.size(); ++a)
					{
						for (size_t i = 0; i < editingBoneSprite->animations[a].getNumKeyframes(); ++i)
						{
							BoneKeyframe *b = editingBoneSprite->animations[a].getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
							if (b)
							{
								b->rot = -b->rot;
							}
						}
					}
				}
			}
		}
	}
}

void AnimationEditor::clearPos()
{
	if (dsq->isNested()) return;

	updateEditingBone();
	if (editingBone)
	{
		BoneKeyframe *b = editingBoneSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
		if (b)
		{
			editingBone->position = Vector(0,0);
			b->x = b->y = 0;
		}
	}
}

void AnimationEditor::toggleHideBone()
{
	if (!dsq->isNested())
	{
		updateEditingBone();
		if (editingBone)
		{
			editingBone->renderQuad = !editingBone->renderQuad;
		}
	}
}

void AnimationEditor::rmbu()
{
	if(editMode != AE_EDITING_ROT)
		return;

	editMode = AE_SELECT;
	if (editingBone)
	{
		Animation *a = editingBoneSprite->getCurrentAnimation();
		if (!core->getShiftState())
		{
			// one bone mode
			BoneKeyframe *b = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (b)
			{
				b->rot = int(editingBone->rotation.z);
			}
		}
		else
		{
			BoneKeyframe *bcur = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (bcur)
			{
				int rotdiff = editingBone->rotation.z - bcur->rot;
				if (!core->getCtrlState())
				{
					for (size_t i = 0; i < a->getNumKeyframes(); ++i)
					{
						BoneKeyframe *b = a->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
						if (b)
						{
							b->rot += rotdiff;
						}
					}
				}
				else
				{
					// all bones in all anims mode
					for (size_t a = 0; a < editingBoneSprite->animations.size(); ++a)
					{
						for (size_t i = 0; i < editingBoneSprite->animations[a].getNumKeyframes(); ++i)
						{
							BoneKeyframe *b = editingBoneSprite->animations[a].getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
							if (b)
							{
								b->rot += rotdiff;
							}
						}
					}
				}
			}
		}
	}
}

void AnimationEditor::mmbd()
{


}

void AnimationEditor::cloneBoneAhead()
{
	updateEditingBone();
	if (editingBone && currentKey >= 0)
	{
		Animation *a = editingBoneSprite->getCurrentAnimation();
		SkeletalKeyframe *s1 = a->getKeyframe(currentKey);
		BoneKeyframe *b1 = 0;
		if (s1)
			b1 = s1->getBoneKeyframe(editingBone->boneIdx);

		SkeletalKeyframe *s2 = a->getKeyframe(currentKey+1);
		BoneKeyframe *b2 = 0;
		if (s2)
			b2 = s2->getBoneKeyframe(editingBone->boneIdx);

		if (b1 && b2)
		{
			b2->x = b1->x;
			b2->y = b1->y;
			b2->rot = b1->rot;
			b2->grid = b1->grid;
		}
	}
}

bool AnimationEditor::savePage(size_t pg)
{
	return getPageSprite(pg)->saveSkeletal(pages[pg].editingFile);
}

void AnimationEditor::saveFile()
{
	if(!getPageSprite(curPage)->isLoaded())
	{
		notify("Nothing to save");
		return;
	}

	if(savePage(curPage))
		notify("Saved anim: " + pages[curPage].editingFile);
	else
		notify("FAILED TO SAVE: " + pages[curPage].editingFile);
}

void AnimationEditor::saveAll()
{
	bool ok = true;
	std::ostringstream os;
	for(size_t i = 0; i < NumPages; ++i)
		if(getPageSprite(i)->isLoaded())
			if(!savePage(i))
			{
				ok = false;
				os << pages[i].editingFile << " ";
			}

	if(ok)
		notify("All saved");
	else
		notify("Failed to save: " + os.str());
}

void AnimationEditor::reloadPage(size_t pg)
{
	loadFile(pg, pages[pg].editingFile.c_str());
}

void AnimationEditor::loadFile(size_t pg, const char* fn)
{
	SkeletalSprite::clearCache();
	editingBone = 0;
	editingBoneIdx = -1;
	pages[pg].clearUndoHistory();
	//editSprite->position = Vector(0,0);
	pages[pg].load(fn);
	currentKey = 0;
	rebuildKeyframeWidgets();

	// disable strip edit mode if still active
	_stopExtraEditModes();

	onKeyframeChanged();
}

void AnimationEditor::reloadFile()
{
	if(getPageSprite(curPage)->isLoaded())
		reloadPage(curPage);
	else
		load(); // prompt what to load here
}

void AnimationEditor::reloadAll()
{
	for(size_t i = 0; i < NumPages; ++i)
		reloadPage(i);
}

void AnimationEditor::goToTitle()
{
	if (dsq->isNested()) return;

	if (!dsq->returnToScene.empty())
		game->transitionToScene(dsq->returnToScene);
	else
		dsq->title(false);
}

void AnimationEditor::quit()
{
	core->quit();
}

void AnimationEditor::nextAnim()
{
	if (dsq->isNested()) return;
	if (core->getShiftState()) return;

	if(editMode == AE_SELECT)
	{
		getCurrentPageSprite()->nextAnimation();
		currentKey = 0;
		rebuildKeyframeWidgets();
	}
}

void AnimationEditor::prevAnim()
{
	if (dsq->isNested()) return;
	if (core->getShiftState()) return;

	if(editMode == AE_SELECT)
	{
		getCurrentPageSprite()->prevAnimation();
		currentKey = 0;
		rebuildKeyframeWidgets();
	}
}

void AnimationEditor::selectAnim()
{
	if (dsq->isNested()) return;

	std::string name = dsq->getUserInputString("Select anim name:");
	if (name.empty())
		return;

	if(getCurrentPageSprite()->selectAnimation(name.c_str()))
	{
		currentKey = 0;
		rebuildKeyframeWidgets();
	}
	else
		notify("No such anim name");
}

void AnimationEditor::reverseAnim()
{
	if (dsq->isNested()) return;

	Animation *a = getCurrentPageAnimation();
	if (a)
	{
		debugLog("calling reverse anim");
		a->reverse();
		rebuildKeyframeWidgets();
	}
}

void AnimationEditor::flipH()
{
	if (dsq->isNested()) return;

	RenderObject *ro = core->getCtrlState() ? (RenderObject*)getSelectedPageSprite() : (RenderObject*)spriteRoot;
	ro->flipHorizontal();

	const Vector red(1,0,0), white(1,1,1);
	toptext->color = spriteRoot->isfh() ? red : white;
	for(size_t i = 0; i < NumPages; ++i)
		pages[i].timeline->label.color = pages[i].editSprite.isfhr() ? red : white;
}

void AnimationEditor::load()
{
	if (dsq->isNested()) return;

	std::string file = dsq->getUserInputString("Enter anim file to load:");
	if (file.empty())		return;
	loadFile(curPage, file.c_str());
}

void AnimationEditor::loadSkin()
{
	if (dsq->isNested()) return;

	std::string file = dsq->getUserInputString("Enter skin file to load:");
	if (file.empty())		return;


	SkeletalSprite::clearCache();
	getCurrentPageSprite()->loadSkin(file);
}

void AnimationEditor::moveNextWidgets(float dt)
{
	if (dsq->isNested()) return;

	bool move = false;
	const std::vector<KeyframeWidget*>& keyframeWidgets = pages[curPage].timeline->getKeyframes();
	Animation *a = getCurrentPageAnimation();
	for (size_t i = 0; i < keyframeWidgets.size(); i++)
	{
		KeyframeWidget *w = keyframeWidgets[i];
		if (move)
		{
			a->getKeyframe(w->key)->t += dt;
		}
		else if (KeyframeWidget::movingWidget == w)
		{
			move = true;
		}
	}

}

void AnimationEditor::toggleRenderBorders()
{
	if (dsq->isNested()) return;

	renderBorderMode = (RenderBorderMode)(renderBorderMode + 1);
	if(renderBorderMode > RENDER_BORDER_ALL)
		renderBorderMode = RENDER_BORDER_NONE;
	updateRenderBorders();
}

void AnimationEditor::updateRenderBorders()
{
	SkeletalSprite *editSprite = getCurrentPageSprite();
	if (!editSprite)
		return;

	// reset
	for (size_t i = 0; i < editSprite->bones.size(); ++i)
	{
		Bone *b = editSprite->bones[i];
		b->renderBorder = false;
		b->renderCenter = false;
		b->borderAlpha = 0.8f;
		b->renderBorderColor = Vector(1,1,1);
	}

	if(renderBorderMode == RENDER_BORDER_NONE)
		return;
	else if(Animation *a = editSprite->getCurrentAnimation())
	{
		for(size_t i = 0; i < a->interpolators.size(); ++i)
		{
			const BoneGridInterpolator& bgip = a->interpolators[i];
			if(Bone *b = editSprite->getBoneByIdx(bgip.idx))
			{
				b->renderBorder = true;
				b->renderCenter = true;
				b->borderAlpha = 0.4f;
				b->renderBorderColor = Vector(0.2f, 0.9f, 0.2f);
			}
		}
	}

	if(renderBorderMode == RENDER_BORDER_ALL)
	{
		for (size_t i = 0; i < editSprite->bones.size(); ++i)
		{
			Bone *b = editSprite->bones[i];
			b->renderBorder = true;
			b->renderCenter = true;
		}
	}
}

// Pick the closest bone
void AnimationEditor::updateEditingBone()
{
	if(editMode != AE_SELECT)
		return;

	if(!mouseSelection)
	{
		if(!editingBoneSprite)
			editingBoneSprite = getCurrentPageSprite();
		editingBonePage = curPage;
		Bone *b = (size_t)editingBoneIdx < editingBoneSprite->bones.size() ? editingBoneSprite->bones[editingBoneIdx] : NULL; // this uses underflow at -1
		if(b && b->selectable)
			_selectBone(b);
		else
			selectNextBone();
		return;
	}

	// When selecting with the mouse, pick the closest bone to the cursor, no matter the skeleton it belongs to
	float mind;
	Bone *nearest = NULL;
	SkeletalSprite *nearestSpr = NULL;
	const Vector& p = core->mouse.position;
	int page = -1, idx = -1;
	for(size_t i = 0; i < NumPages; ++i)
	{
		SkeletalSprite& spr = pages[i].editSprite;
		if(spr.isLoaded())
		{
			int k = spr.findSelectableBoneIdxClosestTo(p, true);
			if(k >= 0)
			{
				Bone *b = spr.bones[k];
				const float d = (b->getWorldPosition() - p).getSquaredLength2D();
				if(!nearest || d < mind)
				{
					mind = d;
					nearest = b;
					nearestSpr = &spr;
					page = i;
					idx = k;
				}
			}
		}
	}

	editingBoneSprite = nearestSpr;
	editingBonePage = page;
	editingBoneIdx = idx;
	_selectBone(nearest);
}

void AnimationEditor::showAllBones()
{
	if (dsq->isNested()) return;

	SkeletalSprite *spr = getSelectedPageSprite();
	for (size_t i = 0; i < spr->bones.size(); ++i)
		spr->bones[i]->renderQuad = true;
}

void AnimationEditor::incrTimelineUnit()
{
	if (dsq->isNested()) return;

	TIMELINE_UNIT += TIMELINE_UNIT_STEP;
	updateTimelineUnit();
}

void AnimationEditor::decrTimelineUnit()
{
	if (dsq->isNested()) return;

	float t = TIMELINE_UNIT - TIMELINE_UNIT_STEP;
	if (t >= TIMELINE_UNIT_STEP)
		TIMELINE_UNIT = t;
	updateTimelineUnit();
}

void AnimationEditor::updateTimelineUnit()
{
	std::ostringstream os;
	os << "Unit: " << TIMELINE_UNIT;
	unitsize->setText(os.str());
}

void AnimationEditor::incrTimelineGrid()
{
	if (dsq->isNested()) return;

	TIMELINE_GRIDSIZE++;
	updateTimelineGrid();
}

void AnimationEditor::decrTimelineGrid()
{
	if (dsq->isNested()) return;

	int t = TIMELINE_GRIDSIZE - 1;
	if (t > 0)
		TIMELINE_GRIDSIZE = t;
	updateTimelineGrid();
}

void AnimationEditor::toggleSplineMode()
{
	if (dsq->isNested()) return;

	assistedSplineEdit = !assistedSplineEdit;
	updateButtonLabels();
	if(splinegrid)
		splinegrid->setAssist(assistedSplineEdit);
}

void AnimationEditor::updateButtonLabels()
{
	{
		std::ostringstream os;
		os << "S.Assist (W)(" << (assistedSplineEdit ? "on" : "off") << ")";
		bSplineAssist->label->setText(os.str());
	}
}

void AnimationEditor::toggleGradient()
{
	if (dsq->isNested()) return;

	bgGrad->alpha.x = float(bgGrad->alpha.x <= 0);
}

float AnimationEditor::getMouseTimelineTime() const
{
	Vector m = core->mouse.position;
	if(m.x > 0 && m.x < 800 && m.y > TIMELINE_CENTER_Y-TIMELINE_HEIGHT/2 && m.y < TIMELINE_CENTER_Y+TIMELINE_HEIGHT/2)
	{
		float t = (m.x - TIMELINE_X_OFFS) * TIMELINE_UNIT / TIMELINE_GRIDSIZE;
		t = std::max(t, 0.0f);
		return t;
	}

	return -1;
}

Animation* AnimationEditor::getPageAnimation(size_t page) const
{
	return pages[page].editSprite.getCurrentAnimation();
}

Animation* AnimationEditor::getCurrentPageAnimation() const
{
	return getPageAnimation(curPage);
}

SkeletalSprite* AnimationEditor::getPageSprite(size_t page) const
{
	return &pages[page].editSprite;
}

SkeletalSprite* AnimationEditor::getCurrentPageSprite() const
{
	return getPageSprite(curPage);
}

SkeletalSprite * AnimationEditor::getSelectedPageSprite() const
{
	return editingBoneSprite ? editingBoneSprite : getCurrentPageSprite();
}

bool AnimationEditor::isAnimating() const
{
	for(size_t i = 0; i < NumPages; ++i)
		if(getPageSprite(i)->isAnimating())
			return true;
	return false;
}

float AnimationEditor::getAnimTime() const
{
	SkeletalSprite *cur = getCurrentPageSprite();
	if(cur->isLoaded() && cur->isAnimating())
		return cur->getAnimationLayer(0)->timer;

	for(size_t i = 0; i < NumPages; ++i)
	{
		SkeletalSprite *spr = getPageSprite(i);
		if(spr->isLoaded() && spr->isAnimating())
			return spr->getAnimationLayer(0)->timer;
	}

	if(Animation *a = getCurrentPageSprite()->getCurrentAnimationOrNull())
	{
		SkeletalKeyframe *k = a->getKeyframe(currentKey);
		if(k)
			return k->t;
	}

	return 0;
}

void AnimationEditor::selectPage(unsigned page)
{
	if (dsq->isNested()) return;

	if(editMode != AE_SELECT)
		return;

	if(!mouseSelection)
	{
		editingBoneSprite = &pages[page].editSprite;
		updateEditingBone();
	}

	curPage = page;
}

void AnimationEditor::updateTimelineGrid()
{
	std::ostringstream os;
	os << "Grid: " << TIMELINE_GRIDSIZE;
	gridsize->setText(os.str());
}

void AnimationEditor::onKeyframeChanged()
{
	applyBoneToSplineGrid();

	updateRenderBorders(); // restore default state
}

void AnimationEditor::applyBoneToSplineGrid()
{
	if(splinegrid && editingBone)
	{
		Animation *a = editingBoneSprite->getCurrentAnimation();
		BoneKeyframe *bk = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);

		assert(bk->grid.size() == editingBone->getGrid()->linearsize());
		splinegrid->importKeyframe(bk);
	}
}

void AnimationEditor::applySplineGridToBone()
{
	if(splinegrid && editingBone)
	{
		Animation *a = editingBoneSprite->getCurrentAnimation();
		BoneKeyframe *bk = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
		assert(bk->grid.size() == editingBone->getGrid()->linearsize());
		splinegrid->exportKeyframe(bk);
		BoneGridInterpolator *interp = a->getBoneGridInterpolator(editingBone->boneIdx);
		interp->updateGridAndBone(*bk, editingBone);
	}
}

void AnimationEditor::_selectBone(Bone *b)
{
	if(editingBone)
		editingBone->color = Vector(1,1,1);

	if (b)
		b->color = mouseSelection ? Vector(1,0,0) : Vector(0.5,0.5,1);
	else
	{
		editingBonePage = -1;
		editingBoneSprite = NULL;
		editingBoneIdx = -1;
	}

	editingBone = b;
}

void AnimationEditor::selectPrevBone()
{
	if (dsq->isNested() || mouseSelection || editMode != AE_SELECT)
		return;

	SkeletalSprite *spr = editingBoneSprite ? editingBoneSprite : getCurrentPageSprite();
	if(spr->bones.empty())
		return;

	size_t idx = editingBoneIdx;
	Bone *b = NULL;
	do
	{
		idx++;
		if(idx == editingBoneIdx)
		{
			idx = -1;
			break;
		}
		if (idx >= spr->bones.size())
			idx  = 0;
		b = spr->bones[idx];
	}
	while (!b->selectable);
	editingBoneIdx = idx;
	editingBonePage = curPage;
	editingBoneSprite = spr;
	_selectBone(b);
}

void AnimationEditor::selectNextBone()
{
	if (dsq->isNested() || mouseSelection || editMode != AE_SELECT)
		return;

	SkeletalSprite *spr = editingBoneSprite ? editingBoneSprite : getCurrentPageSprite();
	if(spr->bones.empty())
		return;

	size_t idx = editingBoneIdx;
	Bone *b = NULL;
	do
	{
		idx--;
		if(idx == editingBoneIdx)
		{
			idx = -1;
			break;
		}
		if (idx >= spr->bones.size())
			idx = spr->bones.size()-1;
		b = spr->bones[idx];
	}
	while (!b->selectable);
	editingBoneIdx = idx;
	editingBonePage = curPage;
	editingBoneSprite = spr;
	_selectBone(b);
}

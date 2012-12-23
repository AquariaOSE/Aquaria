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
#include "States.h"
#include "AquariaMenuItem.h"
#include "../BBGE/Gradient.h"
#include "../BBGE/DebugFont.h"
#include "Game.h"


#ifdef AQUARIA_BUILD_SCENEEDITOR  // Through end of file


int TIMELINE_GRIDSIZE		= 10;
float TIMELINE_UNIT			= 0.1;
float TIMELINE_UNIT_STEP	= 0.01;
const int KEYFRAME_POS_Y	= 570;

class TimelineRender : public RenderObject
{
	void onRender()
	{
#ifdef BBGE_BUILD_OPENGL
		glLineWidth(1);
		glBegin(GL_LINES);
		glColor4f(1, 1, 1, 1);
		for (int x = 0; x < 800; x += TIMELINE_GRIDSIZE)
		{
			glVertex3f(x, -5, 0);
			glVertex3f(x, 5, 0);
		}
		glEnd();
#endif
	}
};

AnimationEditor *ae = 0;

KeyframeWidget *KeyframeWidget::movingWidget = 0;

Gradient *bgGrad = 0;

int keyframeOffset = 0;

Bone *lastSelectedBone = 0;

void AnimationEditor::constrainMouse()
{
	Vector mp=core->mouse.position;
	bool doit = false;
	if (mp.x < 200)	{ mp.x = 200; doit = true; }
	if (mp.x > 600)	{ mp.x = 600; doit = true; }
	if (mp.y < 100)	{ mp.y = 100; doit = true; }
	if (mp.y > 500)	{ mp.y = 500; doit = true; }

	if(doit)
		core->setMousePosition(mp);
}

KeyframeWidget::KeyframeWidget(int key) : Quad()
{
	setTexture("keyframe");
	setWidthHeight(15, 30);
	b = new BitmapText(&dsq->smallFont);
	b->position = Vector(1, -15);
	b->setFontSize(12);
	addChild(b, PM_POINTER);
	this->key = key;
	ae->keyframeWidgets.push_back(this);
}

void KeyframeWidget::shiftLeft()
{
	if (!offset.isInterpolating())
		offset.interpolateTo(Vector(offset.x-80, 0), 0.1, 0, 0, 0);
}

void KeyframeWidget::shiftRight()
{
	if (!offset.isInterpolating())
		offset.interpolateTo(Vector(offset.x+80, 0), 0.1, 0, 0, 0);
}

void KeyframeWidget::onUpdate(float dt)
{
	/*
		if (this->key == ae->currentKey)
			color = Vector(0.75, 0.75, 1);
		else
	*/
	Quad::onUpdate(dt);
	if (life != 1 || ae->editSprite->isAnimating()) return;
	switch(ae->editSprite->getCurrentAnimation()->getKeyframe(this->key)->lerpType)
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

	if (!offset.isInterpolating())
	{
		if (this->key == ae->currentKey)
			offset.y = -12;
		else
			offset.y = 0;
	}

	std::ostringstream os;
	os << key;
	b->setText(os.str());

	if (!movingWidget && isCoordinateInside(core->mouse.position))
	{
		if (core->mouse.buttons.left)
		{
			//ae->selectionLocked = false;
			movingWidget = this;
			ae->currentKey = this->key;
		}
	}
	if (movingWidget == this)
	{
		float lastT = ae->editSprite->getCurrentAnimation()->getKeyframe(this->key)->t;
		this->position.x = int((core->mouse.position.x-offset.x)/TIMELINE_GRIDSIZE)*TIMELINE_GRIDSIZE+TIMELINE_GRIDSIZE/2;
		float newT = int(this->position.x/TIMELINE_GRIDSIZE)*TIMELINE_UNIT;
		ae->editSprite->getCurrentAnimation()->getKeyframe(this->key)->t = newT;
		if (core->getShiftState())
		{
			ae->moveNextWidgets(newT-lastT);
		}
	}
	else
	{
		this->position.x = ae->editSprite->getCurrentAnimation()->getKeyframe(this->key)->t*TIMELINE_GRIDSIZE*(1/TIMELINE_UNIT) + TIMELINE_GRIDSIZE/2;
	}

	if (movingWidget == this && !core->mouse.buttons.left)
	{
		movingWidget = 0;
		ae->reorderKeys();
		return;
	}
	position.y = KEYFRAME_POS_Y;
}

void AnimationEditor::cycleLerpType()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{

		Animation *a = ae->editSprite->getCurrentAnimation();
		if (a->getNumKeyframes() >= 2)
		{
			pushUndo();

			SkeletalKeyframe *k1 = a->getFirstKeyframe();
			SkeletalKeyframe *k2 = a->getLastKeyframe();
			if (k1 && k2)
			{
				k2->copyAllButTime(k1);
			}
			dsq->screenMessage("Copied Loop Key");

		}
	}
	else
	{
		pushUndo();
		int *lt = &ae->editSprite->getCurrentAnimation()->getKeyframe(this->currentKey)->lerpType;
		(*lt)++;
		if (*lt > 3)
			*lt = 0;
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
	else
		editSprite->scale = Vector(1,1);
}

void AnimationEditor::applyState()
{
	dsq->toggleCursor(true, 0.1);
	core->cameraPos = Vector(0,0);
	editingStrip = false;
	selectedStripPoint = 0;
	mouseSelection = true;
	editingFile = "Naija";
	renderBorders = false;
	ae = this;
	StateObject::applyState();
	boneEdit = 0;
	editingBone = 0;

	currentKey = 0;

	editSprite = new SkeletalSprite();
	editSprite->cull = false;
	editSprite->loadSkeletal(editingFile);
	editSprite->position = Vector(400,300);
	//editSprite->scale = Vector(0.5, 0.5);

	addAction(MakeFunctionEvent(AnimationEditor, lmbu), ActionMapper::MOUSE_BUTTON_LEFT, 0);
	addAction(MakeFunctionEvent(AnimationEditor, lmbd), ActionMapper::MOUSE_BUTTON_LEFT, 1);
	addAction(MakeFunctionEvent(AnimationEditor, rmbu), ActionMapper::MOUSE_BUTTON_RIGHT, 0);
	addAction(MakeFunctionEvent(AnimationEditor, rmbd), ActionMapper::MOUSE_BUTTON_RIGHT, 1);
	addAction(MakeFunctionEvent(AnimationEditor, mmbd), ActionMapper::MOUSE_BUTTON_MIDDLE, 1);


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
	addAction(MakeFunctionEvent(AnimationEditor, toggleHideBone), KEY_N, 0);
	addAction(MakeFunctionEvent(AnimationEditor, copy), KEY_C, 0);
	addAction(MakeFunctionEvent(AnimationEditor, paste), KEY_V, 0);

	addAction(MakeFunctionEvent(AnimationEditor, undo), KEY_Z, 0);
	addAction(MakeFunctionEvent(AnimationEditor, redo), KEY_Y, 0);

	//addAction(MakeFunctionEvent(AnimationEditor, lockSelection), KEY_L, 0);
	addAction(MakeFunctionEvent(AnimationEditor, cycleLerpType), KEY_L, 0);

	addAction(MakeFunctionEvent(AnimationEditor, selectPrevBone), KEY_UP, 0);
	addAction(MakeFunctionEvent(AnimationEditor, selectNextBone), KEY_DOWN, 0);

	addAction(MakeFunctionEvent(AnimationEditor, editStripKey), KEY_E, 0);

	addAction(MakeFunctionEvent(AnimationEditor, prevAnim), KEY_PGUP, 0);
	addAction(MakeFunctionEvent(AnimationEditor, nextAnim), KEY_PGDN, 0);
	addAction(MakeFunctionEvent(AnimationEditor, animateOrStop), KEY_RETURN, 0);

	addAction(MakeFunctionEvent(AnimationEditor, toggleRenderBorders), KEY_B, 0);
	addAction(MakeFunctionEvent(AnimationEditor, toggleMouseSelection), KEY_M, 0);
	addAction(MakeFunctionEvent(AnimationEditor, showAllBones), KEY_A, 0);

	addAction(MakeFunctionEvent(AnimationEditor, decrTimelineUnit), KEY_U, 0);
	addAction(MakeFunctionEvent(AnimationEditor, incrTimelineUnit), KEY_I, 0);
	addAction(MakeFunctionEvent(AnimationEditor, decrTimelineGrid), KEY_O, 0);
	addAction(MakeFunctionEvent(AnimationEditor, incrTimelineGrid), KEY_P, 0);



	/*
	addAction("mbl", KEY_A);
	addAction("mbr", KEY_D);
	addAction("mbu", KEY_W);
	addAction("mbd", KEY_S);
	*/


	addAction(ACTION_SWIMLEFT,	KEY_J);
	addAction(ACTION_SWIMRIGHT, KEY_K);
	addAction(ACTION_SWIMUP,	KEY_UP);
	addAction(ACTION_SWIMDOWN,	KEY_DOWN);

	/*
	addAction(ACTION_BONELEFT,		KEY_NUMPAD4);
	addAction(ACTION_BONERIGHT,		KEY_NUMPAD6);
	addAction(ACTION_BONEUP,		KEY_NUMPAD8);
	addAction(ACTION_BONEDOWN,		KEY_NUMPAD2);
	*/

	//addAction("", );

	/*
	addAction(MakeFunctionEvent(AnimationEditor, zoomOut), KEY_NUMPAD2, 0);
	addAction(MakeFunctionEvent(AnimationEditor, zoomIn), KEY_NUMPAD8, 0);
	*/

	addRenderObject(editSprite, LR_ENTITIES);

	Quad *back = new Quad;
	{
		back->color = 0;
		back->setWidthHeight(800, 600);
		back->position = Vector(400,300, -0.2);
	}
	addRenderObject(back, LR_BACKDROP);

	bgGrad = new Gradient;
	bgGrad->scale = Vector(800, 600);
	bgGrad->position = Vector(400,300);
	bgGrad->makeVertical(Vector(0.4, 0.4, 0.4), Vector(0.8, 0.8, 0.8));
	addRenderObject(bgGrad, LR_BACKDROP);

	DebugButton *a = new DebugButton(0, 0, 150);
	a->position = Vector(10, 60);
	a->label->setText("prevKey  (LEFT)");
	a->event.set(MakeFunctionEvent(AnimationEditor, prevKey));
	addRenderObject(a, LR_HUD);

	DebugButton *a2 = new DebugButton(0, 0, 150);
	a2->position = Vector(10, 90);
	a2->label->setText("nextKey (RIGHT)");
	a2->event.set(MakeFunctionEvent(AnimationEditor, nextKey));
	addRenderObject(a2, LR_HUD);

	DebugButton *a3 = new DebugButton(0, 0, 150);
	a3->position = Vector(10, 120);
	a3->label->setText("cloneKey");
	a3->event.set(MakeFunctionEvent(AnimationEditor, newKey));
	addRenderObject(a3, LR_HUD);

	DebugButton *animate = new DebugButton(0, 0, 150);
	animate->position = Vector(10, 200);
	animate->label->setText("animate (ENTER)");
	animate->event.set(MakeFunctionEvent(AnimationEditor, animate));
	addRenderObject(animate, LR_HUD);

	DebugButton *stop = new DebugButton(0, 0, 150);
	stop->position = Vector(10, 230);
	stop->label->setText("stop  (S-ENTER)");
	stop->event.set(MakeFunctionEvent(AnimationEditor, stop));
	addRenderObject(stop, LR_HUD);

	DebugButton *prevAnimation = new DebugButton(0, 0, 150);
	prevAnimation->label->setText("prevAnim (PGUP)");
	prevAnimation->position = Vector(10, 330);
	prevAnimation->event.set(MakeFunctionEvent(AnimationEditor, prevAnim));
	addRenderObject(prevAnimation, LR_MENU);

	DebugButton *nextAnimation = new DebugButton(0, 0, 150);
	nextAnimation->label->setText("nextAnim (PGDN)");
	nextAnimation->position = Vector(10, 360);
	nextAnimation->event.set(MakeFunctionEvent(AnimationEditor, nextAnim));
	addRenderObject(nextAnimation, LR_MENU);

	DebugButton *copyKey = new DebugButton(0, 0, 150);
	copyKey->label->setText("copyKey");
	copyKey->position = Vector(10, 390);
	copyKey->event.set(MakeFunctionEvent(AnimationEditor, copyKey));
	addRenderObject(copyKey, LR_MENU);

	DebugButton *pasteKey = new DebugButton(0, 0, 150);
	pasteKey->label->setText("pasteKey");
	pasteKey->position = Vector(10, 420);
	pasteKey->event.set(MakeFunctionEvent(AnimationEditor, pasteKey));
	addRenderObject(pasteKey, LR_MENU);


	DebugButton *newAnim = new DebugButton(0, 0, 150);
	newAnim->label->setText("NewAnim");
	newAnim->position = Vector(640, 150);
	newAnim->event.set(MakeFunctionEvent(AnimationEditor, newAnim));
	addRenderObject(newAnim, LR_MENU);

	DebugButton *tm = new DebugButton(0, 0, 150);
	tm->label->setText("SelMode (M)");
	tm->position = Vector(640, 210);
	tm->event.set(MakeFunctionEvent(AnimationEditor, toggleMouseSelection));
	addRenderObject(tm, LR_MENU);

	DebugButton *rb = new DebugButton(0, 0, 150);
	rb->label->setText("ShowJoints (B)");
	rb->position = Vector(640, 240);
	rb->event.set(MakeFunctionEvent(AnimationEditor, toggleRenderBorders));
	addRenderObject(rb, LR_MENU);

	DebugButton *sa = new DebugButton(0, 0, 150);
	sa->label->setText("ShowAll (A)");
	sa->position = Vector(640, 270);
	sa->event.set(MakeFunctionEvent(AnimationEditor, showAllBones));
	addRenderObject(sa, LR_MENU);

	DebugButton *a4 = new DebugButton(0, 0, 150);
	a4->label->setText("deleteKey");
	a4->position = Vector(640, 340);
	a4->event.set(MakeFunctionEvent(AnimationEditor, deleteKey));
	addRenderObject(a4, LR_MENU);

	unitsize = new DebugFont(10, "Unitsize");
	unitsize->position = Vector(650, 400);
	addRenderObject(unitsize, LR_MENU);

	DebugButton *unitdown = new DebugButton(0, 0, 70);
	unitdown->label->setText("Down");
	unitdown->position = Vector(640, 420);
	unitdown->event.set(MakeFunctionEvent(AnimationEditor, decrTimelineUnit));
	addRenderObject(unitdown, LR_MENU);

	DebugButton *unitup = new DebugButton(0, 0, 70);
	unitup->label->setText("Up");
	unitup->position = Vector(640+80, 420);
	unitup->event.set(MakeFunctionEvent(AnimationEditor, incrTimelineUnit));
	addRenderObject(unitup, LR_MENU);

	gridsize = new DebugFont(10, "Gridsize");
	gridsize->position = Vector(650, 450);
	addRenderObject(gridsize, LR_MENU);

	DebugButton *griddown = new DebugButton(0, 0, 70);
	griddown->label->setText("Down");
	griddown->position = Vector(640, 470);
	griddown->event.set(MakeFunctionEvent(AnimationEditor, decrTimelineGrid));
	addRenderObject(griddown, LR_MENU);

	DebugButton *gridup = new DebugButton(0, 0, 70);
	gridup->label->setText("Up");
	gridup->position = Vector(640+80, 470);
	gridup->event.set(MakeFunctionEvent(AnimationEditor, incrTimelineGrid));
	addRenderObject(gridup, LR_MENU);

	DebugButton *save = new DebugButton(0, 0, 150);
	save->label->setText("Save");
	save->position = Vector(640, 100);
	save->event.set(MakeFunctionEvent(AnimationEditor, saveFile));
	addRenderObject(save, LR_MENU);

	DebugButton *load = new DebugButton(0, 0, 150);
	load->label->setText("Reload");
	load->position = Vector(640, 50);
	load->event.set(MakeFunctionEvent(AnimationEditor, loadFile));
	addRenderObject(load, LR_MENU);

	DebugButton *reverseAnim = new DebugButton(0, 0, 150);
	reverseAnim->label->setText("reverseAnim");
	reverseAnim->position = Vector(10, 480);
	reverseAnim->event.set(MakeFunctionEvent(AnimationEditor, reverseAnim));
	addRenderObject(reverseAnim, LR_MENU);


	OutlineRect *rect = new OutlineRect;
	rect->setWidthHeight(400,400);
	rect->position = Vector(400,300);
	addRenderObject(rect, LR_MENU);

	text = new DebugFont();
	text->position = Vector(200,90);
	text->setFontSize(6);
	addRenderObject(text, LR_HUD);

	text2 = new DebugFont();
	text2->position = Vector(200,510);
	text2->setFontSize(6);
	addRenderObject(text2, LR_HUD);

	TimelineRender *tr = new TimelineRender();
	tr->position = Vector(0, KEYFRAME_POS_Y);
	addRenderObject(tr, LR_BLACKGROUND);

	editSprite->setSelectedBone(0);

	dsq->overlay->alpha.interpolateTo(0, 0.5);

	rebuildKeyframeWidgets();

	dsq->resetTimer();

	dsq->toggleCursor(true, 0.1);

	updateTimelineGrid();
	updateTimelineUnit();
}

void AnimationEditor::clearUndoHistory()
{
	undoHistory.clear();
}

void AnimationEditor::pushUndo()
{
	SkeletalSprite sk;
	sk.animations = editSprite->animations;
	undoHistory.push_back(sk);
	undoEntry = undoHistory.size()-1;
}

void AnimationEditor::undo()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		if (undoEntry >= 0 && undoEntry < undoHistory.size())
		{
			editSprite->animations = undoHistory[undoEntry].animations;
			undoEntry--;
			if (undoEntry<0) undoEntry = 0;
		}
	}
}

void AnimationEditor::redo()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		undoEntry++;
		if (undoEntry >= 0 && undoEntry < undoHistory.size())
		{

			editSprite->animations = undoHistory[undoEntry].animations;
		}
		else
		{
			undoEntry --;
		}
	}
}

void AnimationEditor::action(int id, int state)
{
	StateObject::action(id, state);
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
}

void AnimationEditor::zoomIn()
{
	if (dsq->isNested()) return;

	core->globalScale += Vector(ANIM_EDIT_ZOOM, ANIM_EDIT_ZOOM);
}

void AnimationEditor::reorderKeys()
{
	editSprite->getCurrentAnimation()->reorderKeyframes();
	rebuildKeyframeWidgets();
}

void AnimationEditor::rebuildKeyframeWidgets()
{
	int offx=0;
	for (int i = 0; i < keyframeWidgets.size(); i++)
	{
		keyframeWidgets[i]->setLife(0.03);
		keyframeWidgets[i]->setDecayRate(1);
		offx = keyframeWidgets[i]->offset.x;
	}
	keyframeWidgets.clear();
	for (int i = 0; i < 1000; i++)
	{
		if (editSprite->getCurrentAnimation())
		{
			SkeletalKeyframe *key = editSprite->getCurrentAnimation()->getKeyframe(i);
			if (!key) break;
			KeyframeWidget *k = new KeyframeWidget(i);
			k->offset.x = offx;
			addRenderObject(k, LR_HUD);
			keyframeWidgets.push_back(k);
		}
	}
}

void AnimationEditor::removeState()
{
	keyframeWidgets.clear();
	StateObject::removeState();
	core->cameraPos = Vector(0,0);
}

void AnimationEditor::toggleMouseSelection()
{
	if (dsq->isNested()) return;

	if (editSprite && mouseSelection)
		editSprite->updateSelectedBoneColor();

	mouseSelection = !mouseSelection;
}

void AnimationEditor::moveBoneStripPoint(const Vector &mov)
{
	if (dsq->isNested()) return;

	Bone *sel = editSprite->getSelectedBone(false);
	if (sel)
	{
		BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(sel->boneIdx);
		if (b)
		{
			if (!sel->changeStrip.empty())
			{
				if (b->strip.size() < sel->changeStrip.size())
				{
					b->strip.resize(sel->changeStrip.size());
				}

				b->strip[selectedStripPoint] = sel->changeStrip[selectedStripPoint] += mov*0.006f;
				sel->setGridPoints(sel->stripVert, sel->strip);
				/*


				float sz = sel->getStripSegmentSize();
				for (int i = selectedStripPoint; i > 0; i--)
				{
					Vector diff = sel->changeStrip[i] - sel->changeStrip[i-1];
					if (!diff.isLength2DIn(sz))
					{
						diff.setLength2D(sz);
						sel->changeStrip[i-1] = sel->changeStrip[i] - diff;
					}
				}
				for (int i = selectedStripPoint; i < sel->changeStrip.size()-1; i++)
				{
					Vector diff = sel->changeStrip[i] - sel->changeStrip[i+1];
					if (!diff.isLength2DIn(sz))
					{
						diff.setLength2D(sz);
						sel->changeStrip[i+1] = sel->changeStrip[i] - diff;
					}
				}

				b->strip = sel->changeStrip;


				*/

				//sel->setStrip(sel->changeStrip);
			}
		}
	}
}

void AnimationEditor::selectPrevBone()
{
	if (dsq->isNested()) return;

	if (editingStrip)
	{
		//moveBoneStripPoint(Vector(0, 1));
	}
	else
	{
		editSprite->selectPrevBone();
	}
}

void AnimationEditor::selectNextBone()
{
	if (dsq->isNested()) return;

	if (editingStrip)
	{
		//moveBoneStripPoint(Vector(0, -1));
	}
	else
	{
		editSprite->selectNextBone();
	}
}

void AnimationEditor::update(float dt)
{
	StateObject::update(dt);
	std::ostringstream os;
	os << editingFile;
	os << " anim[" << editSprite->getCurrentAnimation()->name << "] ";
	os << "currentKey " << currentKey;
	SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
	if (k)
	{
		os << " keyTime: " << k->t;
	}

	Vector ebdata;

	if (editingBone)
	{
		os << " bone: " << editingBone->name << " [idx " << editingBone->boneIdx << "]";
		ebdata.x = editingBone->position.x;
		ebdata.y = editingBone->position.y;
		ebdata.z = editingBone->rotation.z;
	}
	text->setText(os.str());

	char t2buf[128];
	sprintf(t2buf, "Bone x: %.3f, y: %.3f, rot: %.3f  strip: %d", ebdata.x, ebdata.y, ebdata.z, selectedStripPoint);
	text2->setText(t2buf);

	if (core->mouse.buttons.middle)
	{
		editSprite->position += core->mouse.change;
		//core->setMousePosition(Vector(400,300));
	}

	if (editingStrip)
	{

		if (isActing(ACTION_SWIMLEFT))
			moveBoneStripPoint(Vector(-dt, 0));
		if (isActing(ACTION_SWIMRIGHT))
			moveBoneStripPoint(Vector(dt, 0));

		if (isActing(ACTION_SWIMUP))
			moveBoneStripPoint(Vector(0, -dt));
		if (isActing(ACTION_SWIMDOWN))
			moveBoneStripPoint(Vector(0, dt));
	}
	int spd = 1;
	if (core->mouse.scrollWheelChange < 0)
	{
		editSprite->scale -= Vector(spd*0.05f,spd*0.05f);
	}
	else if (core->mouse.scrollWheelChange > 0)
	{
		editSprite->scale += Vector(spd*0.05f,spd*0.05f);
	}
	if (core->getKeyState(KEY_PGDN) && core->getShiftState())
	{
		editSprite->scale -= Vector(spd*0.05f,spd*0.05f);
	}
	if (core->getKeyState(KEY_PGUP) && core->getShiftState())
	{
		editSprite->scale += Vector(spd*0.05f,spd*0.05f);
	}
	if (editSprite->scale.x < 0.05f)
	{
		editSprite->scale = Vector(0.05f,0.05f);
	}

	if (boneEdit == 0)
	{
		if (editSprite)
			updateEditingBone();
		if (editingBone)
		{
			/*
			float amt = dt;
			if (isActing("mbl"))
			{
				editingBone->position.x -= amt;
				applyTranslation();
			}
			if (isActing("mbr"))
			{
				editingBone->position.x += amt;
				applyTranslation();
			}
			if (isActing("mbu"))
			{
				editingBone->position.y -= amt;
				applyTranslation();
			}
			if (isActing("mbd"))
			{
				editingBone->position.y += amt;
				applyTranslation();
			}
			*/
		}
	}
	if (editingBone && boneEdit == 1)
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
	if (editingBone && boneEdit == 2)
	{
		if (editingBone->getParent() && editingBone->getParent()->isfhr())
			editingBone->rotation.z = rotOffset + (cursorOffset.x - core->mouse.position.x)/2;
		else
			editingBone->rotation.z = rotOffset + (core->mouse.position.x - cursorOffset.x)/2;
		constrainMouse();
	}
	if (boneEdit == 0)
	{
		if (!editSprite->isAnimating())
		{
			SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
			if (k)
				editSprite->setTime(k->t);
			editSprite->updateBones();
		}
	}
}

void AnimationEditor::copy()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
		copyBuffer = *editSprite->getCurrentAnimation()->getKeyframe(currentKey);
}

void AnimationEditor::paste()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
		float time = k->t;
		*k = copyBuffer;
		k->t = time;
	}
}

void AnimationEditor::nextKey()
{
	if (dsq->isNested()) return;

	if (editingStrip)
	{
		selectedStripPoint++;
		if (selectedStripPoint >= editSprite->getSelectedBone(false)->changeStrip.size())
			selectedStripPoint --;
	}
	else
	{
		if (core->getCtrlState())
		{
			for (int i = 0; i < keyframeWidgets.size(); i++)
			{
				keyframeWidgets[i]->shiftLeft();
			}
		}
		else
		{
			currentKey++;
			SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
			if (k)
				editSprite->setTime(k->t);
			else
				currentKey --;
		}
	}
}

void AnimationEditor::prevKey()
{
	if (dsq->isNested()) return;

	if (editingStrip)
	{
		selectedStripPoint--;
		if (selectedStripPoint < 0)
			selectedStripPoint = 0;
	}
	else
	{
		if (core->getCtrlState())
		{
			for (int i = 0; i < keyframeWidgets.size(); i++)
			{
				keyframeWidgets[i]->shiftRight();
			}
		}
		else
		{
			if (currentKey > 0)
			{
				currentKey --;
				SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
				if (k)
					editSprite->setTime(k->t);
			}
		}
	}
}


void AnimationEditor::copyKey()
{
	SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
	buffer = *k;
}

void AnimationEditor::newAnim()
{
	if (dsq->isNested()) return;

	std::string name = dsq->getUserInputString("NewAnimName", "");
	if (!name.empty())
	{
		Animation anim = *editSprite->getCurrentAnimation();
		anim.name = name;
		editSprite->animations.push_back(anim);
		editSprite->lastAnimation();
	}
}

void AnimationEditor::pasteKey()
{
	if (dsq->isNested()) return;

	SkeletalKeyframe *k = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
	(*k) = buffer;
}

void AnimationEditor::newKey()
{
	if (dsq->isNested()) return;

	editSprite->getCurrentAnimation()->cloneKey(currentKey, TIMELINE_UNIT);
	currentKey++;
	rebuildKeyframeWidgets();
}

void AnimationEditor::editStripKey()
{
	if (dsq->isNested()) return;

	if (editingStrip)
	{
		selectedStripPoint = 0;
		editingStrip = false;
		bgGrad->makeVertical(Vector(0.4, 0.4, 0.4), Vector(0.8, 0.8, 0.8));
	}
	else
	{
		editingStrip = true;
		bgGrad->makeVertical(Vector(0.4, 0.4, 0.6), Vector(0.8, 0.8, 1));
	}
}

void AnimationEditor::deleteKey()
{
	if (dsq->isNested()) return;

	if (currentKey > 0)
	{
		editSprite->getCurrentAnimation()->deleteKey(currentKey);
		currentKey --;
	}
	rebuildKeyframeWidgets();
}

void AnimationEditor::animate()
{
	if (dsq->isNested()) return;

	editSprite->playCurrentAnimation(-1);
}

void AnimationEditor::stop()
{
	if (dsq->isNested()) return;

	editSprite->stopAnimation();
}

void AnimationEditor::animateOrStop()
{
	if (dsq->isNested()) return;

	if (core->getShiftState())
		editSprite->stopAnimation();
	else
		editSprite->playCurrentAnimation(-1);
}

void AnimationEditor::lmbd()
{
	pushUndo();
	updateEditingBone();
	if (editingBone /*&& (editSprite->position - core->mouse.position).isLength2DIn(400)*/
		/*&& core->mouse.position.x > 200 && core->mouse.position.y < 560*/
		&& core->mouse.position.x > 400-200 && core->mouse.position.x < 400+200
		&& core->mouse.position.y > 300-200 && core->mouse.position.y < 300+200
		)
	{
		cursorOffset = editingBone->position + editSprite->position - core->mouse.position;
		boneEdit = 1;
	}
}

void AnimationEditor::applyTranslation()
{
	if (editingBone)
	{
		if (!core->getShiftState())
		{
			// one bone mode
			BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (b)
			{
				b->x = editingBone->position.x;
				b->y = editingBone->position.y;
			}
		}
		else
		{
			BoneKeyframe *bcur = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (bcur)
			{
				int xdiff = editingBone->position.x - bcur->x;
				int ydiff = editingBone->position.y - bcur->y;
				// all bones mode
				for (int i = 0; i < editSprite->getCurrentAnimation()->getNumKeyframes(); ++i)
				{
					BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
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

void AnimationEditor::applyRotation()
{
	BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
	if (b)
	{
		b->rot = editingBone->rotation.z = 0;
	}
}

void AnimationEditor::lmbu()
{
	switch(boneEdit)
	{
	case 1:
	{
		applyTranslation();
	}
	break;
	}

	boneEdit = 0;
}

void AnimationEditor::rmbd()
{
	pushUndo();
	updateEditingBone();
	if (editingBone)
	{
		//cursorOffset = editingBone->position + editSprite->position - core->mouse.position;
		cursorOffset = core->mouse.position;
		rotOffset = editingBone->rotation.z;
		boneEdit = 2;
	}
}

void AnimationEditor::clearRot()
{
	if (dsq->isNested()) return;

	updateEditingBone();
	if (editingBone)
	{
		applyRotation();
	}
}

void AnimationEditor::clearPos()
{
	if (dsq->isNested()) return;

	updateEditingBone();
	if (editingBone)
	{
		BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
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
	switch(boneEdit)
	{
	case 2:
	{
		if (editingBone)
		{
			if (!core->getShiftState())
			{
				// one bone mode
				BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
				if (b)
				{
					b->rot = int(editingBone->rotation.z);
				}
			}
			else
			{
				BoneKeyframe *bcur = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
				if (bcur)
				{
					int rotdiff = editingBone->rotation.z - bcur->rot;
					for (int i = 0; i < editSprite->getCurrentAnimation()->getNumKeyframes(); ++i)
					{
						BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
						if (b)
						{
							b->rot += rotdiff;
						}
					}
				}
			}
		}
	}
	break;
	}

	boneEdit = 0;
}

void AnimationEditor::mmbd()
{
	//editingBone = editSprite->getSelectedBone(ignoreBone);
	//cloneBoneAhead();
}

void AnimationEditor::cloneBoneAhead()
{
	updateEditingBone();
	if (editingBone && currentKey >= 0)
	{
		SkeletalKeyframe *s1 = editSprite->getCurrentAnimation()->getKeyframe(currentKey);
		BoneKeyframe *b1 = 0;
		if (s1)
			b1 = s1->getBoneKeyframe(editingBone->boneIdx);

		SkeletalKeyframe *s2 = editSprite->getCurrentAnimation()->getKeyframe(currentKey+1);
		BoneKeyframe *b2 = 0;
		if (s2)
			b2 = s2->getBoneKeyframe(editingBone->boneIdx);

		if (b1 && b2)
		{
			b2->x = b1->x;
			b2->y = b1->y;
			b2->rot = b1->rot;
			b2->strip = b1->strip;
		}
	}
}

void AnimationEditor::saveFile()
{
	editSprite->saveSkeletal(editingFile);
	dsq->screenMessage("Saved anim: " + editingFile);
}

void AnimationEditor::loadFile()
{
	SkeletalSprite::clearCache();
	lastSelectedBone = 0;
	editingBone = 0;
	clearUndoHistory();
	editSprite->position = Vector(400,300);
	editSprite->loadSkeletal(editingFile);
	currentKey = 0;
	rebuildKeyframeWidgets();

	// disable strip edit mode if still active
	if (editingStrip)
		editStripKey();
}

void AnimationEditor::goToTitle()
{
	if (dsq->isNested()) return;

	if (!dsq->returnToScene.empty())
		dsq->game->transitionToScene(dsq->returnToScene);
	else
		dsq->title();
}

void AnimationEditor::quit()
{
	core->quit();
}

void AnimationEditor::nextAnim()
{
	if (dsq->isNested()) return;

	if (!core->getShiftState())
	{
		editSprite->nextAnimation();
		currentKey = 0;
		rebuildKeyframeWidgets();
	}
}

void AnimationEditor::prevAnim()
{
	if (dsq->isNested()) return;

	if (!core->getShiftState())
	{
		editSprite->prevAnimation();
		currentKey = 0;
		rebuildKeyframeWidgets();
	}
}

void AnimationEditor::reverseAnim()
{
	if (dsq->isNested()) return;

	Animation *a = editSprite->getCurrentAnimation();
	if (a)
	{
		debugLog("calling reverse anim");
		a->reverse();
		rebuildKeyframeWidgets();
	}
}

void AnimationEditor::load()
{
	if (dsq->isNested()) return;

	std::string file = dsq->getUserInputString("Enter anim file to load:");
	if (file.empty())		return;
	this->editingFile = file;
	loadFile();
}

void AnimationEditor::loadSkin()
{
	if (dsq->isNested()) return;

	std::string file = dsq->getUserInputString("Enter skin file to load:");
	if (file.empty())		return;
	//this->editingFile = file;
	//loadFile();
	editSprite->loadSkin(file);
}

void AnimationEditor::moveNextWidgets(float dt)
{
	if (dsq->isNested()) return;

	int s = 0;
	KeyframeWidget *w=0;
	for (int i = 0; i < keyframeWidgets.size(); i++)
	{
		w = keyframeWidgets[i];
		if (s)
		{
			editSprite->getCurrentAnimation()->getKeyframe(w->key)->t += dt;
		}
		else if (!s && KeyframeWidget::movingWidget == w)
		{
			s = 1;
		}
	}
	
}

void AnimationEditor::toggleRenderBorders()
{
	if (dsq->isNested()) return;

	renderBorders = !renderBorders;
	updateRenderBorders();
}

void AnimationEditor::updateRenderBorders()
{
	if (!editSprite)
		return;

	for (size_t i = 0; i < editSprite->bones.size(); ++i)
	{
		editSprite->bones[i]->renderBorder = renderBorders;
		editSprite->bones[i]->renderCenter = renderBorders;
		editSprite->bones[i]->borderAlpha = 0.8f;
	}
}

void AnimationEditor::updateEditingBone()
{
	if (!editingStrip)
		editingBone = editSprite->getSelectedBone(mouseSelection);
}

void AnimationEditor::showAllBones()
{
	if (dsq->isNested()) return;

	for (size_t i = 0; i < editSprite->bones.size(); ++i)
		editSprite->bones[i]->renderQuad = true;
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

void AnimationEditor::updateTimelineGrid()
{
	std::ostringstream os;
	os << "Grid: " << TIMELINE_GRIDSIZE;
	gridsize->setText(os.str());
}



#endif  // AQUARIA_BUILD_SCENEEDITOR

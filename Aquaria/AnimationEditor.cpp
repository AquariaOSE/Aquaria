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
const int KEYFRAME_POS_Y	= 570;

class TimelineRender : public RenderObject
{
	void onRender(const RenderState& rs) const OVERRIDE
	{
		glLineWidth(1);
		glBegin(GL_LINES);
		glColor4f(1, 1, 1, 1);
		for (int x = 0; x < 800; x += TIMELINE_GRIDSIZE)
		{
			glVertex3f(x, -5, 0);
			glVertex3f(x, 5, 0);
		}
		glEnd();
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
	b = new BitmapText(dsq->smallFont);
	b->position = Vector(1, -15);
	b->setFontSize(12);
	addChild(b, PM_POINTER);
	this->key = key;
	ae->keyframeWidgets.push_back(this);
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
	dsq->toggleCursor(true, 0.1f);
	core->cameraPos = Vector(0,0);
	editingStrip = false;
	selectedStripPoint = 0;
	mouseSelection = true;
	editingFile = "Naija";
	renderBorderMode = RENDER_BORDER_MINIMAL;
	ae = this;
	StateObject::applyState();
	boneEdit = 0;
	editingBone = 0;
	currentKey = 0;
	splinegrid = 0;

	editSprite = new SkeletalSprite();
	editSprite->cull = false;
	editSprite->loadSkeletal(editingFile);
	editSprite->position = Vector(400,300);


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
	addAction(MakeFunctionEvent(AnimationEditor, copy), KEY_C, 0);
	addAction(MakeFunctionEvent(AnimationEditor, paste), KEY_V, 0);

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

	addAction(MakeFunctionEvent(AnimationEditor, decrTimelineUnit), KEY_U, 0);
	addAction(MakeFunctionEvent(AnimationEditor, incrTimelineUnit), KEY_I, 0);
	addAction(MakeFunctionEvent(AnimationEditor, decrTimelineGrid), KEY_O, 0);
	addAction(MakeFunctionEvent(AnimationEditor, incrTimelineGrid), KEY_P, 0);



	addAction(ACTION_SWIMLEFT,	KEY_J, -1);
	addAction(ACTION_SWIMRIGHT, KEY_K, -1);
	addAction(ACTION_SWIMUP,	KEY_UP, -1);
	addAction(ACTION_SWIMDOWN,	KEY_DOWN, -1);



	addRenderObject(editSprite, LR_ENTITIES);

	Quad *back = new Quad;
	{
		back->color = 0;
		back->setWidthHeight(800, 600);
		back->position = Vector(400,300, -0.2f);
	}
	addRenderObject(back, LR_BACKDROP);

	bgGrad = new Gradient;
	bgGrad->scale = Vector(800, 600);
	bgGrad->position = Vector(400,300);
	bgGrad->makeVertical(Vector(0.4f, 0.4f, 0.4f), Vector(0.8f, 0.8f, 0.8f));
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

	dsq->overlay->alpha.interpolateTo(0, 0.5f);

	rebuildKeyframeWidgets();

	dsq->resetTimer();

	dsq->toggleCursor(true, 0.1f);

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

	if(undoHistory.size() > 50)
		undoHistory.pop_front();

	undoEntry = undoHistory.size()-1;
}

void AnimationEditor::undo()
{
	if (dsq->isNested()) return;

	if (core->getCtrlState())
	{
		if (undoEntry < undoHistory.size())
		{
			editSprite->animations = undoHistory[undoEntry].animations;
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
		undoEntry++;
		if (undoEntry < undoHistory.size())
		{
			editSprite->animations = undoHistory[undoEntry].animations;
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
	editSprite->getCurrentAnimation()->reorderKeyframes();
	rebuildKeyframeWidgets();
}

void AnimationEditor::rebuildKeyframeWidgets()
{
	int offx=0;
	for (size_t i = 0; i < keyframeWidgets.size(); i++)
	{
		keyframeWidgets[i]->setLife(0.03f);
		keyframeWidgets[i]->setDecayRate(1);
		offx = keyframeWidgets[i]->offset.x;
	}
	keyframeWidgets.clear();
	if (Animation *a = editSprite->getCurrentAnimation())
	{
		for (int i = 0; i < 1000; i++)
		{
			SkeletalKeyframe *key = a->getKeyframe(i);
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
				if (b->grid.size() < sel->changeStrip.size())
				{
					b->grid.resize(sel->changeStrip.size());
				}

				b->grid[selectedStripPoint] = sel->changeStrip[selectedStripPoint] += mov*0.006f;
			}
		}
	}
}

void AnimationEditor::selectPrevBone()
{
	if (dsq->isNested()) return;

	if (editingStrip)
	{

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

	}
	else
	{
		editSprite->selectNextBone();
	}
}

void AnimationEditor::update(float dt)
{
	StateObject::update(dt);
	if(!editSprite->getCurrentAnimation())
		return;
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

	RenderObject *ctrlSprite;
	if(splinegrid)
		ctrlSprite = splinegrid;
	else
		ctrlSprite = editSprite;

	if (core->mouse.buttons.middle)
	{
		ctrlSprite->position += core->mouse.change;
	}

	if (editingStrip && !splinegrid)
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
	float spd = 1.0f;
	if (core->mouse.scrollWheelChange < 0)
	{
		ctrlSprite->scale.x /= 1.12f;
	}
	else if (core->mouse.scrollWheelChange > 0)
	{
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

	if (boneEdit == 0)
	{
		if (editSprite)
			updateEditingBone();
		if (editingBone)
		{

		}
	}
	if (editingBone && boneEdit == 1 && !splinegrid)
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

	if(splinegrid && editingBone && editingStrip && splinegrid->wasModified)
	{
		applySplineGridToBone();
		splinegrid->wasModified = false;
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

	if (editingStrip && !splinegrid)
	{
		selectedStripPoint++;
		if (selectedStripPoint >= editSprite->getSelectedBone(false)->changeStrip.size()
				&& selectedStripPoint > 0)
			selectedStripPoint --;
	}
	else
	{
		if (core->getCtrlState())
		{
			for (size_t i = 0; i < keyframeWidgets.size(); i++)
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

			onKeyframeChanged();
		}
	}
}

void AnimationEditor::prevKey()
{
	if (dsq->isNested()) return;

	if (editingStrip && !splinegrid)
	{
		if(selectedStripPoint > 0) {
			selectedStripPoint--;
		}
	}
	else
	{
		if (core->getCtrlState())
		{
			for (size_t i = 0; i < keyframeWidgets.size(); i++)
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

				onKeyframeChanged();
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
		bgGrad->makeVertical(Vector(0.4f, 0.4f, 0.4f), Vector(0.8f, 0.8f, 0.8f));

		if(splinegrid)
		{
			//editSprite->alphaMod = 1;
			//editSprite->removeChild(splinegrid);
			splinegrid->safeKill();
			splinegrid = NULL;
		}
	}
	else
	{
		if(editingBone && editingBone->getGrid())
		{
			RenderGrid *grid = editingBone->getGrid();
			Animation *a = editSprite->getCurrentAnimation();
			BoneGridInterpolator *interp = a->getBoneGridInterpolator(editingBone->boneIdx);

			editingStrip = true;

			if(interp)
			{
				bgGrad->makeVertical(Vector(0.4f, 0.6f, 0.4f), Vector(0.8f, 1, 0.8f));

				BoneKeyframe *bk = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
				assert(bk->controlpoints.size() == interp->bsp.ctrlX() * interp->bsp.ctrlY());

				splinegrid = new SplineGrid;
				RenderGrid *rgrid = splinegrid->resize(interp->bsp.ctrlX(), interp->bsp.ctrlY(), grid->width(), grid->height(), interp->bsp.degX(), interp->bsp.degY());
				rgrid->drawOrder = grid->drawOrder;
				splinegrid->setTexture(editingBone->texture->name);
				splinegrid->setWidthHeight(editingBone->width, editingBone->height);
				splinegrid->position = Vector(400, 300);
				//splinegrid->followCamera = 1;
				splinegrid->importControlPoints(&bk->controlpoints[0]);
				//editSprite->addChild(splinegrid, PM_STATIC, RBP_OFF, CHILD_FRONT);
				//editSprite->alphaMod = 0.5f;
				addRenderObject(splinegrid, LR_PARTICLES_TOP);
			}
			else
			{
				bgGrad->makeVertical(Vector(0.4f, 0.4f, 0.6f), Vector(0.8f, 0.8f, 1));
			}

		}
		else if(editingBone)
		{
			debugLog("Bone has no grid, cannot edit grid");
			dsq->sound->playSfx("denied");
		}
		else
		{
			debugLog("No bone selected for grid edit mode");
			dsq->sound->playSfx("denied");
		}
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
	if (editingBone

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
				if(!core->getCtrlState())
				{
					// all bones in one anim mode
					for (size_t i = 0; i < editSprite->getCurrentAnimation()->getNumKeyframes(); ++i)
					{
						BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
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
					for (size_t a = 0; a < editSprite->animations.size(); ++a)
					{
						for (size_t i = 0; i < editSprite->animations[a].getNumKeyframes(); ++i)
						{
							BoneKeyframe *b = editSprite->animations[a].getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
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
		if (!core->getShiftState())
		{
			BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (b)
			{
				b->rot = -b->rot;
			}
		}
		else
		{
			BoneKeyframe *bcur = editSprite->getCurrentAnimation()->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
			if (bcur)
			{
				if (!core->getCtrlState())
				{
					for (size_t i = 0; i < editSprite->getCurrentAnimation()->getNumKeyframes(); ++i)
					{
						BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
						if (b)
						{
							b->rot = -b->rot;
						}
					}
				}
				else
				{
					// all bones in all anims mode
					for (size_t a = 0; a < editSprite->animations.size(); ++a)
					{
						for (size_t i = 0; i < editSprite->animations[a].getNumKeyframes(); ++i)
						{
							BoneKeyframe *b = editSprite->animations[a].getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
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
					if (!core->getCtrlState())
					{
						for (size_t i = 0; i < editSprite->getCurrentAnimation()->getNumKeyframes(); ++i)
						{
							BoneKeyframe *b = editSprite->getCurrentAnimation()->getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
							if (b)
							{
								b->rot += rotdiff;
							}
						}
					}
					else
					{
						// all bones in all anims mode
						for (size_t a = 0; a < editSprite->animations.size(); ++a)
						{
							for (size_t i = 0; i < editSprite->animations[a].getNumKeyframes(); ++i)
							{
								BoneKeyframe *b = editSprite->animations[a].getKeyframe(i)->getBoneKeyframe(editingBone->boneIdx);
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
	break;
	}

	boneEdit = 0;
}

void AnimationEditor::mmbd()
{


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
			b2->grid = b1->grid;
		}
	}
}

void AnimationEditor::saveFile()
{
	if(editSprite->saveSkeletal(editingFile))
		dsq->screenMessage("Saved anim: " + editingFile);
	else
		dsq->screenMessage("FAILED TO SAVE: " + editingFile);
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

	onKeyframeChanged();
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
	if (editingStrip) return;

	if (!core->getShiftState())
	{
		editSprite->prevAnimation();
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

	if(editSprite->selectAnimation(name.c_str()))
	{
		currentKey = 0;
		rebuildKeyframeWidgets();
	}
	else
		dsq->screenMessage("No such anim name");
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
	SkeletalSprite::clearCache();
	loadFile();
}

void AnimationEditor::loadSkin()
{
	if (dsq->isNested()) return;

	std::string file = dsq->getUserInputString("Enter skin file to load:");
	if (file.empty())		return;


	SkeletalSprite::clearCache();
	editSprite->loadSkin(file);
}

void AnimationEditor::moveNextWidgets(float dt)
{
	if (dsq->isNested()) return;

	int s = 0;
	KeyframeWidget *w=0;
	for (size_t i = 0; i < keyframeWidgets.size(); i++)
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

	renderBorderMode = (RenderBorderMode)(renderBorderMode + 1);
	if(renderBorderMode > RENDER_BORDER_ALL)
		renderBorderMode = RENDER_BORDER_NONE;
	updateRenderBorders();
}

void AnimationEditor::updateRenderBorders()
{
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

void AnimationEditor::onKeyframeChanged()
{
	applyBoneToSplineGrid();

	updateRenderBorders(); // restore default state
}

void AnimationEditor::applyBoneToSplineGrid()
{
	if(splinegrid && editingBone)
	{
		Animation *a = editSprite->getCurrentAnimation();
		BoneKeyframe *bk = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
		assert(bk->controlpoints.size() == splinegrid->getSpline().ctrlX() * splinegrid->getSpline().ctrlY());
		assert(bk->grid.size() == editingBone->getDrawGrid().linearsize());
		splinegrid->importControlPoints(&bk->controlpoints[0]);
	}
}

void AnimationEditor::applySplineGridToBone()
{
	if(splinegrid && editingBone)
	{
		Animation *a = editSprite->getCurrentAnimation();
		BoneKeyframe *bk = a->getKeyframe(currentKey)->getBoneKeyframe(editingBone->boneIdx);
		assert(bk->controlpoints.size() == splinegrid->getSpline().ctrlX() * splinegrid->getSpline().ctrlY());
		assert(bk->grid.size() == editingBone->getDrawGrid().linearsize());
		splinegrid->exportControlPoints(&bk->controlpoints[0]);
		BoneGridInterpolator *interp = a->getBoneGridInterpolator(editingBone->boneIdx);
		interp->updateGridAndBone(*bk, editingBone);
	}
}

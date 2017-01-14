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
#ifndef __title__
#define __title__

#include "DSQ.h"
#include "../BBGE/SkeletalSprite.h"

class ParticleEffect;

class Bubble : public Quad
{
public:
	Bubble();
protected:
	void onUpdate(float dt);
	int speed;
};

class GameOver : public StateObject
{
public:
	GameOver();
	void applyState();
	void removeState();
	void update(float dt);

	void onClick();
	Quad *frame1, *frame2, *frame3;

	float timer;
};

class Intro : public StateObject
{
public:
	Intro();
	void applyState();
	void removeState();
	void update(float dt);

	void endIntro();
	bool waitQuit(float t);
protected:
	void createMeteor(int layer, Vector pos, Vector off, Vector sz);
	void clearMeteors();

	std::vector<Quad*>meteors;

	bool done;

	Precacher cachy;

};

class Intro2 : public StateObject
{
public:
	Intro2();
	void applyState();
	void removeState();
	void update(float dt);
	void skipIntro();
};

class BitBlotLogo : public StateObject
{
public:
	BitBlotLogo();
	void applyState();
	void removeState();
	void update(float dt);

	void doShortBitBlot();
	void getOut();
	void skipLogo();

	bool watchQuit(float time);
protected:
	int quitFlag;
	int logo;
};

class KeyframeWidget : public Quad
{
public:
	KeyframeWidget(int key);
	float t;
	int key;
	static KeyframeWidget *movingWidget;
	BitmapText *b;

	void shiftLeft();
	void shiftRight();
protected:
	void onUpdate(float dt);
};

class Hair;

class ParticleEditor : public StateObject
{
public:
	ParticleEditor();
	void applyState();
	void removeState();
	ParticleEffect *emitter;
	void load();
	void start();
	void stop();
	void reload();
	void goToTitle();
	void update(float dt);
	void toggleHair();
	Quad *test;
protected:
	Hair *hair;
	std::string lastLoadedParticle;
};

class AnimationEditor : public StateObject
{
public:
	AnimationEditor();
	void applyState();
	void removeState();
	void update(float dt);

	void prevKey();
	void nextKey();
	void newKey();
	void deleteKey();
	void animate();
	void stop();
	void animateOrStop();
	void newAnim();

	void lmbu();
	void lmbd();
	void rmbu();
	void rmbd();
	void mmbd();

	void constrainMouse();

	void reorderKeys();

	void saveFile();
	void loadFile();

	void copyKey();
	void pasteKey();

	void nextAnim();
	void prevAnim();

	void quit();

	void load();
	void loadSkin();

	void copy();
	void paste();

	void cloneBoneAhead();

	void zoomIn();
	void zoomOut();

	void resetScaleOrSave();

	void clearRot();
	void clearPos();
	void flipRot();
	void cycleLerpType();

	void toggleHideBone();

	void undo();
	void redo();
	void pushUndo();
	void clearUndoHistory();

	void applyTranslation();
	void applyRotation();

	void moveNextWidgets(float dt);

	std::deque<SkeletalSprite> undoHistory;

	size_t undoEntry;

	int currentKey;

	int rotOffset;

	SkeletalSprite *editSprite;
	Bone *editingBone;
	int boneEdit;
	DebugFont *text, *text2;

	void goToTitle();

	SkeletalKeyframe copyBuffer;

	std::string editingFile;

	std::vector<KeyframeWidget*> keyframeWidgets;

	void action(int id, int state);

	void rebuildKeyframeWidgets();

	Vector cursorOffset;

	void moveBoneStripPoint(const Vector &mov);

	void editStripKey();
	void toggleMouseSelection();
	void selectPrevBone();
	void selectNextBone();

	bool mouseSelection;

	SkeletalKeyframe buffer;

	bool editingStrip;
	size_t selectedStripPoint;

	void reverseAnim();

	void toggleRenderBorders();
	void updateRenderBorders();
	bool renderBorders;
	void updateEditingBone();
	void showAllBones();
	void incrTimelineUnit();
	void decrTimelineUnit();
	void updateTimelineUnit();
	void incrTimelineGrid();
	void decrTimelineGrid();
	void updateTimelineGrid();
	DebugFont *gridsize, *unitsize;
};

class Credits : public StateObject
{
public:
	Credits();
	void applyState();
	void removeState();

	void update(float dt);
};

class Nag : public StateObject
{
public:
	Nag();
	void applyState();
	void removeState();

	void update(float dt);

	void onBuy();
	void onExit();

protected:
	int click;
	bool grab;
	bool hitBuy;
};

#endif


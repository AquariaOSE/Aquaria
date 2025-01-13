#ifndef ANIMATION_EDITOR_H
#define ANIMATION_EDITOR_H

#include "../BBGE/SkeletalSprite.h"
#include "StateManager.h"
#include <deque>

class DebugFont;
class BitmapText;
class SplineGrid;
class DebugButton;
class Gradient;

// internal
struct AnimationEditorPage;
class TimelineTickRender;

class AnimationEditor : public StateObject
{
	void _copyKey();
	void _pasteKey();

	enum EditMode
	{
		AE_SELECT,   // moving mouse selects nearest bone
		AE_EDITING_MOVE,  // moving mouse moves editingBone
		AE_EDITING_ROT,  // moving mouse rotates editingBone
		AE_STRIP,    // in strip edit mode for editingBone
		AE_SPLINE,   // in spline edit mode for editingBone
	};

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
	void loadFile(size_t pg, const char *fn);
	void reloadFile();

	void nextAnim();
	void prevAnim();
	void selectAnim();

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

	void applyTranslation();
	void applyRotation();

	void moveNextWidgets(float dt);

	int currentKey;

	int rotOffset;

	Bone *editingBone; // only changed when editMode == AE_SELECT
	SkeletalSprite *editingBoneSprite; // updated together with editingBone
	int editingBonePage;
	int editingBoneIdx;  // editingBoneSprite->bones[editingBoneIdx] == editingBone
	EditMode editMode;
	DebugFont *text, *text2, *toptext, *btmtext;

	void goToTitle();

	SkeletalKeyframe copyBuffer;

	void action(int id, int state, int source, InputDevice device);

	void rebuildKeyframeWidgets();

	Vector cursorOffset;

	void moveBoneStripPoint(const Vector &mov);

	void editStripKey();
	void toggleMouseSelection();
	void selectPrevBone();
	void selectNextBone();

	bool mouseSelection;

	bool assistedSplineEdit;
	size_t selectedStripPoint;

	void reverseAnim();
	void flipH();

	void toggleRenderBorders();
	void updateRenderBorders();

	enum RenderBorderMode
	{
		RENDER_BORDER_NONE,
		RENDER_BORDER_MINIMAL,
		RENDER_BORDER_ALL, // must be last
	};
	RenderBorderMode renderBorderMode;
	void updateEditingBone();
	void showAllBones();
	void incrTimelineUnit();
	void decrTimelineUnit();
	void updateTimelineUnit();
	void incrTimelineGrid();
	void decrTimelineGrid();
	void updateTimelineGrid();
	DebugFont *gridsize, *unitsize;

	void onKeyframeChanged();

	SplineGrid *splinegrid;
	void applySplineGridToBone();
	void applyBoneToSplineGrid();

	void toggleSplineMode();
	DebugButton *bSplineAssist;
	void updateButtonLabels();
	void toggleGradient();
	float getMouseTimelineTime() const; // <0 when not in timeline area
	bool isMouseInRect() const;

	Gradient *bgGrad;
	Quad *rect;

	Animation *getPageAnimation(size_t page) const;
	Animation *getCurrentPageAnimation() const;
	SkeletalSprite *getPageSprite(size_t page) const;
	SkeletalSprite *getCurrentPageSprite() const; // sprite on active page
	SkeletalSprite *getSelectedPageSprite() const; //sprite that belong to selected bone, alternatively sprite on active page
	bool isAnimating() const;
	float getAnimTime() const;

	AnimationEditorPage *pages;
	Quad *spriteRoot;
	TimelineTickRender *timelineTicks;

	void selectPage(unsigned page);
	int curPage;

private:
	void selectPage0() { selectPage(0); }
	void selectPage1() { selectPage(1); }
	void selectPage2() { selectPage(2); }
	void selectPage3() { selectPage(3); }
	void selectPage4() { selectPage(4); }
	void selectPage5() { selectPage(5); }
	void selectPage6() { selectPage(6); }
	void selectPage7() { selectPage(7); }
	void selectPage8() { selectPage(8); }

	void _stopExtraEditModes();

	void saveAll();
	void reloadAll();
	bool savePage(size_t pg);
	void reloadPage(size_t pg);

	void _selectBone(Bone *b); // NULL to unselect
};


#endif

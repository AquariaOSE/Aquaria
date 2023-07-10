#ifndef AQUARIA_SCENEEDITOR_H
#define AQUARIA_SCENEEDITOR_H

#include "Base.h"
#include "AquariaCompileConfig.h"
#include "DebugFont.h"
#include "ActionMapper.h"
#include "Quad.h"
#include "DataStructures.h"

class Element;
class Entity;
class Path;

struct EntityGroupEntity
{
	std::string name;
	std::string gfx;
};

typedef std::vector<EntityGroupEntity> EntityGroupEntities;

struct EntityGroup
{
	std::string name;
	EntityGroupEntities entities;
};

typedef std::vector<EntityGroup> EntityGroups;

enum EditTypes
{
	ET_ELEMENTS		=0,
	ET_ENTITIES		=1,
	ET_PATHS		=2,
	ET_SELECTENTITY =4,
	ET_MAX
};

enum EditorStates
{
	ES_SELECTING	=0,
	ES_SCALING		=1,
	ES_ROTATING		=2,
	ES_MOVING		=3,
	ES_MAX
};

class SceneEditorMenuReceiver : public DebugButtonReceiver
{
public:
	void buttonPress(DebugButton *db);
};

struct SelectedEntity
{
	SelectedEntity();
	void clear();
	void setIndex(int idx);
	void setName(const std::string &name, const std::string &prevGfx);
	void setSelectEntity(const SelectedEntity &ent);

	bool nameBased;
	int index, typeListIndex;
	std::string name;
	std::string prevGfx;
	float prevScale;
};

class SceneEditor : public ActionMapper
{
public:
	SceneEditor();
	void init();
	void shutdown();
	void toggle();
	void toggle(bool on);
	void update(float dt);
	void prevElement();
	void nextElement();
	void doPrevElement();
	Element *cycleElementNext(Element *e);
	Element *cycleElementPrev(Element *e);
	void selectZero();
	void selectEnd();
	void placeElement();
	void flipElementHorz();
	void flipElementVert();
	void deleteSelectedElement();
	void deleteElement(int selectedIdx);
	virtual void action(int id, int state, int source, InputDevice device);
	void scaleElementUp();
	void scaleElementDown();
	void scaleElement1();
	void placeAvatar();

	void executeButtonID(int bid);

	void openMainMenu();
	void closeMainMenu();

	void setBackgroundGradient();

	bool isOn();

	void generateLevel();
	void skinLevel(int minX, int minY, int maxX, int maxY);
	void skinLevel();

	void regenLevel();

	void down();
	void up();

	void exitMoveState();

	EditTypes editType;
	EditorStates state;

	Element *getElementAtCursor();
	Entity *getEntityAtCursor();

	void mouseButtonLeftUp();
	void mouseButtonRightUp();
	void moveToBack();
	void moveToFront();
	int bgLayer;
	Element *editingElement;
	Entity *editingEntity;
	Path *editingPath;

	size_t selectedIdx;
	size_t selectedNode;

	Path *getSelectedPath();
	void changeDepth();
	void updateEntitySaveData(Entity *editingEntity);
	void moveLayer();
	void moveElementToLayer(Element *e, int bgLayer);
	void toggleElementRepeat();
	bool multiSelecting;
	Vector multiSelectPoint;
	std::vector <Element*> selectedElements;

	Vector groupCenter;
	Vector getSelectedElementsCenter();

	Quad dummy;

	void updateSelectedElementPosition(Vector position);
	int selectedEntityType;

	SelectedEntity selectedEntity;

	size_t entityPageNum;

	void checkForRebuild();
	void createAquarian();
	void dumpObs();
	void moveEverythingBy(int x, int y);

	DebugButton *btnMenu;
protected:

	void reversePath();

	void updateEntityPlacer();
	void updateMultiSelect();
	float autoSaveTimer;
	int autoSaveFile;

	void enterName();
	void changeShape();
	int skinMinX, skinMinY, skinMaxX, skinMaxY;

	enum TileProperty
	{
		TPR_DEFAULT,
		TPR_DONT_SKIN
	};
	Array2d<unsigned char> tileProps; // only filled on generateLevel()
	static TileProperty GetColorProperty(unsigned char r, unsigned char g, unsigned char b);

	void setGridPattern(int gi);
	void setGridPattern0();
	void setGridPattern1();
	void setGridPattern2();
	void setGridPattern3();
	void setGridPattern4();
	void setGridPattern5();
	void setGridPattern6();
	void setGridPattern7();
	void setGridPattern8();
	void setGridPattern9();
	void toggleElementSolid();
	void toggleElementHurt();
	void editModeElements();
	void editModeEntities();
	void editModePaths();

	void deleteSelected();
	void cloneSelectedElement();
	void enterScaleState();
	void enterRotateState();
	void enterMoveState();

	Vector oldPosition, oldRotation, oldScale, cursorOffset, oldRepeatScale;

	Entity *movingEntity;

	void nextEntityType();
	void prevEntityType();

	void removeEntity();

	void selectEntityFromGroups();

	Vector zoom;

	void rotateElement();
	void rotateElement2();
	void updateText();

	void saveScene();
	void loadScene();

	SceneEditorMenuReceiver menuReceiver;

	void addMainMenuItem(const std::string &label, int bid);

	void loadSceneByName();
	void reloadScene();

	void mouseButtonLeft();
	void mouseButtonRight();

	size_t curElement;

	Quad *placer;
	DebugFont *text;
	bool on;
	InterpolatedVector oldGlobalScale;
};

#endif // AQUARIA_SCENEEDITOR_H

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
class PathRender;
class TileStorage;

class MultiTileHelper;

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
	void cyclePlacer(int direction);
	void cycleSelectedTiles(int direction); // transmute selected to next/prev in tileset
	void selectZero();
	void selectEnd();
	void placeElement();
	void flipElementHorz();
	void flipElementVert();
	virtual void action(int id, int state, int source, InputDevice device);
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

	int getTileAtCursor(); // <0 when no tile, otherwise index
	Entity *getEntityAtCursor();

	void mouseButtonLeftUp();
	void mouseButtonRightUp();
	void moveToBack();
	void moveToFront();
	int bgLayer;
	Entity *editingEntity;
	Path *editingPath;

	size_t selectedIdx;
	size_t selectedNode;

	Path *getSelectedPath();
	void updateEntitySaveData(Entity *editingEntity);
	void toggleElementRepeat();
	bool multiSelecting;
	Vector multiSelectPoint;
	std::vector <size_t> selectedTiles; // indices

	void updateSelectedElementPosition(Vector rel);
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
	void enterAnyStateHelper(EditorStates newstate);

	float oldRotation;
	Vector oldPosition, oldScale, cursorOffset, oldRepeatScale;

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

	void setActiveLayer(unsigned bglayer);
	TileStorage& getCurrentLayerTiles();
	void clearSelection();
	MultiTileHelper *createMultiTileHelperFromSelection();
	void destroyMultiTileHelper();

	size_t curElement;

	Quad *placer;
	MultiTileHelper *multi;
	DebugFont *text;
	bool on;
	InterpolatedVector oldGlobalScale;
	PathRender *pathRender;
};

#endif // AQUARIA_SCENEEDITOR_H

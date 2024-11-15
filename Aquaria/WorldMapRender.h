#ifndef WORLDMAPRENDER_H
#define WORLDMAPRENDER_H

#include "Quad.h"
#include "ActionMapper.h"

class GemMover;
struct WorldMapTile;
struct GemData;
class BitmapText;
class AquariaMenuItem;
struct WorldMap;
class Gradient;


// This is used for properly positioning the tile and gems on top of it.
// Affected by scale2 -- gems also move with scale2.
class WorldMapTileContainer : public RenderObject
{
public:
	WorldMapTileContainer(WorldMapTile& tile);
	virtual ~WorldMapTileContainer();

	virtual void onUpdate(float dt) OVERRIDE;

	void refresh(); // Called whenever we need to prepare for rendering

	void removeGems();
	void addGem(GemMover *gem);
	bool removeGem(GemMover *gem);
	GemMover *removeGem(const GemData *gem);
	GemMover *getGem(const GemData *gemData) const;
	void updateGems();

	Vector worldPosToTilePos(const Vector& p) const;
	Vector worldPosToMapPos(const Vector& p) const;

	WorldMapTile& tile;
	Quad q; // The actual world map tile, additionally affected by scale (NOT scale2), and always at (0, 0)
	std::vector<GemMover*> gems;
};

class WorldMapRender : public RenderObject, public ActionMapper
{
public:
	WorldMapRender(WorldMap& wm);
	virtual ~WorldMapRender();
	void init();
	void toggle(bool on);
	bool isOn();
	void setProperTileColor(WorldMapTileContainer& wt);
	void action(int id, int state, int source, InputDevice device);
	GemMover* addGem(GemData *gemData);
	void updateGem(const GemData *gemData);
	void removeGem(GemMover *gemMover);
	void removeGem(const GemData *gemData);
	GemMover *getGem(const GemData *gemData) const;
	WorldMapTileContainer *getTileWithGem(const GemData *gemData) const;
	void bindInput();
	void createGemHint(const std::string &gfx);
	void onToggleHelpScreen();
	bool isCursorOffHud();
	void updateAllTilesColor();
	WorldMapTileContainer *getCurrentTile() { return playerTile; }
	bool getWorldToPlayerTile(Vector& dst, const Vector& pos, bool global) const;
	WorldMapTileContainer *getTileByName(const char *name) const;
	WorldMapTileContainer * setCurrentMap(const char *mapname);

protected:
	Quad *addHintQuad1, *addHintQuad2;
	AquariaMenuItem *helpButton;
	float doubleClickTimer;
	float inputDelay;
	BitmapText *areaLabel, *areaLabel2, *areaLabel3;
	WorldMapTileContainer *playerTile; // tile where the player is located
	WorldMapTileContainer *selectedTile; // starts out == playerTile but changed on selection
	bool on;
	void onUpdate(float dt);
	bool mb, wasEditorSaveDown;
	Vector lastMousePosition; // See FIXME in WorldMapRender.cpp  --achurch
	void updateEditor();
	std::vector<WorldMapTileContainer*> tiles;
	WorldMap& worldmap;
	Quad *tophud;
	Gradient *underlay;
};



#endif

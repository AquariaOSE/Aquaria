#ifndef AQUARIA_MOD_H
#define AQUARIA_MOD_H

#include "GameEnums.h"
#include "Precacher.h"

namespace tinyxml2
{
	class XMLDocument;
	class XMLElement;
}


enum ModType
{
	MODTYPE_MOD,
	MODTYPE_PATCH
};


struct ModEntry
{
	unsigned int id; // index in vector
	ModType type;
	std::string path;
};

class ModSelectorScreen;

class Mod
{
public:
	Mod();
	~Mod();
	void clear();
	void setActive(bool v);
	void start();
	void stop();
	void load(const std::string &path);
	bool loadSavedGame(const std::string& path);

	void update(float dt);

	void recache();

	const std::string& getBaseModPath() const;

	bool isActive() const;
	bool isDebugMenu() const;
	bool hasWorldMap() const;
	bool isEditorBlocked() const;

	const std::string& getPath() const;
	const std::string& getName() const;

	void shutdown();
	bool isShuttingDown();

	static bool loadModXML(tinyxml2::XMLDocument *d, std::string modName);
	static ModType getTypeFromXML(tinyxml2::XMLElement *xml);

	WorldMapRevealMethod mapRevealMethod;

protected:
	bool loadCompatScript();
	bool shuttingDown;
	bool active;
	bool hasMap;
	bool blockEditor;
	int debugMenu;
	int enqueueModStart;
	void applyStart();
	bool tryStart();

	std::string name;
	std::string path;
	Precacher modcache;
	std::string compatScript;
};


#endif

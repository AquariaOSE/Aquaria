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
#include "DSQ.h"
#include "Game.h"
#include "ReadXML.h"
#include "Shot.h"

static std::string baseModPath = "./_mods/";

void refreshBaseModPath()
{
#if defined(BBGE_BUILD_UNIX)
	baseModPath = dsq->getUserDataFolder() + "/_mods/";
#endif
}

Mod::Mod()
{
	clear();

	enqueueModStart = 0;

	shuttingDown = false;
}

Mod::~Mod()
{
	modcache.clear();
}

/*
queue for actual stop and recache
which happens in game::applystate
*/
void Mod::shutdown()
{
	shuttingDown = true;
}

bool Mod::isShuttingDown()
{
	return shuttingDown;
}

void Mod::clear()
{
	active = false;
	debugMenu = false;
	hasMap = false;
	blockEditor = false;
	mapRevealMethod = REVEAL_UNSPECIFIED;
	compatScript = "";
}

bool Mod::isDebugMenu() const
{
	return debugMenu;
}

bool Mod::hasWorldMap() const
{
	return hasMap;
}

bool Mod::isEditorBlocked() const
{
	return blockEditor;
}

bool Mod::loadModXML(XMLDocument *d, std::string modName)
{
	return readXML((baseModPath + modName + ".xml").c_str(), *d) == XML_SUCCESS;

}

const std::string& Mod::getBaseModPath() const
{
	refreshBaseModPath();

	return baseModPath;
}

bool Mod::loadSavedGame(const std::string& path)
{
	load(path);
	if(loadCompatScript())
		return true;

	debugLog("MOD: loadSavedGame/compatScript failed");
	setActive(false);
	dsq->title(false);
	return false;
}

void Mod::load(const std::string &p)
{
	clear();

	refreshBaseModPath();

	name = p;
	path = baseModPath + p + "/";

	setLocalisationModPath(path);

	setActive(true);

	XMLDocument d;
	loadModXML(&d, p);

	XMLElement *mod = d.FirstChildElement("AquariaMod");
	if (mod)
	{
		XMLElement *props = mod->FirstChildElement("Properties");
		if (props)
		{
			props->QueryIntAttribute("debugMenu", &debugMenu);
			props->QueryBoolAttribute("hasWorldMap", &hasMap);
			props->QueryBoolAttribute("blockEditor", &blockEditor);

			if (props->BoolAttribute("runBG"))
				core->settings.runInBackground = true;

			if (props->Attribute("worldMapRevealMethod"))
				mapRevealMethod = (WorldMapRevealMethod) props->IntAttribute("worldMapRevealMethod");
		}
		XMLElement *compat = mod->FirstChildElement("Compatibility");
		if(compat)
		{
			if(const char *script = compat->Attribute("script"))
				compatScript = script;
		}
	}

	dsq->setExtraTexturePath((path + "graphics/").c_str());

	dsq->sound->audioPath2 = path + "audio/";
	dsq->sound->setVoicePath2(path + "audio/");

	SkeletalSprite::secondaryAnimationPath = path + "animations/";

	dsq->particleBank2 = path + "particles/";
	dsq->shotBank2 = path + "shots/";

	Shot::loadShotBank(dsq->shotBank1, dsq->shotBank2);
	particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);
}

const std::string& Mod::getPath() const
{
	return path;
}

const std::string& Mod::getName() const
{
	return name;
}

void Mod::recache()
{
	core->texmgr.reloadAll(TextureMgr::KEEP_IF_SAME);

	if(active)
	{
		modcache.setBaseDir(dsq->getExtraTexturePath());
		std::string fname = path;
		if(fname[fname.length() - 1] != '/')
			fname += '/';
		fname += "precache.txt";
		fname = adjustFilenameCase(fname);
		if (exists(fname))
			modcache.precacheList(fname);
	}
	else
	{
		modcache.clear();
	}

	core->resetTimer();
}

void Mod::start()
{
	dsq->overlay->color = 0;

	dsq->toggleVersionLabel(0);
	dsq->toggleCursor(0, 1);

	float t = 1;
	dsq->overlay->alpha.interpolateTo(1, t);
	core->sound->fadeMusic(SFT_OUT, t*0.9f);

	core->run(t);

	core->sound->stopMusic();

	enqueueModStart = 1;
	dsq->recentSaveSlot = -1;
}

void Mod::applyStart()
{
	enqueueModStart = 0;

	core->popAllStates();
	core->clearGarbage();
	recache();
	dsq->continuity.reset();
	dsq->scriptInterface.reset();

	if(!tryStart())
	{
		setActive(false);
		dsq->title(false);
	}
}

bool Mod::tryStart()
{

	// Before loading init.lua, load a compatibility layer, if necessary
	if(!loadCompatScript())
	{
		debugLog("MOD: compatScript failed");
		return false;
	}

	// load the mod-init.lua file
	// which is in the root of the mod's folder
	// e.g. _mods/recachetest/
	std::string scriptPath = path + "mod-init.lua";
	debugLog("scriptPath: " + scriptPath);
	if (!dsq->runScript(scriptPath, "init"))
	{
		debugLog("MOD: runscript failed");
		return false;
	}
	if (isActive() && game->sceneToLoad.empty())
	{
		debugLog("MOD: no scene loaded in mod-init");
		return false;
	}

	return true;
}

bool Mod::isActive() const
{
	return active;
}

void Mod::setActive(bool a)
{
	bool wasActive = active;

	active = a;

	if (wasActive != active)
	{
		if (!active)
		{
			dsq->unloadMods();
			compatScript = "";

			mapRevealMethod = REVEAL_UNSPECIFIED;
			setLocalisationModPath("");
			name = path = "";
			dsq->setExtraTexturePath(NULL);
			dsq->sound->audioPath2 = "";
			dsq->sound->setVoicePath2("");
			SkeletalSprite::secondaryAnimationPath = "";

			dsq->particleBank2 = "";
			dsq->shotBank2 = "";

			Shot::loadShotBank(dsq->shotBank1, dsq->shotBank2);
			particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);

			recache();
		}
		game->loadEntityTypeList();
	}
}

void Mod::stop()
{
	setActive(false);

	core->settings.runInBackground = false;
	debugMenu = false;
	shuttingDown = false;
	dsq->scriptInterface.reset();
	game->setWorldPaused(false);
}

void Mod::update(float dt)
{
	if (enqueueModStart)
	{
		enqueueModStart = 0;

		applyStart();
	}
}

ModType Mod::getTypeFromXML(XMLElement *xml) // should be <AquariaMod>...</AquariaMod> - element
{
	if(xml)
	{
		XMLElement *prop = xml->FirstChildElement("Properties");
		if(prop)
		{
			const char *type = prop->Attribute("type");
			if(type)
			{
				if(!strcmp(type, "mod"))
					return MODTYPE_MOD;
				else if(!strcmp(type, "patch"))
					return MODTYPE_PATCH;
				else
				{
					std::ostringstream os;
					os << "Unknown mod type '" << type << "' in XML, default to MODTYPE_MOD";
					debugLog(os.str());
				}
			}
		}
	}
	return MODTYPE_MOD; // the default
}

bool Mod::loadCompatScript()
{
	std::string cs = compatScript.c_str();
	if(cs.empty())
		cs = "default";
	if(dsq->runScript("scripts/compat/" + cs + ".lua"))
		return true;

	dsq->scriptInterface.reset();
	return false;
}

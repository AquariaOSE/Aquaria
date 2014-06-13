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
	modcache.clean();
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
	doRecache = 0;
	debugMenu = false;
	hasMap = false;
	blockEditor = false;
	mapRevealMethod = REVEAL_UNSPECIFIED;
}

bool Mod::isDebugMenu()
{
	return debugMenu;
}

bool Mod::hasWorldMap()
{
	return hasMap;
}

bool Mod::isEditorBlocked()
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
			props->QueryIntAttribute("recache", &doRecache);
			props->QueryIntAttribute("debugMenu", &debugMenu);
			props->QueryBoolAttribute("hasWorldMap", &hasMap);
			props->QueryBoolAttribute("blockEditor", &blockEditor);

			if (props->BoolAttribute("runBG"))
				core->settings.runInBackground = true;

			if (props->Attribute("worldMapRevealMethod"))
				mapRevealMethod = (WorldMapRevealMethod) props->IntAttribute("worldMapRevealMethod");
		}
	}

	dsq->secondaryTexturePath = path + "graphics/";

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
	if(doRecache)
	{
		dsq->precacher.clean();
		dsq->unloadResources();
	}

	if(active)
	{
		modcache.setBaseDir(dsq->secondaryTexturePath);
		std::string fname = path;
		if(fname[fname.length() - 1] != '/')
			fname += '/';
		fname += "precache.txt";
		fname = core->adjustFilenameCase(fname);
		if (exists(fname))
		{
			modcache.precacheList(fname);
			core->resetTimer();
		}
	}
	else
	{
		modcache.clean();
	}

	if(doRecache)
	{
		dsq->precacher.precacheList("data/precache.txt");
		dsq->reloadResources();
		core->resetTimer();
	}
}

void Mod::start()
{
	dsq->overlay->color = 0;

	dsq->toggleVersionLabel(0);
	dsq->toggleCursor(0, 1);

	float t = 1;
	dsq->overlay->alpha.interpolateTo(1, t);
	core->sound->fadeMusic(SFT_OUT, t*0.9f);

	core->main(t);

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

	// load the mod-init.lua file
	// which is in the root of the mod's folder
	// e.g. _mods/recachetest/
	std::string scriptPath = path + "mod-init.lua";
	debugLog("scriptPath: " + scriptPath);
	if (!dsq->runScript(scriptPath, "init"))
	{
		debugLog("MOD: runscript failed");
		setActive(false);
		dsq->title();
	}
	if (isActive() && dsq->game->sceneToLoad.empty())
	{
		debugLog("MOD: no scene loaded in mod-init");
		setActive(false);
		dsq->title();
	}
	else if (isActive())
	{
	}
}

bool Mod::isActive()
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

			mapRevealMethod = REVEAL_UNSPECIFIED;
			setLocalisationModPath("");
			name = path = "";
			dsq->secondaryTexturePath = "";
			dsq->sound->audioPath2 = "";
			dsq->sound->setVoicePath2("");
			SkeletalSprite::secondaryAnimationPath = "";

			dsq->particleBank2 = "";
			dsq->shotBank2 = "";

			Shot::loadShotBank(dsq->shotBank1, dsq->shotBank2);
			particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);

			dsq->setFilter(dsq->dsq_filter);

			recache();
		}
		dsq->game->loadEntityTypeList();
	}
}

void Mod::stop()
{
	setActive(false);

	core->settings.runInBackground = false;
	debugMenu = false;
	shuttingDown = false;
	dsq->scriptInterface.reset();
	dsq->game->setWorldPaused(false);
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

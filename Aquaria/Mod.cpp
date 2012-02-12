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

void Mod::loadModXML(TiXmlDocument *d, std::string modName)
{
	d->LoadFile(baseModPath + modName + ".xml");
}

std::string Mod::getBaseModPath()
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

	setActive(true);
	
	TiXmlDocument d;
	loadModXML(&d, p);
	
	TiXmlElement *mod = d.FirstChildElement("AquariaMod");
	if (mod)
	{
		TiXmlElement *props = mod->FirstChildElement("Properties");
		if (props)
		{
			if (props->Attribute("recache")){
				props->Attribute("recache", &doRecache);
			}

			if (props->Attribute("runBG")){
				int runBG = 0;
				props->Attribute("runBG", &runBG);
				if (runBG){
					core->settings.runInBackground = true;
				}
			}

			if (props->Attribute("debugMenu")) {
				props->Attribute("debugMenu", &debugMenu);
			}

			if (props->Attribute("hasWorldMap")) {
				int t;
				props->Attribute("hasWorldMap", &t);
				hasMap = t;
			}
			if (props->Attribute("blockEditor")) {
				int t;
				props->Attribute("blockEditor", &t);
				blockEditor = t;
			}
		}
	}

#if defined(BBGE_BUILD_UNIX)
	dsq->secondaryTexturePath = path + "graphics/";
#else
	dsq->secondaryTexturePath = "./" + path + "graphics/";
#endif

	dsq->sound->audioPath2 = path + "audio/";
	dsq->sound->setVoicePath2(path + "audio/");

	SkeletalSprite::secondaryAnimationPath = path + "animations/";

	dsq->particleBank2 = path + "particles/";
	dsq->shotBank2 = path + "shots/";

	Shot::loadShotBank(dsq->shotBank1, dsq->shotBank2);
	particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);
}

std::string Mod::getPath()
{
	return path;
}

std::string Mod::getName()
{
	return name;
}

void Mod::recache()
{
	if (doRecache)
	{
		dsq->precacher.clean();
		dsq->unloadResources();

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
		dsq->continuity.reset();
		setActive(false);
		dsq->continuity.reset();
		dsq->title();
	}
	if (isActive() && dsq->game->sceneToLoad.empty())
	{
		debugLog("MOD: no scene loaded in mod-init");
		dsq->continuity.reset();
		setActive(false);
		dsq->continuity.reset();
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
}

void Mod::update(float dt)
{
	if (enqueueModStart)
	{
		enqueueModStart = 0;
		
		applyStart();
	}
}

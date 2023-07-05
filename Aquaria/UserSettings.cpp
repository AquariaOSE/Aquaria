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
#include "UserSettings.h"

#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "ReadXML.h"


void UserSettings::save()
{


	XMLDocument doc;
	{
		XMLElement *xml_version = doc.NewElement("Version");
		{
			xml_version->SetAttribute("settingsVersion", VERSION_USERSETTINGS);
		}
		doc.InsertEndChild(xml_version);

		XMLElement *xml_system = doc.NewElement("System");
		{
			XMLElement *xml_debugLog = doc.NewElement("DebugLog");
			{
				xml_debugLog->SetAttribute("on", system.debugLogOn);
			}
			xml_system->InsertEndChild(xml_debugLog);

			XMLElement *xml_locale = doc.NewElement("Locale");
			{
				xml_locale->SetAttribute("name", system.locale.c_str());
			}
			xml_system->InsertEndChild(xml_locale);

			XMLElement *xml_devmode = doc.NewElement("DeveloperMode");
			{
				xml_devmode->SetAttribute("on", system.devModeOn);
			}
			xml_system->InsertEndChild(xml_devmode);

			XMLElement *xml_unsafe = doc.NewElement("AllowDangerousScriptFunctions");
			{
				xml_unsafe->SetAttribute("on", system.allowDangerousScriptFunctions);
			}
			xml_system->InsertEndChild(xml_unsafe);

			XMLElement *xml_grabInp = doc.NewElement("GrabInput");
			{
				xml_grabInp->SetAttribute("on", system.grabInput);
			}
			xml_system->InsertEndChild(xml_grabInp);
		}
		doc.InsertEndChild(xml_system);

		XMLElement *xml_audio = doc.NewElement("Audio");
		{
			XMLElement *xml_volume = doc.NewElement("Volume");
			{
				xml_volume->SetAttribute("sfx", double(audio.sfxvol));
				xml_volume->SetAttribute("vox", double(audio.voxvol));
				xml_volume->SetAttribute("mus", double(audio.musvol));
				xml_volume->SetAttribute("subs", audio.subtitles);
			}
			xml_audio->InsertEndChild(xml_volume);

			XMLElement *xml_device = doc.NewElement("Device");
			{
				xml_device->SetAttribute("name", audio.deviceName.c_str());
			}
			xml_audio->InsertEndChild(xml_device);

			XMLElement *xml_prebuf = doc.NewElement("Prebuffer");
			{
				xml_prebuf->SetAttribute("on", audio.prebuffer);
			}
			xml_audio->InsertEndChild(xml_prebuf);
		}
		doc.InsertEndChild(xml_audio);

		XMLElement *xml_video = doc.NewElement("Video");
		{
			XMLElement *xml_blur = doc.NewElement("Blur");
			{
				xml_blur->SetAttribute("on", video.blur);
			}
			xml_video->InsertEndChild(xml_blur);

			XMLElement *xml_noteEffects = doc.NewElement("NoteEffects");
			{
				xml_noteEffects->SetAttribute("on", video.noteEffects);
			}
			xml_video->InsertEndChild(xml_noteEffects);

			XMLElement *xml_fpsSmoothing = doc.NewElement("FpsSmoothing");
			{
				xml_fpsSmoothing->SetAttribute("v", video.fpsSmoothing);
			}
			xml_video->InsertEndChild(xml_fpsSmoothing);

			XMLElement *xml_numParticles = doc.NewElement("NumParticles");
			xml_numParticles->SetAttribute("v", video.numParticles);
			xml_video->InsertEndChild(xml_numParticles);

			XMLElement *xml_screenMode = doc.NewElement("ScreenMode");
			{
				xml_screenMode->SetAttribute("resx",				video.resx);
				xml_screenMode->SetAttribute("resy",				video.resy);
				xml_screenMode->SetAttribute("hz",				video.hz);
				xml_screenMode->SetAttribute("bits",				video.bits);
				xml_screenMode->SetAttribute("fbuffer",			video.fbuffer);
				xml_screenMode->SetAttribute("full",				video.full);
				xml_screenMode->SetAttribute("vsync",			video.vsync);
				xml_screenMode->SetAttribute("darkfbuffer",		video.darkfbuffer);
				xml_screenMode->SetAttribute("darkbuffersize",	video.darkbuffersize);
				xml_screenMode->SetAttribute("displayindex",		video.displayindex);
			}
			xml_video->InsertEndChild(xml_screenMode);

			XMLElement *xml_saveSlotScreens = doc.NewElement("SaveSlotScreens");
			{
				xml_saveSlotScreens->SetAttribute("on", video.saveSlotScreens);
			}
			xml_video->InsertEndChild(xml_saveSlotScreens);

			XMLElement *xml_worldMap = doc.NewElement("WorldMap");
			{
				xml_worldMap->SetAttribute("revealMethod", video.worldMapRevealMethod);
			}
			xml_video->InsertEndChild(xml_worldMap);
		}
		doc.InsertEndChild(xml_video);


		XMLElement *xml_control = doc.NewElement("Control");
		{
			XMLElement *xml_toolTipsOn = doc.NewElement("ToolTipsOn");
			{
				xml_toolTipsOn->SetAttribute("on", control.toolTipsOn);
			}
			xml_control->InsertEndChild(xml_toolTipsOn);

			XMLElement *xml_joystickEnabled = doc.NewElement("JoystickEnabled");
			{
				xml_joystickEnabled->SetAttribute("on", control.joystickEnabled);
			}
			xml_control->InsertEndChild(xml_joystickEnabled);

			XMLElement *xml_autoAim = doc.NewElement("AutoAim");
			{
				xml_autoAim->SetAttribute("on", control.autoAim);
			}
			xml_control->InsertEndChild(xml_autoAim);

			XMLElement *xml_targeting = doc.NewElement("Targeting");
			{
				xml_targeting->SetAttribute("on", control.targeting);
			}
			xml_control->InsertEndChild(xml_targeting);

			XMLElement *xml_flip = doc.NewElement("FlipInputButtons");
			{
				xml_flip->SetAttribute("on", control.flipInputButtons);
			}
			xml_control->InsertEndChild(xml_flip);

			XMLElement *xml_minas = doc.NewElement("MinActionSets");
			{
				xml_minas->SetAttribute("num", control.flipInputButtons);
			}
			xml_control->InsertEndChild(xml_minas);

			for(size_t i = 0; i < control.actionSets.size(); ++i)
			{
				const ActionSet& as = control.actionSets[i];
				XMLElement *xml_actionSet = doc.NewElement("ActionSet");
				xml_actionSet->SetAttribute("enabled", as.enabled);
				xml_actionSet->SetAttribute("name", as.name.c_str());
				xml_actionSet->SetAttribute("joystickName", as.joystickName.c_str());
				xml_actionSet->SetAttribute("joystickGUID", as.joystickGUID.c_str());
				XMLElement *xml_joyAxes = doc.NewElement("JoyAxes");
				{
					xml_joyAxes->SetAttribute("s1ax", as.joycfg.s1ax);
					xml_joyAxes->SetAttribute("s1ay", as.joycfg.s1ay);
					xml_joyAxes->SetAttribute("s2ax", as.joycfg.s2ax);
					xml_joyAxes->SetAttribute("s2ay", as.joycfg.s2ay);
					xml_joyAxes->SetAttribute("s1dead", as.joycfg.s1dead);
					xml_joyAxes->SetAttribute("s2dead", as.joycfg.s2dead);
				}
				xml_actionSet->InsertEndChild(xml_joyAxes);
				for (size_t i = 0; i < as.inputSet.size(); i++)
				{
					XMLElement *xml_action = doc.NewElement("Action");
					const ActionInput& ai = as.inputSet[i];
					xml_action->SetAttribute("name", ai.name.c_str());
					xml_action->SetAttribute("input", ai.toString().c_str());

					xml_actionSet->InsertEndChild(xml_action);
				}
				xml_control->InsertEndChild(xml_actionSet);
			}
		}
		doc.InsertEndChild(xml_control);

		XMLElement *xml_demo = doc.NewElement("Demo");
		{
			XMLElement *xml_warpKeys = doc.NewElement("WarpKeys");
			{
				xml_warpKeys->SetAttribute("on", demo.warpKeys);
			}
			xml_demo->InsertEndChild(xml_warpKeys);

			XMLElement *xml_intro = doc.NewElement("Intro2");
			{
				xml_intro->SetAttribute("on", demo.intro);
			}
			xml_demo->InsertEndChild(xml_intro);

			XMLElement *xml_shortLogos = doc.NewElement("ShortLogos");
			{
				xml_shortLogos->SetAttribute("on", demo.shortLogos);
			}
			xml_demo->InsertEndChild(xml_shortLogos);
		}
		doc.InsertEndChild(xml_demo);

		XMLElement *xml_data = doc.NewElement("Data");
		{
			xml_data->SetAttribute("savePage", (unsigned int) data.savePage);
			xml_data->SetAttribute("saveSlot", (unsigned int) data.saveSlot);

			std::ostringstream ss;
			for (std::vector<std::string>::iterator it = dsq->activePatches.begin(); it != dsq->activePatches.end(); ++it)
				ss << *it << " ";
			xml_data->SetAttribute("activePatches",	ss.str().c_str());
		}
		doc.InsertEndChild(xml_data);

		XMLElement *xml_net = doc.NewElement("Network");
		{
			xml_net->SetAttribute("masterServer", network.masterServer.c_str());
		}
		doc.InsertEndChild(xml_net);

	}

#if defined(BBGE_BUILD_UNIX)
	doc.SaveFile((dsq->getPreferencesFolder() + "/" + userSettingsFilename).c_str());
#elif defined(BBGE_BUILD_WINDOWS)
	doc.SaveFile(userSettingsFilename.c_str());
#endif
}

static void ensureDefaultActions(ActionSet& as)
{
	as.clearActions();
	as.addActionInput("PrimaryAction");
	as.addActionInput("SecondaryAction");
	as.addActionInput("SwimUp");
	as.addActionInput("SwimDown");
	as.addActionInput("SwimLeft");
	as.addActionInput("SwimRight");
	as.addActionInput("Roll");
	as.addActionInput("Revert");
	as.addActionInput("WorldMap");
	as.addActionInput("Escape");
	as.addActionInput("PrevPage");
	as.addActionInput("NextPage");
	as.addActionInput("CookFood");
	as.addActionInput("FoodLeft");
	as.addActionInput("FoodRight");
	as.addActionInput("FoodDrop");
	as.addActionInput("Look");
	as.addActionInput("ToggleHelp");
	as.addActionInput("Screenshot");
	for(int i = 1; i <= 10; ++i)
	{
		std::ostringstream os;
		os << "SongSlot" << i;
		as.addActionInput(os.str());
	}
}

static void readInt(XMLElement *xml, const char *elem, const char *att, int *toChange)
{
	if (xml)
	{
		XMLElement *xml2 = xml->FirstChildElement(elem);
		if (xml2) xml2->QueryIntAttribute(att, toChange);
	}
}

bool UserSettings::loadDefaults(bool doApply)
{
	std::string fn = "default_usersettings.xml";
	if (exists(fn))
	{
		return load(doApply, fn);
	}

	return false;
}

bool UserSettings::load(bool doApply, const std::string &overrideFile)
{
	std::string filename;

	if (!overrideFile.empty())
		filename = overrideFile;
	else
	{
#if defined(BBGE_BUILD_UNIX)
		filename = dsq->getPreferencesFolder() + "/" + userSettingsFilename;
#else
		filename = userSettingsFilename;
#endif
	}

	if(!exists(filename))
		return false;

	XMLDocument doc;
	if(readXML(filename, doc) != XML_SUCCESS)
	{
		errorLog("UserSettings [" + filename + "]: Error, malformed XML");
		return false;
	}

	version.settingsVersion = 0;

	XMLElement *xml_version = doc.FirstChildElement("Version");
	if (xml_version)
	{
		version.settingsVersion = xml_version->IntAttribute("settingsVersion");
	}

	XMLElement *xml_system = doc.FirstChildElement("System");
	if (xml_system)
	{
		XMLElement *xml_debugLog = xml_system->FirstChildElement("DebugLog");
		if (xml_debugLog)
		{
			system.debugLogOn = xml_debugLog->IntAttribute("on");
		}

		XMLElement *xml_locale = xml_system->FirstChildElement("Locale");
		if (xml_locale)
		{
			system.locale = xml_locale->Attribute("name");
		}

		XMLElement *xml_devmode = xml_system->FirstChildElement("DeveloperMode");
		if (xml_devmode)
		{
			system.devModeOn = xml_devmode->IntAttribute("on");
		}

		XMLElement *xml_unsafe = xml_system->FirstChildElement("AllowDangerousScriptFunctions");
		if (xml_unsafe)
		{
			system.allowDangerousScriptFunctions = xml_unsafe->IntAttribute("on");
		}

		XMLElement *xml_grabInp = xml_system->FirstChildElement("GrabInput");
		if (xml_grabInp)
		{
			system.grabInput = xml_grabInp->IntAttribute("on");
		}
	}

	XMLElement *xml_audio = doc.FirstChildElement("Audio");
	if (xml_audio)
	{
		XMLElement *xml_volume = xml_audio->FirstChildElement("Volume");
		if (xml_volume)
		{
			audio.sfxvol = xml_volume->FloatAttribute("sfx");
			audio.voxvol = xml_volume->FloatAttribute("vox");
			audio.musvol = xml_volume->FloatAttribute("mus");
			audio.subtitles = xml_volume->IntAttribute("subs");
		}

		XMLElement *xml_device = xml_audio->FirstChildElement("Device");
		if (xml_device)
		{
			audio.deviceName = xml_device->Attribute("name");
		}

		XMLElement *xml_prebuf = xml_audio->FirstChildElement("Prebuffer");
		if (xml_prebuf)
		{
			audio.prebuffer = xml_prebuf->IntAttribute("on");
		}
	}
	XMLElement *xml_video = doc.FirstChildElement("Video");
	if (xml_video)
	{
		readInt(xml_video, "Blur", "on", &video.blur);

		readInt(xml_video, "NoteEffects", "on", &video.noteEffects);

		readInt(xml_video, "FpsSmoothing", "v", &video.fpsSmoothing);

		readInt(xml_video, "NumParticles", "v", &video.numParticles);

		XMLElement *xml_screenMode = xml_video->FirstChildElement("ScreenMode");
		if (xml_screenMode)
		{
			xml_screenMode->QueryIntAttribute("resx",			&video.resx);
			xml_screenMode->QueryIntAttribute("resy",			&video.resy);
			xml_screenMode->QueryIntAttribute("hz",				&video.hz);
			xml_screenMode->QueryIntAttribute("bits",			&video.bits);
			xml_screenMode->QueryIntAttribute("fbuffer",		&video.fbuffer);
			xml_screenMode->QueryIntAttribute("full",			&video.full);
			xml_screenMode->QueryIntAttribute("vsync",			&video.vsync);
			xml_screenMode->QueryIntAttribute("darkfbuffer",	&video.darkfbuffer);
			xml_screenMode->QueryIntAttribute("darkbuffersize",	&video.darkbuffersize);
			xml_screenMode->QueryIntAttribute("displayindex",	&video.displayindex);
		}

		readInt(xml_video, "SaveSlotScreens", "on", &video.saveSlotScreens);

		readInt(xml_video, "WorldMap", "revealMethod", &video.worldMapRevealMethod);
	}

	XMLElement *xml_control = doc.FirstChildElement("Control");
	if (xml_control)
	{
		readInt(xml_control, "JoystickEnabled", "on", &control.joystickEnabled);
		readInt(xml_control, "AutoAim", "on", &control.autoAim);
		readInt(xml_control, "Targeting", "on", &control.targeting);
		readInt(xml_control, "FlipInputButtons", "on", &control.flipInputButtons);
		readInt(xml_control, "ToolTipsOn", "on", &control.toolTipsOn);

		control.actionSets.clear();

		for(XMLElement *xml_actionSet = xml_control->FirstChildElement("ActionSet"); xml_actionSet; xml_actionSet = xml_actionSet->NextSiblingElement("ActionSet"))
		{
			control.actionSets.push_back(ActionSet());
			ActionSet& as = control.actionSets.back();
			ensureDefaultActions(as);

			if(const char *s = xml_actionSet->Attribute("name"))
				as.name = s;
			if(const char *s = xml_actionSet->Attribute("joystickName"))
				as.joystickName = s;
			if(const char *s = xml_actionSet->Attribute("joystickGUID"))
				as.joystickGUID = s;
			as.enabled = xml_actionSet->BoolAttribute("enabled");

			if(XMLElement *xml_joyAxes = xml_actionSet->FirstChildElement("JoyAxes"))
			{
				as.joycfg.s1ax = xml_joyAxes->IntAttribute("s1ax");
				as.joycfg.s1ay = xml_joyAxes->IntAttribute("s1ay");
				as.joycfg.s2ax = xml_joyAxes->IntAttribute("s2ax");
				as.joycfg.s2ay = xml_joyAxes->IntAttribute("s2ay");
				as.joycfg.s1dead = xml_joyAxes->FloatAttribute("s1dead");
				as.joycfg.s2dead = xml_joyAxes->FloatAttribute("s2dead");
			}

			for(XMLElement *xml_action = xml_actionSet->FirstChildElement(); xml_action; xml_action = xml_action->NextSiblingElement())
			{
				const char *name = xml_action->Attribute("name");
				const char *input = xml_action->Attribute("input");
				if (name && *name && input && *input)
				{
					ActionInput *ai = as.addActionInput(name);
					ai->fromString(input);
				}
			}
		}
	}

	if(control.actionSets.empty())
		control.actionSets.resize(1);

	if(control.actionSets.size() == 1)
		control.actionSets[0].enabled = true;

	XMLElement *xml_demo = doc.FirstChildElement("Demo");
	if (xml_demo)
	{
		readInt(xml_demo, "WarpKeys", "on", &demo.warpKeys);
		readInt(xml_demo, "Intro2", "on", &demo.intro);
		readInt(xml_demo, "ShortLogos", "on", &demo.shortLogos);
	}

	XMLElement *xml_data = doc.FirstChildElement("Data");
	if (xml_data)
	{
		// use a temporary variable so we don't get into trouble on big-endian architectures
		unsigned int tmp;
		xml_data->QueryUnsignedAttribute("savePage", &tmp);
		data.savePage = tmp;
		xml_data->QueryUnsignedAttribute("saveSlot", &tmp);
		data.saveSlot = tmp;

		if(const char *patchlist = xml_data->Attribute("activePatches"))
		{
			SimpleIStringStream ss(patchlist, SimpleIStringStream::REUSE);
			std::string tmp;
			while(ss)
			{
				ss >> tmp;
				if(tmp.length() && !dsq->isPatchActive(tmp))
					dsq->activePatches.push_back(tmp);
			}
		}
	}

	XMLElement *xml_net = doc.FirstChildElement("Network");
	if (xml_net)
	{
		const char *serv = xml_net->Attribute("masterServer");
		if (serv)
			network.masterServer = serv;
	}



	if (system.locale.empty())
	{
		std::string loc = getSystemLocale();
		debugLog("Using autodetected system locale: " + loc);
		setUsedLocale(loc);
	}
	else
	{
		debugLog("Using user config locale: " + system.locale);
		setUsedLocale(system.locale);
	}

	core->initLocalization();

	if (doApply)
		apply();

	return true;
}

void UserSettings::apply()
{
	core->sound->setMusicVolume(audio.musvol);
	core->sound->setSfxVolume(audio.sfxvol);
	core->sound->setVoiceVolume(audio.voxvol);

	core->flipMouseButtons = control.flipInputButtons;

	dsq->loops.updateVolume();

	for(size_t i = 0; i < control.actionSets.size(); ++i)
	{
		ActionSet& as = control.actionSets[i];
		Joystick *j = core->getJoystick(as.joystickID);
		if(j)
		{
			j->s1ax = as.joycfg.s1ax;
			j->s1ay = as.joycfg.s1ay;
			j->s2ax = as.joycfg.s2ax;
			j->s2ay = as.joycfg.s2ay;
			j->deadZone1 = as.joycfg.s1dead;
			j->deadZone2 = as.joycfg.s2dead;
		}
	}
	dsq->initActionButtons();
	dsq->fixupJoysticks();

	core->debugLogActive = system.debugLogOn;

	if (game)
	{
		game->bindInput();
	}

	dsq->bindInput();

	core->settings.prebufferSounds = audio.prebuffer;
}


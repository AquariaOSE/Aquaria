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

			XMLElement *xml_parallax = doc.NewElement("Parallax");
			std::ostringstream os;
			os << video.parallaxOn0 << " " << video.parallaxOn1 << " " << video.parallaxOn2;
			xml_parallax->SetAttribute("on", os.str().c_str());
			xml_video->InsertEndChild(xml_parallax);

			XMLElement *xml_numParticles = doc.NewElement("NumParticles");
			xml_numParticles->SetAttribute("v", video.numParticles);
			xml_video->InsertEndChild(xml_numParticles);

			XMLElement *xml_screenMode = doc.NewElement("ScreenMode");
			{
				xml_screenMode->SetAttribute("resx",				video.resx);
				xml_screenMode->SetAttribute("resy",				video.resy);
				xml_screenMode->SetAttribute("bits",				video.bits);
				xml_screenMode->SetAttribute("fbuffer",			video.fbuffer);
				xml_screenMode->SetAttribute("full",				video.full);
				xml_screenMode->SetAttribute("vsync",			video.vsync);
				xml_screenMode->SetAttribute("darkfbuffer",		video.darkfbuffer);
				xml_screenMode->SetAttribute("darkbuffersize",	video.darkbuffersize);
				xml_screenMode->SetAttribute("displaylists",		video.displaylists);
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

			XMLElement *xml_joyAxes = doc.NewElement("JoyAxes");
			{
				xml_joyAxes->SetAttribute("s1ax", control.s1ax);
				xml_joyAxes->SetAttribute("s1ay", control.s1ay);
				xml_joyAxes->SetAttribute("s2ax", control.s2ax);
				xml_joyAxes->SetAttribute("s2ay", control.s2ay);
				xml_joyAxes->SetAttribute("s1dead", double(control.s1dead));
				xml_joyAxes->SetAttribute("s2dead", double(control.s2dead));
			}
			xml_control->InsertEndChild(xml_joyAxes);

			XMLElement *xml_actionSet = doc.NewElement("ActionSet");
			{
				for (int i = 0; i < control.actionSet.inputSet.size(); i++)
				{
					XMLElement *xml_action = doc.NewElement("Action");
					ActionInput *actionInput = &control.actionSet.inputSet[i];
					xml_action->SetAttribute("name", actionInput->name.c_str());
					xml_action->SetAttribute("input", actionInput->toString().c_str());

					xml_actionSet->InsertEndChild(xml_action);
				}
			}
			xml_control->InsertEndChild(xml_actionSet);
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
			xml_data->SetAttribute("savePage",			data.savePage);
			xml_data->SetAttribute("saveSlot",			data.saveSlot);

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

static void readInt(XMLElement *xml, const char *elem, const char *att, int *toChange)
{
	if (xml)
	{
		XMLElement *xml2 = xml->FirstChildElement(elem);
		if (xml2) xml2->QueryIntAttribute(att, toChange);
	}
}

void UserSettings::loadDefaults(bool doApply)
{
	std::ostringstream os;
	os << "default-" << VERSION_USERSETTINGS << ".xml";
	if (exists(os.str()))
	{
		load(doApply, os.str());
		return;
	}

	if (exists("default_usersettings.xml"))
	{
		load(doApply, "default_usersettings.xml");
		return;
	}

	errorLog("No default user settings file found! Controls may be broken.");
}

void UserSettings::load(bool doApply, const std::string &overrideFile)
{
	std::string filename;

#if defined(BBGE_BUILD_UNIX)
	filename = dsq->getPreferencesFolder() + "/" + userSettingsFilename;
#elif defined(BBGE_BUILD_WINDOWS)
	if (!overrideFile.empty())
		filename = overrideFile;
	else
		filename = userSettingsFilename;
#endif

	XMLDocument doc;
	if(readXML(filename, doc) != XML_SUCCESS)
	{
		errorLog("UserSettings: Malformed XML, continuing with defaults");
		doc.Clear(); // just in case
	}

	version.settingsVersion = 0;

	XMLElement *xml_version = doc.FirstChildElement("Version");
	if (xml_version)
	{
		version.settingsVersion = xml_version->IntAttribute("settingsVersion");
	}

	control.actionSet.clearActions();


	control.actionSet.addActionInput("lmb");
	control.actionSet.addActionInput("rmb");
	control.actionSet.addActionInput("PrimaryAction");
	control.actionSet.addActionInput("SecondaryAction");
	control.actionSet.addActionInput("SwimUp");
	control.actionSet.addActionInput("SwimDown");
	control.actionSet.addActionInput("SwimLeft");
	control.actionSet.addActionInput("SwimRight");
	control.actionSet.addActionInput("Roll");
	control.actionSet.addActionInput("Revert");
	control.actionSet.addActionInput("WorldMap");
	control.actionSet.addActionInput("Escape");
	control.actionSet.addActionInput("PrevPage");
	control.actionSet.addActionInput("NextPage");
	control.actionSet.addActionInput("CookFood");
	control.actionSet.addActionInput("FoodLeft");
	control.actionSet.addActionInput("FoodRight");
	control.actionSet.addActionInput("FoodDrop");
	control.actionSet.addActionInput("Look");
	control.actionSet.addActionInput("ToggleHelp");

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
	}

	XMLElement *xml_audio = doc.FirstChildElement("Audio");
	if (xml_audio)
	{
		XMLElement *xml_volume = xml_audio->FirstChildElement("Volume");
		if (xml_volume)
		{
			audio.sfxvol = xml_volume->DoubleAttribute("sfx");
			audio.voxvol = xml_volume->DoubleAttribute("vox");
			audio.musvol = xml_volume->DoubleAttribute("mus");
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


		XMLElement *xml_parallax = xml_video->FirstChildElement("Parallax");
		if (xml_parallax)
		{
			if (xml_parallax->Attribute("on"))
			{
				std::istringstream is(xml_parallax->Attribute("on"));
				is >> video.parallaxOn0 >> video.parallaxOn1 >> video.parallaxOn2;
			}
		}

		readInt(xml_video, "NumParticles", "v", &video.numParticles);

		XMLElement *xml_screenMode = xml_video->FirstChildElement("ScreenMode");
		if (xml_screenMode)
		{
			xml_screenMode->QueryIntAttribute("resx",			&video.resx);
			xml_screenMode->QueryIntAttribute("resy",			&video.resy);
			xml_screenMode->QueryIntAttribute("bits",			&video.bits);
			xml_screenMode->QueryIntAttribute("fbuffer",		&video.fbuffer);
			xml_screenMode->QueryIntAttribute("full",			&video.full);
			xml_screenMode->QueryIntAttribute("vsync",			&video.vsync);
			xml_screenMode->QueryIntAttribute("darkfbuffer",	&video.darkfbuffer);
			xml_screenMode->QueryIntAttribute("darkbuffersize",	&video.darkbuffersize);
			xml_screenMode->QueryIntAttribute("displaylists",	&video.displaylists);
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

		XMLElement *xml_joyAxes = xml_control->FirstChildElement("JoyAxes");
		if (xml_joyAxes)
		{
			control.s1ax = xml_joyAxes->IntAttribute("s1ax");
			control.s1ay = xml_joyAxes->IntAttribute("s1ay");
			control.s2ax = xml_joyAxes->IntAttribute("s2ax");
			control.s2ay = xml_joyAxes->IntAttribute("s2ay");
			control.s1dead = xml_joyAxes->DoubleAttribute("s1dead");
			control.s2dead = xml_joyAxes->DoubleAttribute("s2dead");
		}

		XMLElement *xml_actionSet = xml_control->FirstChildElement("ActionSet");
		if (xml_actionSet)
		{
			XMLElement *xml_action = 0;
			xml_action = xml_actionSet->FirstChildElement();
			while (xml_action)
			{
				std::string name = xml_action->Attribute("name");

				if (!name.empty())
				{
					ActionInput *ai = control.actionSet.addActionInput(name);

					ai->fromString(xml_action->Attribute("input"));
				}
				xml_action = xml_action->NextSiblingElement();
			}
		}

		readInt(xml_control, "ToolTipsOn", "on", &control.toolTipsOn);
	}

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
		xml_data->QueryIntAttribute("savePage", &data.savePage);
		xml_data->QueryIntAttribute("saveSlot", &data.saveSlot);

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
}

void UserSettings::apply()
{
	core->sound->setMusicVolume(audio.musvol);
	core->sound->setSfxVolume(audio.sfxvol);
	core->sound->setVoiceVolume(audio.voxvol);

	core->flipMouseButtons = control.flipInputButtons;

	dsq->loops.updateVolume();

	// FIXME: This should be per-joystick
	/*core->joystick.s1ax = control.s1ax;
	core->joystick.s1ay = control.s1ay;
	core->joystick.s2ax = control.s2ax;
	core->joystick.s2ay = control.s2ay;

	core->joystick.deadZone1 = control.s1dead;
	core->joystick.deadZone2 = control.s2dead;*/

	core->debugLogActive = system.debugLogOn;

	if (dsq->game)
	{
		dsq->game->bindInput();
	}

	dsq->bindInput();

	core->settings.prebufferSounds = audio.prebuffer;
}


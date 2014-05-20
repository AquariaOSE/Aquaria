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

#ifndef AQUARIA_USERSETTINGS_DATAONLY
	#include "DSQ.h"
	#include "Game.h"
	#include "Avatar.h"
#else
	#include "../ExternalLibs/tinyxml.h"
#endif


void UserSettings::save()
{
	//initInputCodeMap();

	TiXmlDocument doc;
	{
		TiXmlElement xml_version("Version");
		{
			xml_version.SetAttribute("settingsVersion", VERSION_USERSETTINGS);
		}
		doc.InsertEndChild(xml_version);

		TiXmlElement xml_system("System");
		{
			TiXmlElement xml_debugLog("DebugLog");
			{
				xml_debugLog.SetAttribute("on", system.debugLogOn);
			}
			xml_system.InsertEndChild(xml_debugLog);

			TiXmlElement xml_locale("Locale");
			{
				xml_locale.SetAttribute("name", system.locale);
			}
			xml_system.InsertEndChild(xml_locale);

			TiXmlElement xml_devmode("DeveloperMode");
			{
				xml_devmode.SetAttribute("on", system.devModeOn);
			}
			xml_system.InsertEndChild(xml_devmode);

			TiXmlElement xml_unsafe("AllowDangerousScriptFunctions");
			{
				xml_unsafe.SetAttribute("on", system.allowDangerousScriptFunctions);
			}
			xml_system.InsertEndChild(xml_unsafe);
		}
		doc.InsertEndChild(xml_system);

		TiXmlElement xml_audio("Audio");
		{
			TiXmlElement xml_microphone("Mic");
			{
				xml_microphone.SetAttribute("on", audio.micOn);
				xml_microphone.SetAttribute("octave", audio.octave);
			}
			xml_audio.InsertEndChild(xml_microphone);

			TiXmlElement xml_volume("Volume");
			{
				xml_volume.SetDoubleAttribute("sfx", double(audio.sfxvol));
				xml_volume.SetDoubleAttribute("vox", double(audio.voxvol));
				xml_volume.SetDoubleAttribute("mus", double(audio.musvol));
				xml_volume.SetAttribute("subs", audio.subtitles);
			}
			xml_audio.InsertEndChild(xml_volume);

			TiXmlElement xml_device("Device");
			{
				xml_device.SetAttribute("name", audio.deviceName);
			}
			xml_audio.InsertEndChild(xml_device);

			TiXmlElement xml_prebuf("Prebuffer");
			{
				xml_prebuf.SetAttribute("on", audio.prebuffer);
			}
			xml_audio.InsertEndChild(xml_prebuf);
		}
		doc.InsertEndChild(xml_audio);

		TiXmlElement xml_video("Video");
		{
			TiXmlElement xml_blur("Blur");
			{
				xml_blur.SetAttribute("on", video.blur);
			}
			xml_video.InsertEndChild(xml_blur);

			TiXmlElement xml_noteEffects("NoteEffects");
			{
				xml_noteEffects.SetAttribute("on", video.noteEffects);
			}
			xml_video.InsertEndChild(xml_noteEffects);

			TiXmlElement xml_fpsSmoothing("FpsSmoothing");
			{
				xml_fpsSmoothing.SetAttribute("v", video.fpsSmoothing);
			}
			xml_video.InsertEndChild(xml_fpsSmoothing);

			TiXmlElement xml_parallax("Parallax");
			std::ostringstream os;
			os << video.parallaxOn0 << " " << video.parallaxOn1 << " " << video.parallaxOn2;
			xml_parallax.SetAttribute("on", os.str());
			xml_video.InsertEndChild(xml_parallax);

			TiXmlElement xml_numParticles("NumParticles");
			xml_numParticles.SetAttribute("v", video.numParticles);
			xml_video.InsertEndChild(xml_numParticles);

			TiXmlElement xml_screenMode("ScreenMode");
			{
				xml_screenMode.SetAttribute("resx",				video.resx);
				xml_screenMode.SetAttribute("resy",				video.resy);
				xml_screenMode.SetAttribute("bits",				video.bits);
				xml_screenMode.SetAttribute("fbuffer",			video.fbuffer);
				xml_screenMode.SetAttribute("full",				video.full);
				xml_screenMode.SetAttribute("vsync",			video.vsync);
				xml_screenMode.SetAttribute("darkfbuffer",		video.darkfbuffer);
				xml_screenMode.SetAttribute("darkbuffersize",	video.darkbuffersize);
				xml_screenMode.SetAttribute("displaylists",		video.displaylists);
			}
			xml_video.InsertEndChild(xml_screenMode);

			TiXmlElement xml_saveSlotScreens("SaveSlotScreens");
			{
				xml_saveSlotScreens.SetAttribute("on", video.saveSlotScreens);
			}
			xml_video.InsertEndChild(xml_saveSlotScreens);

			TiXmlElement xml_worldMap("WorldMap");
			{
				xml_worldMap.SetAttribute("revealMethod", video.worldMapRevealMethod);
			}
			xml_video.InsertEndChild(xml_worldMap);
		}
		doc.InsertEndChild(xml_video);


		TiXmlElement xml_control("Control");
		{
			TiXmlElement xml_toolTipsOn("ToolTipsOn");
			{
				xml_toolTipsOn.SetAttribute("on", control.toolTipsOn);
			}
			xml_control.InsertEndChild(xml_toolTipsOn);

			TiXmlElement xml_joystickEnabled("JoystickEnabled");
			{
				xml_joystickEnabled.SetAttribute("on", control.joystickEnabled);
			}
			xml_control.InsertEndChild(xml_joystickEnabled);

			TiXmlElement xml_autoAim("AutoAim");
			{
				xml_autoAim.SetAttribute("on", control.autoAim);
			}
			xml_control.InsertEndChild(xml_autoAim);

			TiXmlElement xml_targeting("Targeting");
			{
				xml_targeting.SetAttribute("on", control.targeting);
			}
			xml_control.InsertEndChild(xml_targeting);

			TiXmlElement xml_joyCursorSpeed("JoyCursorSpeed");
			{
				xml_joyCursorSpeed.SetDoubleAttribute("v", double(control.joyCursorSpeed));
			}
			xml_control.InsertEndChild(xml_joyCursorSpeed);

			TiXmlElement xml_joyAxes("JoyAxes");
			{
				xml_joyAxes.SetAttribute("s1ax", control.s1ax);
				xml_joyAxes.SetAttribute("s1ay", control.s1ay);
				xml_joyAxes.SetAttribute("s2ax", control.s2ax);
				xml_joyAxes.SetAttribute("s2ay", control.s2ay);
				xml_joyAxes.SetDoubleAttribute("s1dead", double(control.s1dead));
				xml_joyAxes.SetDoubleAttribute("s2dead", double(control.s2dead));
			}
			xml_control.InsertEndChild(xml_joyAxes);

			TiXmlElement xml_actionSet("ActionSet");
			{
				for (int i = 0; i < control.actionSet.inputSet.size(); i++)
				{
					TiXmlElement xml_action("Action");
					ActionInput *actionInput = &control.actionSet.inputSet[i];
					xml_action.SetAttribute("name", actionInput->name);
					xml_action.SetAttribute("input", actionInput->toString());

					xml_actionSet.InsertEndChild(xml_action);
				}
			}
			xml_control.InsertEndChild(xml_actionSet);
		}
		doc.InsertEndChild(xml_control);

		TiXmlElement xml_demo("Demo");
		{
			TiXmlElement xml_warpKeys("WarpKeys");
			{
				xml_warpKeys.SetAttribute("on", demo.warpKeys);
			}
			xml_demo.InsertEndChild(xml_warpKeys);

			TiXmlElement xml_intro("Intro2");
			{
				xml_intro.SetAttribute("on", demo.intro);
			}
			xml_demo.InsertEndChild(xml_intro);

			TiXmlElement xml_shortLogos("ShortLogos");
			{
				xml_shortLogos.SetAttribute("on", demo.shortLogos);
			}
			xml_demo.InsertEndChild(xml_shortLogos);
		}
		doc.InsertEndChild(xml_demo);

		TiXmlElement xml_data("Data");
		{
			xml_data.SetAttribute("savePage",			data.savePage);
			xml_data.SetAttribute("saveSlot",			data.saveSlot);

			std::ostringstream ss;
			for (std::set<std::string>::iterator it = dsq->activePatches.begin(); it != dsq->activePatches.end(); ++it)
				ss << *it << " ";
			xml_data.SetAttribute("activePatches",	ss.str());
		}
		doc.InsertEndChild(xml_data);

		TiXmlElement xml_net("Network");
		{
			xml_net.SetAttribute("masterServer",		network.masterServer);
		}
		doc.InsertEndChild(xml_net);

	}

#if defined(BBGE_BUILD_UNIX)
	doc.SaveFile(dsq->getPreferencesFolder() + "/" + userSettingsFilename);
#elif defined(BBGE_BUILD_WINDOWS)
	doc.SaveFile(userSettingsFilename);
#endif

	//clearInputCodeMap();
}

void readInt(TiXmlElement *xml, const std::string &elem, std::string att, int *toChange)
{
	if (xml)
	{
		TiXmlElement *xml2 = xml->FirstChildElement(elem);
		if (xml2)
		{
			const std::string *s = xml2->Attribute(att);
			if (s) {
				const char *c = s->c_str();
				if (c)
				{
					*toChange = atoi(c);
				}
			}
		}
	}
}

void readIntAtt(TiXmlElement *xml, std::string att, int *toChange)
{
	if (xml)
	{
		const std::string *s = xml->Attribute(att);
		if (s) {
			const char *c = s->c_str();
			if (c)
			{
				*toChange = atoi(c);
			}
		}
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
	TiXmlDocument doc;

#if defined(BBGE_BUILD_UNIX)
	doc.LoadFile(dsq->getPreferencesFolder() + "/" + userSettingsFilename);
#elif defined(BBGE_BUILD_WINDOWS)
	if (!overrideFile.empty())
		doc.LoadFile(overrideFile);
	else
		doc.LoadFile(userSettingsFilename);
#endif

	version.settingsVersion = 0;

	TiXmlElement *xml_version = doc.FirstChildElement("Version");
	if (xml_version)
	{
		xml_version->Attribute("settingsVersion", &version.settingsVersion);
	}

	control.actionSet.clearActions();
	//initInputCodeMap();

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

	TiXmlElement *xml_system = doc.FirstChildElement("System");
	if (xml_system)
	{
		TiXmlElement *xml_debugLog = xml_system->FirstChildElement("DebugLog");
		if (xml_debugLog)
		{
			xml_debugLog->Attribute("on", &system.debugLogOn);
		}

		TiXmlElement *xml_locale = xml_system->FirstChildElement("Locale");
		if (xml_locale)
		{
			system.locale = xml_locale->Attribute("name");
		}

		TiXmlElement *xml_devmode = xml_system->FirstChildElement("DeveloperMode");
		if (xml_devmode)
		{
			xml_devmode->Attribute("on", &system.devModeOn);
		}

		TiXmlElement *xml_unsafe = xml_system->FirstChildElement("AllowDangerousScriptFunctions");
		if (xml_unsafe)
		{
			xml_unsafe->Attribute("on", &system.allowDangerousScriptFunctions);
		}
	}

	TiXmlElement *xml_audio = doc.FirstChildElement("Audio");
	if (xml_audio)
	{
		TiXmlElement *xml_microphone = xml_audio->FirstChildElement("Mic");
		if (xml_microphone)
		{
			xml_microphone->Attribute("on", &audio.micOn);
			xml_microphone->Attribute("octave", &audio.octave);
		}

		TiXmlElement *xml_volume = xml_audio->FirstChildElement("Volume");
		if (xml_volume)
		{
			double d;
			xml_volume->Attribute("sfx", &d), audio.sfxvol = d;
			xml_volume->Attribute("vox", &d), audio.voxvol = d;
			xml_volume->Attribute("mus", &d), audio.musvol = d;
			xml_volume->Attribute("subs", &audio.subtitles);
		}

		TiXmlElement *xml_device = xml_audio->FirstChildElement("Device");
		if (xml_device)
		{
			audio.deviceName = xml_device->Attribute("name");
		}

		TiXmlElement *xml_prebuf = xml_audio->FirstChildElement("Prebuffer");
		if (xml_prebuf)
		{
			xml_prebuf->Attribute("on", &audio.prebuffer);
		}
	}
	TiXmlElement *xml_video = doc.FirstChildElement("Video");
	if (xml_video)
	{
		readInt(xml_video, "Blur", "on", &video.blur);

		readInt(xml_video, "NoteEffects", "on", &video.noteEffects);

		readInt(xml_video, "FpsSmoothing", "v", &video.fpsSmoothing);

		/*
		readInt(xml_video, "Parallax", "on", &video.parallaxOn);
		*/
		TiXmlElement *xml_parallax = xml_video->FirstChildElement("Parallax");
		if (xml_parallax)
		{
			if (xml_parallax->Attribute("on"))
			{
				std::istringstream is(xml_parallax->Attribute("on"));
				is >> video.parallaxOn0 >> video.parallaxOn1 >> video.parallaxOn2;
			}
		}

		readInt(xml_video, "NumParticles", "v", &video.numParticles);

		TiXmlElement *xml_screenMode = xml_video->FirstChildElement("ScreenMode");
		if (xml_screenMode)
		{
			readIntAtt(xml_screenMode, "resx",				&video.resx);
			readIntAtt(xml_screenMode, "resy",				&video.resy);
			readIntAtt(xml_screenMode, "bits",				&video.bits);
			readIntAtt(xml_screenMode, "fbuffer",			&video.fbuffer);
			readIntAtt(xml_screenMode, "full",				&video.full);
			readIntAtt(xml_screenMode, "vsync",				&video.vsync);
			readIntAtt(xml_screenMode, "darkfbuffer",		&video.darkfbuffer);
			readIntAtt(xml_screenMode, "darkbuffersize",	&video.darkbuffersize);
			readIntAtt(xml_screenMode, "displaylists",		&video.displaylists);
		}

		readInt(xml_video, "SaveSlotScreens", "on", &video.saveSlotScreens);

		readInt(xml_video, "WorldMap", "revealMethod", &video.worldMapRevealMethod);
	}

	TiXmlElement *xml_control = doc.FirstChildElement("Control");
	if (xml_control)
	{
		readInt(xml_control, "JoystickEnabled", "on", &control.joystickEnabled);

		readInt(xml_control, "AutoAim", "on", &control.autoAim);

		readInt(xml_control, "Targeting", "on", &control.targeting);

		TiXmlElement *xml_joyCursorSpeed = xml_control->FirstChildElement("JoyCursorSpeed");
		if (xml_joyCursorSpeed)
		{
			double d;
			if (xml_joyCursorSpeed->Attribute("v"))
				xml_joyCursorSpeed->Attribute("v", &d), control.joyCursorSpeed = d;
		}

		TiXmlElement *xml_joyAxes = xml_control->FirstChildElement("JoyAxes");
		if (xml_joyAxes)
		{
			xml_joyAxes->Attribute("s1ax", &control.s1ax);
			xml_joyAxes->Attribute("s1ay", &control.s1ay);
			xml_joyAxes->Attribute("s2ax", &control.s2ax);
			xml_joyAxes->Attribute("s2ay", &control.s2ay);
			double d;
			xml_joyAxes->Attribute("s1dead", &d), control.s1dead = d;
			xml_joyAxes->Attribute("s2dead", &d), control.s2dead = d;
		}

		TiXmlElement *xml_actionSet = xml_control->FirstChildElement("ActionSet");
		if (xml_actionSet)
		{
			TiXmlElement *xml_action = 0;
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

	TiXmlElement *xml_demo = doc.FirstChildElement("Demo");
	if (xml_demo)
	{
		readInt(xml_demo, "WarpKeys", "on", &demo.warpKeys);
		readInt(xml_demo, "Intro2", "on", &demo.intro);
		readInt(xml_demo, "ShortLogos", "on", &demo.shortLogos);
	}

	TiXmlElement *xml_data = doc.FirstChildElement("Data");
	if (xml_data)
	{
		readIntAtt(xml_data, "savePage", &data.savePage);
		readIntAtt(xml_data, "saveSlot", &data.saveSlot);

		if(const char *patchlist = xml_data->Attribute("activePatches"))
		{
			SimpleIStringStream ss(patchlist, SimpleIStringStream::REUSE);
			std::string tmp;
			while(ss)
			{
				ss >> tmp;
				if(tmp.length())
					dsq->activePatches.insert(tmp);
			}
		}
	}

	TiXmlElement *xml_net = doc.FirstChildElement("Network");
	if (xml_net)
	{
		const char *serv = xml_net->Attribute("masterServer");
		if (serv)
			network.masterServer = serv;
	}

	//clearInputCodeMap();

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
#ifndef AQUARIA_USERSETTINGS_DATAONLY
	core->sound->setMusicVolume(audio.musvol);
	core->sound->setSfxVolume(audio.sfxvol);
	core->sound->setVoiceVolume(audio.voxvol);

	core->flipMouseButtons = control.flipInputButtons;

	dsq->loops.updateVolume();

	core->joystick.s1ax = control.s1ax;
	core->joystick.s1ay = control.s1ay;
	core->joystick.s2ax = control.s2ax;
	core->joystick.s2ay = control.s2ay;

	core->joystick.deadZone1 = control.s1dead;
	core->joystick.deadZone2 = control.s2dead;

	core->debugLogActive = system.debugLogOn;

	if (dsq->game)
	{
		dsq->game->bindInput();

		if (dsq->game->avatar)
		{
			dsq->game->avatar->updateHeartbeatSfx();
		}
	}

	dsq->bindInput();

	core->settings.prebufferSounds = audio.prebuffer;

#endif
}


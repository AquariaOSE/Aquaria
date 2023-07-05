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
#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include <string>

const std::string userSettingsFilename = "usersettings.xml";

#include "Base.h"
#include "ActionMapper.h"


// MAKE SURE to update this when changing the user settings
const int VERSION_USERSETTINGS	= 1;

class UserSettings
{
public:
	struct System
	{
		System() { debugLogOn = 0; devModeOn = 0; allowDangerousScriptFunctions = 0; grabInput=1; }
		int debugLogOn;
		std::string locale;
		int devModeOn;
		int allowDangerousScriptFunctions;
		int grabInput;
	} system;

	struct Audio
	{
		Audio() { musvol=voxvol=sfxvol=1.0; subtitles=false; prebuffer=false;}
		float voxvol, sfxvol, musvol;
		int subtitles;
		std::string deviceName;
		int prebuffer;
	} audio;

	struct Video
	{
		Video() {
			numParticles = 2048;
			saveSlotScreens = 1;
			blur = 1;
			noteEffects = 0;
			fpsSmoothing = 30;
			resx = 800;
			resy = 600;
			hz = 60;
			displayindex = 0;
			full = 1;
			fbuffer = 1;
			darkfbuffer = 1;
			bits = 32;
			vsync = 1;
			darkbuffersize = 256;
			worldMapRevealMethod = 0;
		}
		int blur;
		int noteEffects;
		int fpsSmoothing;
		int resx, resy, full, fbuffer, bits, vsync, darkfbuffer, darkbuffersize, hz, displayindex;
		int saveSlotScreens;
		int numParticles;
		int worldMapRevealMethod;
	} video;

	struct Control
	{
		Control() {
			toolTipsOn = 1;
			autoAim = 1;
			targeting = 1;
			flipInputButtons = 0;
			joystickEnabled = 0;
		}
		int joystickEnabled;
		int autoAim;
		int targeting;
		int flipInputButtons;
		std::vector<ActionSet> actionSets;
		int toolTipsOn;
	} control;

	struct Demo
	{
		Demo() { warpKeys=0; intro=0; shortLogos=0; }
		int warpKeys;
		int intro;
		int shortLogos;
	} demo;

	struct Data
	{
		Data() { savePage=0; saveSlot=0; }
		size_t savePage;
		size_t saveSlot;
	} data;

	struct Version
	{
		Version() { settingsVersion=1; }
		int settingsVersion;
	} version;

	struct Network
	{
		std::string masterServer;
	} network;

	bool loadDefaults(bool doApply=true);
	bool load(bool doApply=true, const std::string &overrideFile="");
	void save();
	void apply();
};

#endif

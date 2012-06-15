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
#pragma once

#include <string>

const std::string userSettingsFilename = "usersettings.xml";

#ifndef AQUARIA_USERSETTINGS_DATAONLY

	#include "Base.h"
	#include "ActionMapper.h"

#else

	#include <string>
	#include <vector>
	#include <sstream>

	class ActionInput
	{
	public:
		std::string toString()
		{
			return input;
		}
		void fromString(const std::string &str)
		{
			input = str;
		}

		std::string name, input;
	};

	class ActionSet
	{
	public:
		void clearActions()
		{
			inputSet.clear();
		}
		ActionInput* addActionInput(const std::string &name)
		{
			ActionInput newActionInput;
			newActionInput.name = name;
			inputSet.push_back(newActionInput);
			return &inputSet[inputSet.size()-1];
		}
		std::vector<ActionInput> inputSet;
	};

#endif

// MAKE SURE to update this when changing the user settings
const int VERSION_USERSETTINGS	= 1;

class UserSettings
{
public:
	struct System
	{
		System() { debugLogOn = 0; isSystemLocale = false; }
		int debugLogOn;
		bool isSystemLocale;
		std::string locale;
	} system;

	struct Audio
	{
		Audio() { micOn = 0; octave=0; musvol=voxvol=sfxvol=1.0; subtitles=false; prebuffer=false;}
		int micOn;
		int octave;
		float voxvol, sfxvol, musvol;
		int subtitles;
		std::string deviceName;
		int prebuffer;
	} audio;

	struct Video
	{
		Video() {
			numParticles = 2048;
			parallaxOn0 = parallaxOn1 = parallaxOn2 = 1;
			saveSlotScreens = 1;
			shader = 0;
			blur = 1;
			noteEffects = 0;
			fpsSmoothing = 30;
			resx = 800;
			resy = 600;
			full = 1;
			fbuffer = 1;
			darkfbuffer = 1;
			bits = 32;
			vsync = 1;
			darkbuffersize = 256;
			displaylists = 0;
		}
		int shader;
		int blur;
		int noteEffects;
		int fpsSmoothing;
		int resx, resy, full, fbuffer, bits, vsync, darkfbuffer, darkbuffersize;
		int saveSlotScreens;
		int parallaxOn0, parallaxOn1, parallaxOn2;
		int numParticles;
		int displaylists;
	} video;

	struct Control
	{
		Control() {
			toolTipsOn = 1;
			autoAim = 1;
			targeting = 1;
			joyCursorSpeed = 4.0;
			flipInputButtons = 0;
			s1ax = 0;
			s1ay = 0;
			s2ax = 0;
			s2ay = 0;
			s1dead = 0.3;
			s2dead = 0.3;
			joystickEnabled = 0;
		}
		int joystickEnabled;
		int autoAim;
		int targeting;
		float joyCursorSpeed;
		int flipInputButtons;
		ActionSet actionSet;
		int s1ax, s1ay, s2ax, s2ay;
		float s1dead, s2dead;
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
		int savePage;
		int saveSlot;
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

	void loadDefaults(bool doApply=true);
	void load(bool doApply=true, const std::string &overrideFile="");
	void save();
	void apply();
	std::string localisePath(const std::string &path, const std::string &modpath="");

private:
	void getSystemLocale();
};

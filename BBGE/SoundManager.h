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
#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <stdlib.h>
#include <string>
#include <list>
#include <queue>
#include <set>
#include "Vector.h"


#define BBGE_AUDIO_NOCHANNEL NULL

const int BBGE_AUDIO_LOOPINFINITE	= -1;
const int BBGE_AUDIO_LOOPNONE		= 0;

namespace SoundCore
{
	typedef void *Buffer;
}

enum SoundEffectType
{
	SFX_NONE		= -1,
	SFX_FLANGE		= 0,
	SFX_MAX
};

enum SoundLoopType
{
	SLT_NONE	= -1,
	SLT_NORMAL	= 0,
	SLT_LOOP	= 0,
	SLT_OFF		= 1,
	SLT_BI		= 2
};

enum SoundFadeType
{
	SFT_NONE	= -1,
	SFT_IN		= 0,
	SFT_CROSS	= 1,
	SFT_OUT		= 2
};

enum SoundConditionType
{
	SCT_NONE			= -1,
	SCT_NORMAL			= 0,
	SCT_ISNOTPLAYING	= 1
};

enum SoundVoiceType
{
	SVT_QUEUE			= 0,
	SVT_INTERRUPT		= 1
};

enum SoundLoadType
{
	SFXLOAD_CACHE		= 0,
	SFXLOAD_LOCAL		= 1
};

struct PlaySfx
{
	PlaySfx() : priority(0.5), handle(0), vol(1), fade(SFT_NONE),
		time(0), freq(1), loops(0),
		maxdist(0), x(0), y(0), relative(true), positional(false) {}

	std::string name;
	intptr_t handle;
	float vol;
	float time;
	float freq;
	int loops;
	float priority;
	float maxdist; // distance gain attenuation. if 0: use default value, -1: don't attenuate at all
	SoundFadeType fade;
	float x, y;
	bool relative; // relative to listener?
	bool positional; // if true, this indicates that we want positional sound (stereo will be downmixed to mono to make OpenAL happy)
};

class SoundHolder; // defined below

class SoundManager
{
public:
	SoundManager(const std::string &defaultDevice="");
	~SoundManager();

	void stopAll();

	void setChannelVolume(void *chan, float v);

	void loadSoundCache(const std::string &spath="sfx/cache/", const std::string &ftype=".ogg", void progressCallback()=NULL);

	void stopAllSfx();

	void clearLocalSounds();

	void setVoicePath2(const std::string &voicePath2) { this->voicePath2 = voicePath2; }

	SoundCore::Buffer loadLocalSound(const std::string &sound);
	SoundCore::Buffer loadSoundIntoBank(const std::string &filename, const std::string &path, const std::string &format, SoundLoadType = SFXLOAD_CACHE);
	SoundCore::Buffer getBuffer(const std::string &name);

	void *playSfx(const PlaySfx &play);
	void *playSfx(const std::string &name, float vol=1);

	bool playMusic(const std::string &name, SoundLoopType=SLT_NORMAL, SoundFadeType sft=SFT_NONE, float trans=0, SoundConditionType sct=SCT_NORMAL);
	bool playVoice(const std::string &name, SoundVoiceType=SVT_QUEUE, float vmod=-1);

	float getMusicFader();
	float getVoxFader();

	void setListenerPos(float x, float y);
	void setSoundPos(void *channel, float x, float y);
	void setSoundRelative(void *channel, bool relative);

	std::string getVolumeString();

	void toggleEffectMusic(SoundEffectType effect, bool on);

	void clearFadingSfx();

	void setMusicSpeed(float speed);
	void setModSpeed(float speed);

	bool isPlayingMusic(const std::string &name);

	void setSfxChannelsVolume(float v);

	bool isPaused();

	void stopVoice();
	void stopAllVoice();
	void stopMusic();
	void stopSfx(void *channel);

	void fadeSfx(void *channel, SoundFadeType sft=SFT_OUT, float t=0.8);

	void fadeMusic(SoundFadeType sft=SFT_OUT, float t=1);

	bool isPlayingMusic();
	bool isPlayingVoice();

	void onVoiceEnded();

	void setVoiceFader(float v);

	void setMusicFader(float v, float t=0);

	void setMusicVolume(float v);
	void setSfxVolume(float v);
	void setVoiceVolume(float v);

	float getSfxVol();

	void updateChannelVolume(void *ch, float v=1);

	void pause();
	void resume();



	float getVoiceTime();

	void update(float dt);

	bool enabled;

	bool checkError();

	void error(const std::string &errMsg);

	EventPtr event_playVoice, event_stopVoice;

	std::string lastVoice, lastMusic;

	typedef std::list<std::string> LocalSounds;
	LocalSounds localSounds;
	void setOverrideVoiceFader(float v);

	std::string audioPath2;

	void getStats(int *curAlloc, int *maxAlloc);

	std::string reverbKeyword;
private:

	std::string voicePath2;

	float overrideVoiceFader;
	// sound voice music
	InterpolatedVector voxVol;
	InterpolatedVector musVol;
	float sfxVol;
	float voiceFader, sfxFader;

	std::queue<std::string> voxQueue;

	void (*loadProgressCallback)();
};

class SoundHolder
{
	friend class SoundManager;
public:
	void updateSoundPosition(float x, float y);
	void stopAllSounds();
	void unlinkSound(void *channel);
	void linkSound(void *channel);
	void unlinkAllSounds();

protected:
	virtual ~SoundHolder();

private:
	std::set<void*> activeSounds;
};


extern SoundManager *sound;



#endif

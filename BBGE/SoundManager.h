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

#include <string>
#include <list>
#include <queue>
#include "Vector.h"

// if using SDL_MIXER
//const int BBGE_AUDIO_NOCHANNEL		= -1;



#define BBGE_BUILD_FMODEX

//#define BBGE_BUILD_BASS20
//#define BBGE_BUILD_SDLMIXER
//#define BBGE_BUILD_IRRKLANG
//#define BBGE_BUILD_OPENALOGG



#ifdef BBGE_BUILD_OPENALOGG
	const int BBGE_AUDIO_NOCHANNEL		= -1;
#elif defined(BBGE_BUILD_FMODEX)
	const int BBGE_AUDIO_NOCHANNEL		= 0;
#endif

const int BBGE_AUDIO_LOOPINFINITE	= -1;
const int BBGE_AUDIO_LOOPNONE		= 0;

namespace SoundCore
{
#if defined(BBGE_BUILD_OPENALOGG)
	//struct Buffer;
#elif defined(BBGE_BUILD_FMODEX)
	typedef void *Buffer;
#endif
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
	PlaySfx() : priority(0.5), handle(0), pan(0), vol(1), fade(SFT_NONE), time(0), freq(1), loops(0), channel(BBGE_AUDIO_NOCHANNEL) {}

	std::string name;
	intptr_t handle;
	float pan;
	float vol;
	float time;
	float freq;
	int loops;
	int channel;
	float priority;
	SoundFadeType fade;
};

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
	void *playSfx(const std::string &name, float vol=1, float pan=0, float freq=1);
	void *playSfx(int handle, float vol=1, float pan=0, float freq=1);

	bool playMod(const std::string &name);
	bool playMusic(const std::string &name, SoundLoopType=SLT_NORMAL, SoundFadeType sft=SFT_NONE, float trans=0, SoundConditionType sct=SCT_NORMAL);
	bool playVoice(const std::string &name, SoundVoiceType=SVT_QUEUE, float vmod=-1);

	float getMusicFader();
	float getVoxFader();

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

	/*
	void setMusVol(float v, float t=0);
	void setSfxVol(float v, float t=0);
	void setVoxVol(float v, float t=0);

	void setMusMul(float v, float t=0);
	void setSfxMul(float v, float t=0);
	void setVoxMul(float v, float t=0);

	float getTotalSfxVol();
	float getTotalMusVol();
	float getTotalVoxVol();
	*/


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

extern SoundManager *sound;

#endif

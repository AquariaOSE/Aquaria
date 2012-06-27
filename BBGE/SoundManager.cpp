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

#include "Core.h"
#include "SoundManager.h"
#include "Base.h"
#include "PackRead.h"

#if defined(BBGE_BUILD_FMODEX)
    #ifdef BBGE_BUILD_FMOD_OPENAL_BRIDGE
	#include "FmodOpenALBridge.h"
	#else
	#include <fmod.h>
	#include <fmod.hpp>
	#ifdef BBGE_BUILD_WINDOWS
		#pragma comment(lib, "fmodex_vc.lib")
	#endif
	#endif
#endif

#ifdef BBGE_BUILD_FMODEX
#endif

SoundManager *sound = 0;

std::string soundPath = "sfx/cache/";
std::string localSoundPath = "sfx/local/";
std::string musicPath = "mus/";
std::string voicePath = "vox/";
std::string fileType = ".ogg";

namespace SoundCore
{
#ifdef BBGE_BUILD_FMODEX

	typedef std::map<std::string, FMOD::Sound*> SoundMap;
	SoundMap soundMap;

	FMOD_RESULT result;
	FMOD::System *system=0;

	FMOD::Sound *musicStream=0, *voiceStream=0, *musicStream2=0;

	FMOD::Channel *musicChannel=0, *musicChannel2=0;
	FMOD::Channel *voiceChannel=0;

	FMOD::ChannelGroup *group_vox = 0;
	FMOD::ChannelGroup *group_sfx = 0;
	FMOD::ChannelGroup *group_mus = 0;

	FMOD::Sound *modSound = 0;
	FMOD::Channel *modChannel = 0;

	FMOD::ChannelGroup *masterChannelGroup = 0;

	FMOD::DSP *dspReverb=0, *dspFlange=0;

	float musicFader2Time = 0, musicFader2Timer = 0, musicFader2Volume = 0;

	struct FadeCh
	{
	public:
		FadeCh() : v(1), s(1), c(0), d(-1), to(0) {}
		FMOD::Channel *c;
		float v,s,to;
		int d;
	};

	float faderV=1;
	FMOD::Channel *faderCh = 0;


	bool stopMusicOnFadeOut=false;
	bool wasPlayingVoice=false;

	typedef std::list<FadeCh> FadeChs;
	FadeChs fadeChs;

	void addFadeCh(const FadeCh &fadeCh)
	{
		FadeChs::iterator i = fadeChs.begin();
		for (; i != fadeChs.end();)
		{
			if ((*i).c == fadeCh.c)
			{
				i = fadeChs.erase(i);
			}
			else
			{
				i++;
			}
		}
		fadeChs.push_back(fadeCh);
	}
#endif
}

using namespace SoundCore;


/*
    TIPS:

    1. use F_CALLBACK.  Do NOT force cast your own function to fmod's callback type.
    2. return FMOD_ERR_FILE_NOTFOUND in open as required.
    3. return number of bytes read in read callback.  Do not get the size and count 
       around the wrong way in fread for example, this would return 1 instead of the number of bytes read.

    QUESTIONS:

    1. Why does fmod seek to the end and read?  Because it is looking for ID3V1 tags.  
       Use FMOD_IGNORETAGS in System::createSound / System::createStream if you don't like this behaviour.

*/

FMOD_RESULT F_CALLBACK myopen(const char *name, int unicode, unsigned int *filesize, void **handle, void **userdata)
{
    if (name)
    {
        VFILE *fp;

        fp = vfopen(name, "rb");
        if (!fp)
        {
            return FMOD_ERR_FILE_NOTFOUND;
        }

#ifdef BBGE_BUILD_VFS
        *filesize = fp->size();
#else
        vfseek(fp, 0, SEEK_END);
        *filesize = ftell(fp);
        vfseek(fp, 0, SEEK_SET);
#endif

        *userdata = (void *)0x12345678;
        *handle = fp;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK myclose(void *handle, void *userdata)
{
    if (!handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    vfclose((VFILE *)handle);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK myread(void *handle, void *buffer, unsigned int sizebytes, unsigned int *bytesread, void *userdata)
{
    if (!handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (bytesread)
    {
        *bytesread = (int)vfread(buffer, 1, sizebytes, (VFILE *)handle);
    
        if (*bytesread < sizebytes)
        {
            return FMOD_ERR_FILE_EOF;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK myseek(void *handle, unsigned int pos, void *userdata)
{
    if (!handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    vfseek((VFILE *)handle, pos, SEEK_SET);

    return FMOD_OK;
}


void SoundManager::pause()
{
#ifdef BBGE_BUILD_FMODEX
	debugLog("SoundManager::pause");

	debugLog("mus");
	result = group_mus->setPaused(true);
	checkError();

	debugLog("vox");
	result = group_vox->setPaused(true);
	checkError();

	debugLog("sfx");
	result = group_sfx->setPaused(true);
	checkError();

	debugLog("master channel");
	result = SoundCore::masterChannelGroup->setPaused(true);
	checkError();

	debugLog("update");
	result = SoundCore::system->update();
	checkError();
	
	debugLog("done");
#endif
}

void SoundManager::resume()
{
#ifdef BBGE_BUILD_FMODEX
	debugLog("SoundManager::resume");

	debugLog("mus");
	result = group_mus->setPaused(false);
	checkError();

	debugLog("vox");
	result = group_vox->setPaused(false);
	checkError();

	debugLog("sfx");
	result = group_sfx->setPaused(false);
	checkError();

	debugLog("master channel");
	result = SoundCore::masterChannelGroup->setPaused(false);
	checkError();

	debugLog("update");
	result = SoundCore::system->update();
	checkError();

	debugLog("done");
#endif
}

Buffer SoundManager::getBuffer(const std::string &name)
{
	std::string n = name;
	stringToLower(n);
	return soundMap[n];
}

void SoundManager::getStats(int *curAlloc, int *maxAlloc)
{
	FMOD::Memory_GetStats(curAlloc, maxAlloc);
}

SoundManager::SoundManager(const std::string &defaultDevice)
{
	overrideVoiceFader = -1;

	sound = this;

	enabled = false;

	sfxFader = 1;

	voxVol = Vector(1,1,1);
	musVol = Vector(1,1,1);
	sfxVol = 1;
	voiceFader = 1;

	loadProgressCallback = NULL;

#ifdef BBGE_BUILD_FMODEX

	int channels	= 128;

    unsigned int     version;
    FMOD_SPEAKERMODE speakermode;
    FMOD_CAPS        caps;

	debugLog("system::create");
	result = FMOD::System_Create(&SoundCore::system);
    if (checkError()) goto get_out;
   
	debugLog("getVersion");
    result = SoundCore::system->getVersion(&version);
    if (checkError()) goto get_out;

    if (version < FMOD_VERSION)
    {
		char str[256];
        sprintf(str, "Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
		debugLog(str);
		goto get_out;
    }

	debugLog("driver caps");
    result = SoundCore::system->getDriverCaps(0, &caps, 0, 0, &speakermode);
    if (checkError()) goto get_out;

	debugLog("set speaker mode");
    result = SoundCore::system->setSpeakerMode(speakermode);       /* Set the user selected speaker mode. */
	if (checkError()) goto get_out;

	debugLog("check caps");
    if (caps & FMOD_CAPS_HARDWARE_EMULATED)             /* The user has the 'Acceleration' slider set to off!  This is really bad for latency!. */
    {                                                   /* You might want to warn the user about this. */
		debugLog("acceleration slider is off");
        result = SoundCore::system->setDSPBufferSize(1024, 10);    /* At 48khz, the latency between issuing an fmod command and hearing it will now be about 213ms. */
        if (checkError()) goto get_out;
    }

	debugLog("init");
    result = SoundCore::system->init(channels, FMOD_INIT_NORMAL, 0);    /* Replace with whatever channel count and flags you use! */
    if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)         /* Ok, the speaker mode selected isn't supported by this soundcard.  Switch it back to stereo... */
    {
		debugLog("err_output_createbuffer, speaker mode");
		result = SoundCore::system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
        if (checkError()) goto get_out;
        
		debugLog("init 2");
        result = SoundCore::system->init(channels, FMOD_INIT_NORMAL, 0); /* Replace with whatever channel count and flags you use! */
		if (checkError()) goto get_out;
    }

#ifdef BBGE_BUILD_FMOD_OPENAL_BRIDGE
	SoundCore::system->getNumChannels(&channels);
#endif

	//FMOD::Debug_SetLevel(FMOD_DEBUG_LEVEL_ALL);

	/*
	result = FMOD::System_Create(&SoundCore::system);		// Create the main system object.
	if (checkError())
	{
		exit(-1);
	}

	result = SoundCore::system->init(64, FMOD_INIT_NORMAL, 0);	// Initialize FMOD.
	if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)
	{
		debugLog("FMOD_ERR_OUTPUT_CREATEBUFFER, setting stereo speaker mode");
		SoundCore::system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
		result = SoundCore::system->init(64, FMOD_INIT_NORMAL, 0);
		if (checkError())
			exit(-1);
	}
	else
	{
		if (checkError())
			exit(-1);
	}
	*/

	debugLog("set file system");
	result = SoundCore::system->setFileSystem(myopen, myclose, myread, myseek, 2048);
    if (checkError()) goto get_out;

	debugLog("create channel group vox");
	result = SoundCore::system->createChannelGroup("vox", &group_vox);
	if (checkError()) goto get_out;

	debugLog("create channel group sfx");
	result = SoundCore::system->createChannelGroup("sfx", &group_sfx);
	if (checkError()) goto get_out;

	debugLog("create channel group mus");
	result = SoundCore::system->createChannelGroup("mus", &group_mus);
	if (checkError()) goto get_out;

	debugLog("getMasterChannelGroup");
	result = SoundCore::system->getMasterChannelGroup(&masterChannelGroup);
	if (checkError()) goto get_out;

	debugLog("createDSPByType flange");
	result = SoundCore::system->createDSPByType(FMOD_DSP_TYPE_FLANGE, &dspFlange);
	if (checkError()) { dspFlange = 0; }

	debugLog("createDSPByType reverb");
	result = SoundCore::system->createDSPByType(FMOD_DSP_TYPE_REVERB, &dspReverb);
	if (checkError()) { dspReverb = 0; }


	//dspReverb->setParameter(FMOD_DSP_REVERB_ROOMSIZE, 0.5);
	//dspReverb->setParameter(FMOD_DSP_REVERB_DAMP, 0.5);
	//dspReverb->setParameter(FMOD_DSP_REVERB_WETMIX, 0.33);
	//dspReverb->setParameter(FMOD_DSP_REVERB_DRYMIX, 0.66);
	//dspReverb->setParameter(FMOD_DSP_REVERB_WIDTH, 1.0);
	//dspReverb->setParameter(FMOD_DSP_REVERB_MODE, 0); // 0 or 1

	if (dspReverb)
	{
		dspReverb->setParameter(FMOD_DSP_REVERB_ROOMSIZE, 0.8);
		dspReverb->setParameter(FMOD_DSP_REVERB_DAMP, 0.7);
		dspReverb->setParameter(FMOD_DSP_REVERB_WETMIX, 0.11);
		dspReverb->setParameter(FMOD_DSP_REVERB_DRYMIX, 0.88);
		dspReverb->setParameter(FMOD_DSP_REVERB_WIDTH, 1.0);
		dspReverb->setParameter(FMOD_DSP_REVERB_MODE, 0); // 0 or 1
	}


#endif

	enabled = true;

	return;

get_out:

	debugLog("get_out");

	enabled = false;

	if (SoundCore::system)
	{
		// clean up?
		SoundCore::system = 0;
		//SoundCore::system
	}
}

void SoundManager::toggleEffectMusic(SoundEffectType effect, bool on)
{
	if (!enabled) return;

#ifdef BBGE_BUILD_FMODEX

	bool active = false;

	switch(effect){
	case SFX_FLANGE:
		if (dspFlange){
			dspFlange->getActive(&active);
			if (on && !active)
				group_mus->addDSP(dspFlange, 0);
			else if (!on && active)
				dspFlange->remove();
		}
	break;
	}

#endif
}


std::string SoundManager::getVolumeString()
{
	std::ostringstream os;
	os << "sfxFader: " << this->sfxFader << " sfxVol: " << this->sfxVol << std::endl;
	os << "musVol: " << musVol.y << " voxVol: " << voxVol.y << std::endl;

	float musicChannelVol = -1;
	if (musicChannel)
		musicChannel->getVolume(&musicChannelVol);

	float musicGroupVol = -1;
	if (group_mus)
		group_mus->getVolume(&musicGroupVol);

	float musicChannel2Vol = -1;
	if (musicChannel2)
		musicChannel2->getVolume(&musicChannel2Vol);

	os << "curMusVol (c1/c2/g): " << musicChannelVol << " " << musicChannel2Vol << " " << musicGroupVol << std::endl;
	os << "runInBG: " << core->settings.runInBackground << std::endl;
	return os.str();
}

float SoundManager::getMusicFader()
{
	return musVol.y;
}

float SoundManager::getVoxFader()
{
	return voxVol.y;
}

void SoundManager::setChannelVolume(void *chan, float v)
{
	// is this now unused?
}

void SoundManager::setOverrideVoiceFader(float v)
{
	overrideVoiceFader = v;
}

void SoundManager::setMusicFader(float v, float t)
{
	// ignore fades if the music is already on its way to fading out to 0
	if (v != 0 && musVol.data && musVol.data->target.y == 0 && musVol.y > 0)
	{
		return;
	}

	/*
	std::ostringstream os;
	os << "musicFader " << v << " over " << t;
	debugLog(os.str());
	*/

	musVol.interpolateTo(Vector(musVol.x, v, musVol.z), t);

#ifdef BBGE_BUILD_FMODEX
	/*
	result = group_mus->setVolume(musVol.x*musVol.y*v);
	checkError();
	*/
#endif
}

void SoundManager::error(const std::string &errMsg)
{
	//std::cout << errMsg << std::endl;
	errorLog(errMsg);
}

SoundManager::~SoundManager()
{
	// release
	if (!enabled) return;

	for (SoundMap::iterator i = soundMap.begin(); i != soundMap.end(); i++)
	{
		std::string snd = (*i).first;
		debugLog("unloading sound [" + snd + "]");
#ifndef BBGE_DISABLE_SOUND_CACHE
		FMOD::Sound *samp = (FMOD::Sound*)((*i).second);
		samp->release();
#else
		SoundInfo *info = (SoundInfo*)((*i).second);
		delete info;
#endif
	}
	soundMap.clear();

#ifdef BBGE_BUILD_FMODEX
	SoundCore::system->release();
#endif
}

void SoundManager::stopAllSfx()
{
#ifdef BBGE_BUILD_FMODEX
	if (group_sfx)
		group_sfx->stop();
#endif
}

void SoundManager::stopAll()
{
}

void SoundManager::onVoiceEnded()
{
	//debugLog("Voice Ended!");
	event_stopVoice.call();
	//debugLog("checking vox queue");

	if (dspReverb)
		dspReverb->remove();

	if (!voxQueue.empty())
	{
		//debugLog("calling playVoice");
		
		std::string vox = voxQueue.front();
		
		//debugLog("popping voxQueue");
		if (!voxQueue.empty())
			voxQueue.pop();
			
		//debugLog("calling playVoice");
		playVoice(vox, SVT_INTERRUPT);
	}
	else
	{
		//debugLog("setting music fader");
		setMusicFader(1, 1);
		sfxFader = 1;
	}



	//debugLog("done onVoiceEnded");
}


bool SoundManager::isPaused()
{
	bool paused = false;

	if (!enabled) return paused;

#ifdef BBGE_BUILD_FMODEX

	result = masterChannelGroup->getPaused(&paused);
	checkError();

#endif

	return paused;
}

void SoundManager::clearFadingSfx()
{
#ifdef BBGE_BUILD_FMODEX

	SoundCore::FadeChs::iterator i = fadeChs.begin();
	for (; i != fadeChs.end(); i++)
	{
		//haha:
		FadeCh *f = &(*i);
		if (f->c)
		{
			f->c->stop();
		}
	}
	SoundCore::fadeChs.clear();

#endif

}

void SoundManager::update(float dt)
{
	if (isPaused()) return;

	dt = core->get_old_dt();

	voxVol.update(dt);
	musVol.update(dt);


#ifdef BBGE_BUILD_FMODEX

	if (musicChannel)
	{
		bool _isplaying = false;
		result = musicChannel->isPlaying(&_isplaying);
		checkError();
		if (!_isplaying)
		{
			result = musicChannel->stop();
			checkError();
			musicChannel = 0;

			if (musicStream)
			{
				result = musicStream->release();
				checkError();
				musicStream = 0;
			}	
		}
	}

	if (musicChannel)
	{
		// fader value
		
		result = musicChannel->setVolume(musVol.y*1.0f);
		checkError();
		

		if (musVol.y <= 0 && stopMusicOnFadeOut)
		{
			result = musicChannel->stop();
			checkError();
			musicChannel = 0;

			if (musicStream)
			{
				result = musicStream->release();
				checkError();
				musicStream = 0;
			}

			stopMusicOnFadeOut = false;
		}
	}

	if (group_sfx)
	{
		group_sfx->setVolume(sfxFader*sfxVol);
	}


	// for cross fading
	if (musicChannel2)
	{
		musicFader2Timer -= dt;
		if (musicFader2Timer < 0) musicFader2Timer = 0;

		musicChannel2->setVolume((musicFader2Timer/musicFader2Time)*musicFader2Volume);

		if (musicFader2Timer <= 0)
		{
			result = musicChannel2->stop();
			checkError();

			result = musicStream2->release();
			checkError();

			musicChannel2 = 0;
			musicStream2 = 0;
		}
	}

	if (!fadeChs.empty())
	{
		int itr=0;
		for (FadeChs::iterator i = fadeChs.begin(); i != fadeChs.end();)
		{
			itr++;
			FadeCh *f = &(*i);

			f->v += dt*f->s * f->d;


			if (f->d > 0)
			{
				if (f->v >= f->to)
				{
					f->v = f->to;
					i = fadeChs.erase(i);
					continue;
				}
			}
			else
			{
				if (f->v <= f->to)
				{
					f->v = f->to;

					result = f->c->stop();
					checkError();
					f->c = 0;
					i = fadeChs.erase(i);
					continue;
				}
			}
	
			if (f->c)
			{
				f->c->setVolume(f->v);
				checkError();
			}

			i++;
		}
	}

	SoundCore::system->update();

#endif

#if defined(BBGE_BUILD_BASS20) || defined(BBGE_BUILD_FMODEX) || defined(BBGE_BUILD_SDLMIXER)
	if (wasPlayingVoice && !isPlayingVoice())
	{
		wasPlayingVoice = false;
		onVoiceEnded();
	}
#endif
}

void SoundManager::fadeMusic(SoundFadeType sft, float t)
{
	switch(sft)
	{
	case SFT_CROSS:
	{
#ifdef BBGE_BUILD_FMODEX

		if (musicChannel2)
		{
			musicChannel2->stop();
			if (musicStream2)
			{
				musicStream2->release();
				musicStream2 = 0;
			}
			musicChannel2 = 0;
		}

		musicChannel2 = musicChannel;
		musicStream2 = musicStream;
		musicStream = 0;
		musicChannel = 0;
		musicFader2Volume = musVol.y;
		musicFader2Time = musicFader2Timer = t;

#endif

	}
	break;
	case SFT_OUT:
		setMusicFader(0, t);

#ifdef BBGE_BUILD_FMODEX
		stopMusicOnFadeOut = true;
#endif
	break;
	default:
		//setMusMul(0, t);
	break;
	}
}

bool SoundManager::isPlayingMusic()
{
#ifdef BBGE_BUILD_FMODEX

	if (musicChannel)
	{
		bool b=false;
		musicChannel->isPlaying(&b);
		return b;
	}

#endif

#ifdef BBGE_BUILD_BASS20

	return musicStream != 0;

#endif

	return false;
}

void SoundManager::setMusicVolume(float v)
{
	musVol.x = v;

#ifdef BBGE_BUILD_FMODEX

	result = group_mus->setVolume(v);
	checkError();

#endif
}

void SoundManager::setSfxVolume(float v)
{
	sfxVol = v;
}

float SoundManager::getSfxVol()
{
	return sfxVol;
}

void SoundManager::setVoiceVolume(float v)
{
	voxVol.x = v;

#ifdef BBGE_BUILD_FMODEX
	result = group_vox->setVolume(v);
	checkError();
#endif
}

bool SoundManager::isPlayingVoice()
{
#ifdef BBGE_BUILD_FMODEX

	if (voiceChannel)
	{
		bool b=false;
		result = voiceChannel->isPlaying(&b);
		if (result == FMOD_ERR_CHANNEL_STOLEN)
		{
			b = false;
			debugLog("voice channel 'stolen'");
		}
		else
		{
			checkError();
		}
		if (!b)
			voiceChannel = 0;
		return b;
	}

#endif

	return false;
}

void SoundManager::setSfxChannelsVolume(float v)
{
}

bool SoundManager::playVoice(const std::string &name, SoundVoiceType svt, float vmod)
{
	//debugLog("playVoice, masterSoundLock: " + name);

	if (!enabled) return false;

	bool checkOther = true;
	std::string fn, n;

	n = name;
	stringToLower(n);
	
	if (!voicePath2.empty())
	{
		fn = core->adjustFilenameCase(voicePath2 + name + fileType);
		if (exists(fn))	checkOther = false;
	}

	if (checkOther)
	{
		fn = core->adjustFilenameCase(voicePath + name + fileType);
		if (!exists(fn))
		{
			debugLog("Could not find voice file [" + fn + "]");
		}
	}

	bool playNow=false;

	debugLog("checking is playing now...");

	switch(svt)
	{
	case SVT_QUEUE:
	{
		if (isPlayingVoice())
		{
			if (voxQueue.empty() || voxQueue.front() != n)
				voxQueue.push(n);
		}
		else
		{
			playNow = true;
		}
	}
	break;
	case SVT_INTERRUPT:
	{
		stopAllVoice();
		playNow = true;
	}
	break;
	default:
	break;
	}

	if (playNow)
	{
#ifdef BBGE_BUILD_FMODEX
		if (voiceStream)
		{
			stopVoice();
		}
#endif

		debugLog("play now");

		if (overrideVoiceFader != -1)
		{
			if (overrideVoiceFader < 1)
			{
				setMusicFader(overrideVoiceFader, 0.5);
				sfxFader = overrideVoiceFader;
			}
		}
		else
		{
			if (voiceFader < 1 )
			{
				setMusicFader(voiceFader, 0.5);
				sfxFader = voiceFader;
			}
		}

#ifdef BBGE_BUILD_FMODEX


		// FMOD_DEFAULT uses the defaults.  These are the same as FMOD_LOOP_OFF | FMOD_2D | FMOD_HARDWARE.

		FMOD_MODE mode=0;
		mode = FMOD_2D | FMOD_SOFTWARE | FMOD_CREATESTREAM;

		result = SoundCore::system->createStream(fn.c_str(), mode, 0, &voiceStream);
		if (checkError())
		{
			voiceStream = 0;
		}

		if (voiceStream)
		{		

			if (!reverbKeyword.empty())
			{
				bool useReverb = (n.find(reverbKeyword) != std::string::npos);

				if (dspReverb)
				{
					bool active = false;
					
					result = dspReverb->getActive(&active);
					checkError();

					if (!active && useReverb)
					{
						result = group_vox->addDSP(dspReverb, 0);
						checkError();
					}
					else if (active && !useReverb)
					{
						result = dspReverb->remove();
						checkError();
					}

					// defaults:
					//dspReverb->setParameter(FMOD_DSP_REVERB_ROOMSIZE, 0.5);
					//dspReverb->setParameter(FMOD_DSP_REVERB_DAMP, 0.5);
					//dspReverb->setParameter(FMOD_DSP_REVERB_WETMIX, 0.33);
					//dspReverb->setParameter(FMOD_DSP_REVERB_DRYMIX, 0.66);
					//dspReverb->setParameter(FMOD_DSP_REVERB_WIDTH, 1.0);
					//dspReverb->setParameter(FMOD_DSP_REVERB_MODE, 0); // 0 or 1
				}
			}

			result = SoundCore::system->playSound(FMOD_CHANNEL_FREE, voiceStream, true, &voiceChannel);
			checkError();

			result = voiceChannel->setChannelGroup(group_vox);
			checkError();
			
			if (vmod != -1)
			{
				result = voiceChannel->setVolume(vmod);
				checkError();
			}

			result = voiceChannel->setPriority(1);
			checkError();

			/*
			result = dspReverb->remove();
			checkError();
			*/

			voiceChannel->setPan(0);
			voiceChannel->setFrequency(1);

			result = voiceChannel->setPaused(false);
			checkError();

			wasPlayingVoice = true;
		}

#endif

		lastVoice = n;
		event_playVoice.call();
		core->onPlayedVoice(n);
	}

	return true;
}

void SoundManager::updateChannelVolume(void *ch, float v)
{
}

float SoundManager::getVoiceTime()
{
#ifdef BBGE_BUILD_FMODEX

	if (isPlayingVoice())
	{
		unsigned int position;
		voiceChannel->getPosition(&position, FMOD_TIMEUNIT_MS);
		return float(position) * 0.001f;
	}

#endif

	return 0;
}

void *SoundManager::playSfx(const PlaySfx &play)
{
	if (!enabled) return 0;

#ifdef BBGE_BUILD_FMODEX

	FMOD::Channel *channel = 0;
	FMOD::Sound *sound = 0;
	

	if (play.handle)
		sound = (FMOD::Sound*)play.handle;
	else if (!play.name.empty())
		sound = (FMOD::Sound*)getBuffer(play.name);

	if (!sound) return 0;

	result = SoundCore::system->playSound(FMOD_CHANNEL_FREE, sound, true, &channel);
	checkError();

	if (channel == NULL)  // the OpenAL bridge code might return NULL here.
		return 0;

	result = channel->setChannelGroup(group_sfx);
	checkError();

	result = channel->setPriority((int) ((1-play.priority)*255));
	checkError();

	if (play.fade == SFT_IN)
	{
		result = channel->setVolume(0);
		checkError();

		FadeCh fade;
		fade.c = channel;
		fade.v = 0;
		fade.s = 1.0f/play.time;
		fade.d = 1;
		fade.to = play.vol;

		SoundCore::addFadeCh(fade);
	}
	else
	{
		result = channel->setVolume(play.vol);
		checkError();
	}

	channel->setPan(play.pan);

	float freq = play.freq;
	if (freq <= 0)
		freq = 1;
	channel->setFrequency(freq);

	result = channel->setPaused(false);
	checkError();

	return channel;
#endif


	return 0;
}

void *SoundManager::playSfx(const std::string &name, float vol, float pan, float freq)
{
	PlaySfx play;
	play.name = name;
	play.vol = vol;
	play.pan = pan;
	play.freq = freq;
	return playSfx(play);
}

void *SoundManager::playSfx(int handle, float vol, float pan, float freq)
{
	PlaySfx play;
	play.handle = handle;
	play.vol = vol;
	play.pan = pan;
	play.freq = freq;
	return playSfx(play);
}

void SoundManager::setVoiceFader(float v)
{
	voiceFader = v;
}

bool SoundManager::isPlayingMusic(const std::string &name)
{
	if (isPlayingMusic())
	{
		std::string test = name;
		stringToLower(test);

		/*
		std::ostringstream os;
		os << "checking lastMusic: " << lastMusic << " test: " << test;
		debugLog(os.str());
		*/

		if (test == lastMusic)
			return true;
	}

	return false;
}

bool SoundManager::playMod(const std::string &name)
{
	std::string fn;

	fn = musicPath + name;
	stringToLower(fn);

	FMOD_MODE mode=0;

	//FMOD_CREATESOUNDEXINFO exinfo;

	//mode = FMOD_2D | FMOD_SOFTWARE | FMOD_CREATESAMPLE;//FMOD_CREATESTREAM;
	mode = FMOD_HARDWARE | FMOD_2D | FMOD_CREATESTREAM;

	debugLog("createSound: " + fn);
	result = SoundCore::system->createSound(fn.c_str(), mode, 0, &modSound);
	if (checkError())
	{
		debugLog("createSound failed");
		return false;
	}

	debugLog("playSound");
	result = SoundCore::system->playSound(FMOD_CHANNEL_FREE, modSound, false, &modChannel);
	checkError();

	debugLog("setChannelGroup");
	result = modChannel->setChannelGroup(group_mus);
	checkError();

	debugLog("setPriority");
	result = modChannel->setPriority(0); // should be highest priority (according to the docs)
	checkError();

	debugLog("setPaused");
	result = modChannel->setPaused(false);
	checkError();

	debugLog("returning");

	return true;
}

bool SoundManager::playMusic(const std::string &name, SoundLoopType slt, SoundFadeType sft, float trans, SoundConditionType sct)
{
	debugLog("playMusic: " + name);

	if (!enabled) return false;


	if (sct == SCT_ISNOTPLAYING && isPlayingMusic())
	{
		if (isPlayingMusic(name))
		{
			#ifdef BBGE_BUILD_OPENALOGG
				if (masterSoundLock) SDL_mutexV(masterSoundLock);
			#endif
			return false;
		}
	}

	std::string fn = "";
	if (!name.empty() && name[0] == '.')
	{
		fn = name;
	}
	else
	{
		if (!audioPath2.empty())
		{
			fn = audioPath2 + name + fileType;
			if (!exists(fn))
			{
				fn = musicPath + name + fileType;
			}
		}
		else
		{
			fn = musicPath + name + fileType;
		}
	}

	fn = core->adjustFilenameCase(fn);

	lastMusic = name;
	stringToLower(lastMusic);

#ifdef BBGE_BUILD_FMODEX

	if (sft == SFT_CROSS)
	{
		fadeMusic(SFT_CROSS, trans);
	}

	if (musicStream)
	{
		musicStream->release();
		musicStream = 0;
	}

	if (musicChannel)
	{
		if (sft == SFT_IN)
		{
			musicChannel->stop();  // we're fading in music but didn't stop the old one?
		}
		musicChannel = 0;
	}

	// FMOD_DEFAULT uses the defaults.  These are the same as FMOD_LOOP_OFF | FMOD_2D | FMOD_HARDWARE.

	FMOD_MODE mode=0;

	///FMOD_DEFAULT;////mode = FMOD_2D | FMOD_SOFTWARE;

	mode = FMOD_2D | FMOD_SOFTWARE | FMOD_CREATESTREAM;


	switch(slt)
	{
	case SLT_OFF:
	case SLT_NONE:
		mode |= FMOD_LOOP_OFF;
	break;
	default:
		mode |= FMOD_LOOP_NORMAL;
	break;
	}

	stopMusicOnFadeOut = false;
	musVol.stop();

	result = SoundCore::system->createStream(fn.c_str(), mode, 0, &musicStream);
	if (checkError()) musicStream = 0;

	if (musicStream)
	{
		

		result = SoundCore::system->playSound(FMOD_CHANNEL_FREE, musicStream, true, &musicChannel);
		checkError();

		result = musicChannel->setChannelGroup(group_mus);
		checkError();

		result = musicChannel->setPriority(0); // should be highest priority (according to the docs)
		checkError();

		if (sft == SFT_IN || sft == SFT_CROSS)
		{
			setMusicFader(0);
			setMusicFader(1,trans);

			result = musicChannel->setVolume(0);
			checkError();
		}
		else
		{
			setMusicFader(1,0);

			result = musicChannel->setVolume(musVol.y);
			checkError();
		}

		musicChannel->setFrequency(1); // in case the channel was used by a pitch-shifted sound before
		musicChannel->setPan(0);

		result = musicChannel->setPaused(false);		// This is where the sound really starts.
		checkError();
	}
#endif

	debugLog("playmusic end");

	return true;
}


void SoundManager::stopMusic()
{

#ifdef BBGE_BUILD_FMODEX
	if (musicChannel)
	{
		musicChannel->stop();
		checkError();
		if (musicStream)
		{
			musicStream->release();
			checkError();
		}
		musicStream = 0;
		musicChannel = 0;
	}
#endif

	lastMusic = "";
}

void SoundManager::stopSfx(void *channel)
{
#ifdef BBGE_BUILD_FMODEX
	if (!channel) return;
	FMOD::Channel *ch = (FMOD::Channel*)channel;
	if (ch)
	{
		ch->stop();
		checkError();
		ch = 0;
	}
#endif
}

void SoundManager::fadeSfx(void *channel, SoundFadeType sft, float t)
{
#ifdef BBGE_BUILD_FMODEX
	if (!channel) return;
	if (sft == SFT_OUT)
	{
		FMOD::Channel *ch = (FMOD::Channel*)channel;
		if (ch)
		{
			FadeCh f;
			f.s = 1.0f/t;
			result = ch->getVolume(&f.v);
			checkError();

			f.c = ch;
			SoundCore::addFadeCh(f);
		}
	}
#endif
}

void SoundManager::stopVoice()
{

#ifdef BBGE_BUILD_FMODEX
	if (voiceChannel)
	{
		bool playing = false;
		result = voiceChannel->isPlaying(&playing);
		checkError();

		if (playing)
		{
			result = voiceChannel->stop();
			checkError();
			voiceChannel = 0;
		}
	}
	if (voiceStream)
	{
		result = voiceStream->release();
		checkError();
		voiceStream = 0;
	}
	onVoiceEnded();
#endif

}

void SoundManager::stopAllVoice()
{
	while (!voxQueue.empty()) voxQueue.pop();
	stopVoice();
}

void loadCacheSoundsCallback (const std::string &filename, intptr_t param)
{
	SoundManager *sm;
	sm = (SoundManager*)param;
	if (!sm->enabled)
	{
		//sm->erorr();
		debugLog("Disabled: Won't Load Sample ["+filename+"]");
		return;
	}
	if (fileType==".ogg")
	{
		debugLog("trying to load sound " + filename);
		sm->loadSoundIntoBank(filename, "", "");
	}
}

void SoundManager::loadSoundCache(const std::string &path, const std::string &ftype, void progressCallback())
{
	loadProgressCallback = progressCallback;
	forEachFile(path, ftype, loadCacheSoundsCallback, (intptr_t)this);
	loadProgressCallback = NULL;
}

Buffer SoundManager::loadSoundIntoBank(const std::string &filename, const std::string &path, const std::string &format, SoundLoadType slt)
{
	if (loadProgressCallback)
		loadProgressCallback();

	std::string f = filename, name;

	// WARNING: local sounds should go here!

	debugLog(filename);
	if (slt == SFXLOAD_LOCAL && !audioPath2.empty())
	{
		f = core->adjustFilenameCase(audioPath2 + filename + format);
		if (!exists(f))
		{
			f = core->adjustFilenameCase(path + filename + format);
		}
	}
	else
	{
		f = core->adjustFilenameCase(path + filename + format);
	}

	bool loop = false;

	if (f.find("loop")!=std::string::npos)
	{
		loop = true;
	}

	int loc = f.find_last_of('/');
	int loc2 = f.rfind('.');
	if (loc != std::string::npos && loc2 != std::string::npos)
	{
		name = f.substr(loc+1, loc2-(loc+1));
	}
	else
	{
	    debugLog("returning 0");
		return Buffer();
	}

	stringToLower(name);

#ifdef BBGE_BUILD_FMODEX

	FMOD::Sound * sound = SoundCore::soundMap[name];

	if (sound)
		return sound;

	FMOD_MODE mode = FMOD_DEFAULT | FMOD_LOWMEM;
	if (loop)
		mode |= FMOD_LOOP_NORMAL;

	result = SoundCore::system->createSound(f.c_str(), mode, 0, &sound);
	if (checkError())
	{
		debugLog("createSound failed");
		return Buffer();
	}

	SoundCore::soundMap[name] = sound;


	if (slt == SFXLOAD_LOCAL)
	{
		localSounds.push_back(name);
	}

	return sound;
#endif

#ifdef BBGE_BUILD_FMODEX

#endif

	return Buffer();
}

Buffer SoundManager::loadLocalSound(const std::string &filename)
{
	Buffer b = loadSoundIntoBank(filename, localSoundPath, fileType, SFXLOAD_LOCAL);

#ifdef BBGE_BUILD_FMODEX
	return b;
#endif

	return BBGE_AUDIO_NOCHANNEL;
}

void SoundManager::setMusicSpeed(float speed)
{
	/*
	FMOD_CAPS caps;
	FMOD_SPEAKERMODE speakerMode;
	int minf, maxf;
	SoundCore::system->getDriverCaps(0, &caps, &minf, &maxf, &speakerMode);
	std::ostringstream os;
	os << "minf: " << minf << " maxf: " << maxf;
	debugLog(os.str());
	*/

	musicChannel->setFrequency(speed);
}

void SoundManager::setModSpeed(float speed)
{
	if (modChannel)
		modChannel->setFrequency(speed);
}

void SoundManager::clearLocalSounds()
{
#ifdef BBGE_BUILD_FMODEX
	for (LocalSounds::iterator i = localSounds.begin(); i != localSounds.end(); i++)
	{
		std::string snd = (*i);
		debugLog("unloading sound [" + snd + "]");
		FMOD::Sound *samp = (FMOD::Sound*)soundMap[snd];
		samp->release();
		soundMap[snd] = 0;
	}
	localSounds.clear();
#endif
}

bool SoundManager::checkError()
{
#ifdef BBGE_BUILD_FMODEX
	if (result != FMOD_OK)
	{
		std::ostringstream os;
		os << "FMODEX error: " << result << ": ";

		switch(result)
		{
		case FMOD_ERR_ALREADYLOCKED:
			os << "FMOD_ERR_ALREADYLOCKED Tried to call lock a second time before unlock was called.";
		break;
		case FMOD_ERR_BADCOMMAND:
			os << "Tried to call a function on a data type that does not allow this type of functionality (ie calling Sound::lock on a streaming sound).";
		break;
		case FMOD_ERR_CHANNEL_ALLOC:
			os << "FMOD_ERR_CHANNEL_ALLOC: Error trying to allocate a channel.";
		break;
		case FMOD_ERR_CHANNEL_STOLEN:
			os << "FMOD_ERR_CHANNEL_STOLEN: The specified channel has been reused to play another sound.";
		break;
		case FMOD_ERR_COM:
			os << "FMOD_ERR_COM: A Win32 COM related error occured. COM failed to initialize or a QueryInterface failed meaning a Windows codec or driver was not installed properly.";
		break;
		case FMOD_ERR_DMA:
			os << "FMOD_ERR_DMA: DMA Failure. See debug output for more information.";
		break;
		case FMOD_ERR_DSP_CONNECTION:
			os << "FMOD_ERR_DSP_CONNECTION: DSP connection error. Connection possibly caused a cyclic dependancy.";
		break;
		case FMOD_ERR_DSP_FORMAT:
			os << "FMOD_ERR_DSP_FORMAT: DSP Format error. A DSP unit may have attempted to connect to this network with the wrong format.";
		break;
		case FMOD_ERR_DSP_NOTFOUND:
			os << "FMOD_ERR_DSP_NOTFOUND: DSP connection error. Couldn't find the DSP unit specified.";
		break;
		default:
			os << "Unknown error code";
		break;
		}
		debugLog(os.str());
		return true;
	}
#endif
	return false;
}

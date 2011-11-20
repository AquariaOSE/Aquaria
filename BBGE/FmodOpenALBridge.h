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
#ifndef _INCL_FMOD_OPENAL_BRIDGE_H_
#define _INCL_FMOD_OPENAL_BRIDGE_H_

#ifdef _WIN32
#include <malloc.h>
#endif

typedef enum
{
    FMOD_OK = 0,
    FMOD_ERR_INTERNAL,
    FMOD_ERR_FILE_NOTFOUND,
    FMOD_ERR_INVALID_PARAM,
    FMOD_ERR_FILE_EOF,
    FMOD_ERR_OUTPUT_CREATEBUFFER,
    FMOD_ERR_CHANNEL_STOLEN,
	FMOD_ERR_ALREADYLOCKED,
	FMOD_ERR_BADCOMMAND,
	FMOD_ERR_CHANNEL_ALLOC,
	FMOD_ERR_COM,
	FMOD_ERR_DMA,
	FMOD_ERR_DSP_CONNECTION,
	FMOD_ERR_DSP_FORMAT,
	FMOD_ERR_DSP_NOTFOUND,
} FMOD_RESULT;

typedef enum
{
    FMOD_SPEAKERMODE_STEREO,
} FMOD_SPEAKERMODE;

typedef enum
{
    FMOD_TIMEUNIT_MS,
} FMOD_TIMEUNIT;

typedef enum
{
    FMOD_DSP_TYPE_FLANGE,
    FMOD_DSP_TYPE_REVERB,
} FMOD_DSP_TYPE;

#define F_CALLBACK

typedef int FMOD_CAPS;
#define FMOD_CAPS_HARDWARE_EMULATED 1

#define FMOD_VERSION 1337

#define FMOD_CHANNEL_FREE -1

typedef int FMOD_MODE;
#define FMOD_HARDWARE     (1<<0)
#define FMOD_SOFTWARE     (1<<1)
#define FMOD_2D           (1<<2)
#define FMOD_CREATESTREAM (1<<3)
#define FMOD_LOOP_OFF     (1<<4)
#define FMOD_LOOP_NORMAL  (1<<5)
#define FMOD_LOWMEM       (1<<6)
#define FMOD_DEFAULT      (FMOD_2D | FMOD_HARDWARE)

typedef int FMOD_CREATESOUNDEXINFO;

#define FMOD_INIT_NORMAL 0
typedef int FMOD_INITFLAGS;

typedef int FMOD_CHANNELINDEX;
typedef bool FMOD_BOOL;

typedef FMOD_RESULT (*FMOD_FILE_OPENCALLBACK)(const char *,int,unsigned int *,void **,void **);
typedef FMOD_RESULT (*FMOD_FILE_CLOSECALLBACK)(void *,void *);
typedef FMOD_RESULT (*FMOD_FILE_READCALLBACK)(void *,void *,unsigned int,unsigned int *,void *);
typedef FMOD_RESULT (*FMOD_FILE_SEEKCALLBACK)(void *,unsigned int,void *);


typedef enum
{
    FMOD_DSP_REVERB_ROOMSIZE,
	FMOD_DSP_REVERB_DAMP,
    FMOD_DSP_REVERB_WETMIX,
    FMOD_DSP_REVERB_DRYMIX,
    FMOD_DSP_REVERB_WIDTH,
    FMOD_DSP_REVERB_MODE,
} FMOD_DSP_REVERB_PARAMS;  // we don't use this, but we should!


namespace FMOD
{
    typedef int DSPConnection;

    class Sound
    {
    public:
        FMOD_RESULT release();
    };

    class DSP
    {
    public:
        FMOD_RESULT getActive(bool *active);
        FMOD_RESULT remove();
        FMOD_RESULT setParameter(int index, float value);
    };

    class ChannelGroup
    {
    public:
        FMOD_RESULT setPaused(bool paused);
        FMOD_RESULT addDSP(DSP *dsp, DSPConnection **connection);
        FMOD_RESULT getPaused(bool *paused);
        FMOD_RESULT getVolume(float *volume);
        FMOD_RESULT setVolume(float volume);
        FMOD_RESULT stop();
    };

    class Channel
    {
    public:
        FMOD_RESULT setVolume(float volume);
        FMOD_RESULT getPosition(unsigned int *position, FMOD_TIMEUNIT postype);
        FMOD_RESULT getVolume(float *volume);
        FMOD_RESULT isPlaying(bool *isplaying);
        FMOD_RESULT setChannelGroup(ChannelGroup *channelgroup);
        FMOD_RESULT setFrequency(float frequency);
        FMOD_RESULT setPriority(int priority);
        FMOD_RESULT stop();
        FMOD_RESULT setPaused(bool paused);
    };

    class System
    {
    public:
        FMOD_RESULT release();
        FMOD_RESULT createChannelGroup(const char *name, ChannelGroup **channelgroup);
        FMOD_RESULT createDSPByType(FMOD_DSP_TYPE type, DSP **dsp);
        FMOD_RESULT createSound(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound);
        FMOD_RESULT createStream(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound);
        FMOD_RESULT getDriverCaps(int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode);
        FMOD_RESULT getMasterChannelGroup(ChannelGroup **channelgroup);
        FMOD_RESULT getVersion(unsigned int *version);
        FMOD_RESULT init(int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata);
        FMOD_RESULT playSound(FMOD_CHANNELINDEX channelid, Sound *sound, bool paused, Channel **channel);
        FMOD_RESULT setDSPBufferSize(unsigned int bufferlength, int numbuffers);
        FMOD_RESULT setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, int blockalign);
        FMOD_RESULT setSpeakerMode(FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT update();

	// BBGE-specific...
	FMOD_RESULT getNumChannels(int *maxchannels_ret);
    };

    typedef System FMOD_SYSTEM;
    FMOD_RESULT Memory_GetStats(int *currentalloced, int *maxalloced, FMOD_BOOL blocking=false);
    FMOD_RESULT System_Create(FMOD_SYSTEM **system);
}  // namespace FMOD

#endif

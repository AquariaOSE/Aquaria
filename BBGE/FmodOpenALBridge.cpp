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

// This file implements just enough of the FMOD library with OpenAL to suit
//  the needs of the existing game code without having to actually ship FMOD.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef BBGE_BUILD_UNIX
#include <signal.h>
#endif

#include "Base.h"
#include "Core.h"

#include "VFSFile.h"

#include "FmodOpenALBridge.h"

#include "al.h"
#include "alc.h"

#include "ogg/ogg.h"
#include "vorbis/vorbisfile.h"

#ifndef _DEBUG
//#define _DEBUG 1
#endif

///////////////////////////////////////////////////////////////////////////

// Decoder implementation for streamed Ogg Vorbis audio.

class OggDecoder {
public:
    // Create a decoder that streams from a file.
    OggDecoder(ttvfs::VFSFile *fp);

    // Create a decoder that streams from a memory buffer.
    OggDecoder(const void *data, long data_size);

    ~OggDecoder();

    // Start playing on the given channel, with optional looping.
    bool start(ALuint source, bool loop);

    // Decode audio into any free buffers.  Must be called periodically
    // on systems without threads; may be called without harm on systems
    // with threads (the function does nothing in that case).
    void update();

    // Terminate playback.
    void stop();

    // Return the current playback position in seconds.
    double position();

    // Memory buffer I/O callback functions for libvorbisfile.
    static size_t mem_read(void *ptr, size_t size, size_t nmemb, void *datasource);
    static int mem_seek(void *datasource, ogg_int64_t offset, int whence);
    static long mem_tell(void *datasource);

private:
    // Decoding loop, run in a separate thread (if threads are available).
    static void decode_loop(OggDecoder *this_);

    // Decode and queue PCM data for one buffer; does nothing if the end
    // of the stream has already been reached or an unrecoverable error
    // has occurred during decoding.  If looping, the audio will instead
    // restart at the beginning of the stream after reaching the end,
    // but will still stop on an unrecoverable error.
    void queue(ALuint buffer);

    static const int NUM_BUFFERS = 8;
    static const int BUFFER_LENGTH = 4096;  // In samples (arbitrary)
    char pcm_buffer[BUFFER_LENGTH * 4];     // Temporary buffer for decoding
    ALuint buffers[NUM_BUFFERS];
    ALuint source;

    // Data source.  If fp != NULL, the source is that file; otherwise, the
    // source is the buffer pointed to by "data" with size "data_size" bytes.
    ttvfs::VFSFile *fp;
    const char *data;
    long data_size;
    long data_pos;  // Current read position for memory buffers

    OggVorbis_File vf;
    ALenum format;
    int freq;

#ifdef BBGE_BUILD_SDL
    SDL_Thread *thread;
#else
    #warning Threads not supported, music may cut out on area changes!
    // ... because the stream runs out of decoded data while the area is
    // still loading, so OpenAL aborts playback.
#endif
    volatile bool stop_thread;

    bool playing;
    bool loop;
    bool eof;  // End of file _or_ unrecoverable error encountered
    unsigned int samples_done;  // Number of samples played and dequeued
};

// File I/O callback set (OV_CALLBACKS_NOCLOSE from libvorbis 1.2.0).
// It might be better to just update libogg/libvorbis to the current
// versions so we don't have to worry about identifier collisions --
// we can then drop all this and use OV_CALLBACKS_NOCLOSE in the
// ov_open_callbacks() call.  Note that we rename the fseek() wrapper
// to avoid an identifier collision when building with more recent
// versions of libvorbis.
static int BBGE_ov_header_fseek_wrap(void *f,ogg_int64_t off,int whence){
  if(f==NULL)return(-1);
  ttvfs::VFSFile *vf = (ttvfs::VFSFile*)f;
  switch(whence)
  {
    case SEEK_SET: return vf->seek(off);
    case SEEK_CUR: return vf->seekRel(off);
    case SEEK_END: return vf->seek(vf->size() - off);
  }
  return -1;
}
static size_t BBGE_ov_fread_wrap(void *ptr, size_t s, size_t count, void *f)
{
    if(f==NULL)return(-1);
    ttvfs::VFSFile *vf = (ttvfs::VFSFile*)f;
    size_t done = vf->read(ptr, s * count);
    return done / s;
}

static long BBGE_ov_ftell_wrap(void *f)
{
    if(f==NULL)return(-1);
    ttvfs::VFSFile *vf = (ttvfs::VFSFile*)f;
    return vf->getpos();
}

static int noclose(void *f) {return 0;}
static const ov_callbacks local_OV_CALLBACKS_NOCLOSE = {
  (size_t (*)(void *, size_t, size_t, void *))  BBGE_ov_fread_wrap,
  (int (*)(void *, ogg_int64_t, int))           BBGE_ov_header_fseek_wrap,
  (int (*)(void *))                             noclose,  // NULL doesn't work in libvorbis-1.1.2
  (long (*)(void *))                            BBGE_ov_ftell_wrap
};

// Memory I/O callback set.
static const ov_callbacks ogg_memory_callbacks = {
    OggDecoder::mem_read,
    OggDecoder::mem_seek,
    (int (*)(void *))noclose,
    OggDecoder::mem_tell
};


OggDecoder::OggDecoder(ttvfs::VFSFile *fp)
{
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        buffers[i] = 0;
    }
    this->source = 0;
    this->fp = fp;
    this->data = NULL;
    this->data_size = 0;
    this->data_pos = 0;
#ifdef BBGE_BUILD_SDL
    this->thread = NULL;
#endif
    this->stop_thread = true;
    this->playing = false;
    this->loop = false;
    this->eof = false;
    this->samples_done = 0;
}

OggDecoder::OggDecoder(const void *data, long data_size)
{
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        buffers[i] = 0;
    }
    this->source = 0;
    this->fp = NULL;
    this->data = (const char *)data;
    this->data_size = data_size;
    this->data_pos = 0;
#ifdef BBGE_BUILD_SDL
    this->thread = NULL;
#endif
    this->stop_thread = true;
    this->playing = false;
    this->loop = false;
    this->eof = false;
    this->samples_done = 0;
}

OggDecoder::~OggDecoder()
{
    if (playing)
        stop();

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        if (buffers[i])
            alDeleteBuffers(1, &buffers[i]);
    }
}

bool OggDecoder::start(ALuint source, bool loop)
{
    this->source = source;
    this->loop = loop;

    if (fp) {
        if (ov_open_callbacks(fp, &vf, NULL, 0, local_OV_CALLBACKS_NOCLOSE) != 0)
        {
            debugLog("ov_open() failed for file");
            return false;
        }
    }
    else
    {
        data_pos = 0;
        if (ov_open_callbacks(this, &vf, NULL, 0, ogg_memory_callbacks) != 0)
        {
            debugLog("ov_open() failed for memory buffer");
            return false;
        }
    }

    vorbis_info *info = ov_info(&vf, -1);
    if (!info)
    {
        debugLog("ov_info() failed");
        ov_clear(&vf);
        return false;
    }
    if (info->channels == 1)
        format = AL_FORMAT_MONO16;
    else if (info->channels == 2)
        format = AL_FORMAT_STEREO16;
    else
    {
        std::ostringstream os;
        os << "Bad channel count " << info->channels;
        debugLog(os.str());
        ov_clear(&vf);
        return false;
    }
    freq = info->rate;

    /* NOTE: The failure to use alGetError() here and elsewhere is
     * intentional -- since alGetError() writes to a global buffer and
     * is thus not thread-safe, we can't use it either in the decoding
     * threads _or_ here in the main thread.  In this case, we rely on 
     * the specification that failing OpenAL calls do not modify return
     * parameters to detect failure; for functions that do not return
     * values, we have no choice but to hope for the best.  (From a
     * multithreading point of view, the insistence on using a global
     * error buffer instead of returning success/failure or error codes
     * from functions is a remarkably poor design decision.  Not that a
     * mere library user has much choice except to live with it...)
     * --achurch */
    buffers[0] = 0;
    alGenBuffers(NUM_BUFFERS, buffers);
    if (!buffers[0])
    {
        debugLog("Failed to generate OpenAL buffers");
        ov_clear(&vf);
        return false;
    }

    playing = true;
    eof = false;
    samples_done = 0;
    for (int i = 0; i < NUM_BUFFERS; i++)
        queue(buffers[i]);

#ifdef BBGE_BUILD_SDL
    stop_thread = false;
    thread = SDL_CreateThread((int (*)(void *))decode_loop, this);
    if (!thread)
    {
        debugLog("Failed to create Ogg Vorbis decode thread: "
                 + std::string(SDL_GetError()));
    }
#endif

    return true;
}

void OggDecoder::update()
{
    if (!playing)
        return;
#ifdef BBGE_BUILD_SDL
    if (thread)
        return;
#endif

    int processed = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    for (int i = 0; i < processed; i++)
    {
        samples_done += BUFFER_LENGTH;
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
        queue(buffer);
    }
}

void OggDecoder::stop()
{
    if (!playing)
        return;

#ifdef BBGE_BUILD_SDL
    if (thread)
    {
        stop_thread = true;
        SDL_WaitThread(thread, NULL);
        thread = NULL;
    }
#endif

    ov_clear(&vf);

    alSourceStop(source);
    int queued = 0;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    for (int i = 0; i < queued; i++)
    {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
    }
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        alDeleteBuffers(1, &buffers[i]);
        buffers[i] = 0;
    }
}

double OggDecoder::position()
{
    ALint samples_played = 0;
    alGetSourcei(source, AL_SAMPLE_OFFSET, &samples_played);
    samples_played += samples_done;
    return (double)samples_played / (double)freq;
}

void OggDecoder::decode_loop(OggDecoder *this_)
{
    while (!this_->stop_thread)
    {
#ifdef BBGE_BUILD_SDL
        SDL_Delay(1);
#endif

        int processed = 0;
        alGetSourcei(this_->source, AL_BUFFERS_PROCESSED, &processed);
        for (int i = 0; i < processed; i++)
        {
            this_->samples_done += BUFFER_LENGTH;
            ALuint buffer = 0;
            alSourceUnqueueBuffers(this_->source, 1, &buffer);
            if (buffer)
                this_->queue(buffer);
        }
    }
}

void OggDecoder::queue(ALuint buffer)
{
    if (!playing || eof)
        return;

    const int channels = (format == AL_FORMAT_STEREO16 ? 2 : 1);
    const int buffer_size = BUFFER_LENGTH * channels * 2;
    int pcm_size = 0;
    bool just_looped = false;  // Avoid infinite loops on empty files.

    while (pcm_size < buffer_size && !eof)
    {
        int bitstream_unused;
        const int nread = ov_read(
            &vf, pcm_buffer + pcm_size, buffer_size - pcm_size,
            /*bigendianp*/ 0, /*word*/ 2, /*sgned*/ 1, &bitstream_unused
        );
        if (nread == 0 || nread == OV_EOF)
        {
            if (loop && !just_looped)
            {
                just_looped = true;
                samples_done = 0;
                ov_pcm_seek(&vf, 0);
            }
            else
            {
                eof = true;
            }
        }
        else if (nread == OV_HOLE)
        {
            debugLog("Warning: decompression error, data dropped");
        }
        else if (nread < 0)
        {
            std::ostringstream os;
            os << "Decompression error: " << nread;
            debugLog(os.str());
            eof = true;
        }
        else
        {
            pcm_size += nread;
            just_looped = false;
        }
    }

    if (pcm_size > 0)
    {
        alBufferData(buffer, format, pcm_buffer, pcm_size, freq);
        alSourceQueueBuffers(source, 1, &buffer);
    }
}

size_t OggDecoder::mem_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    OggDecoder *this_ = (OggDecoder *)datasource;

    long to_read = size * nmemb;
    if (to_read > this_->data_size - this_->data_pos)
        to_read = this_->data_size - this_->data_pos;
    if (to_read < 0)
        to_read = 0;
    memcpy(ptr, this_->data + this_->data_pos, to_read);
    this_->data_pos += to_read;
    return to_read / size;
}

int OggDecoder::mem_seek(void *datasource, ogg_int64_t offset, int whence)
{
    OggDecoder *this_ = (OggDecoder *)datasource;
    if (whence == SEEK_CUR)
        offset += this_->data_pos;
    else if (whence == SEEK_END)
        offset += this_->data_size;
    if (offset < 0)
        offset = 0;
    else if (offset > this_->data_size)
        offset = this_->data_size;
    this_->data_pos = offset;
    return 0;
}

long OggDecoder::mem_tell(void *datasource)
{
    OggDecoder *this_ = (OggDecoder *)datasource;
    return this_->data_pos;
}

///////////////////////////////////////////////////////////////////////////

/* for porting purposes... */
#ifndef STUBBED
#ifndef _DEBUG
#define STUBBED(x)
#else
#define STUBBED(x) { \
    static bool first_time = true; \
    if (first_time) { \
        first_time = false; \
        fprintf(stderr, "STUBBED: %s (%s, %s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
    } \
}
#endif
#endif

namespace FMOD {

#if _DEBUG
    #ifdef _MSC_VER
        #define bbgeDebugBreak _CrtDbgBreak
    #elif defined(__GNUC__) && ((__i386__) || (__x86_64__))
        #define bbgeDebugBreak() __asm__ __volatile__ ( "int $3\n\t" )
    #else
        #define bbgeDebugBreak() raise(SIGTRAP)
    #endif
    #define SANITY_CHECK_OPENAL_CALL() { \
        const ALenum err = alGetError(); \
        if (err != AL_NONE) { \
            fprintf(stderr, "WARNING: OpenAL error %s:%d: 0x%X\n", \
                        __FILE__, __LINE__, (int) err); \
            bbgeDebugBreak(); \
        } \
    }
#else
    #define SANITY_CHECK_OPENAL_CALL()
#endif


// simply nasty.
#define ALBRIDGE(cls,method,params,args) \
    FMOD_RESULT cls::method params { \
        if (!this) return FMOD_ERR_INTERNAL; \
        return ((OpenAL##cls *) this)->method args; \
    }

static ALenum GVorbisFormat = AL_NONE;

// FMOD::Sound implementation ...

class OpenALSound
{
public:
    OpenALSound(ttvfs::VFSFile *_fp, const bool _looping);
    OpenALSound(void *_data, long _size, const bool _looping);
    ttvfs::VFSFile *getFile() const { return fp; }
    const void *getData() const { return data; }
    long getSize() const { return size; }
    bool isLooping() const { return looping; }
    FMOD_RESULT release();
    void reference() { refcount++; }

private:
    ttvfs::VFSFile * const fp;
    void * const data;  // Only used if fp==NULL
    const long size;    // Only used if fp==NULL
    const bool looping;
    int refcount;
};

OpenALSound::OpenALSound(ttvfs::VFSFile *_fp, const bool _looping)
    : fp(_fp)
    , data(NULL)
    , size(0)
    , looping(_looping)
    , refcount(1)
{
    fp->ref++;
}

OpenALSound::OpenALSound(void *_data, long _size, const bool _looping)
    : fp(NULL)
    , data(_data)
    , size(_size)
    , looping(_looping)
    , refcount(1)
{
}

ALBRIDGE(Sound,release,(),())
FMOD_RESULT OpenALSound::release()
{
    refcount--;
    if (refcount <= 0)
    {
	if (fp)
    {
        fp->close();
        fp->dropBuf(true); // just in case there is a buffer...
        fp->ref--;
    }
	else
	    free(data);
        delete this;
    }
    return FMOD_OK;
}


class OpenALChannelGroup;

class OpenALChannel
{
public:
    OpenALChannel();
    FMOD_RESULT setVolume(const float _volume, const bool setstate=true);
    FMOD_RESULT setPaused(const bool _paused, const bool setstate=true);
    FMOD_RESULT setFrequency(const float _frequency);
    FMOD_RESULT setPriority(int _priority);
    FMOD_RESULT getPosition(unsigned int *position, FMOD_TIMEUNIT postype);
    FMOD_RESULT getVolume(float *_volume);
    FMOD_RESULT isPlaying(bool *isplaying);
    FMOD_RESULT setChannelGroup(ChannelGroup *channelgroup);
    FMOD_RESULT stop();
    void setGroupVolume(const float _volume);
    void setSourceName(const ALuint _sid) { sid = _sid; }
    ALuint getSourceName() const { return sid; }
    bool start(OpenALSound *sound);
    void update();
    void reacquire();
    bool isInUse() const { return inuse; }
    void setSound(OpenALSound *sound);

private:
    ALuint sid;  // source id.
    float groupvolume;
    float volume;
    bool paused;
    int priority;
    float frequency;
    OpenALChannelGroup *group;
    OpenALSound *sound;
    OggDecoder *decoder;
    bool inuse;
    bool initial;
};


class OpenALChannelGroup
{
public:
    OpenALChannelGroup(const char *_name);
    ~OpenALChannelGroup();
    FMOD_RESULT stop();
    FMOD_RESULT addDSP(DSP *dsp, DSPConnection **connection);
    FMOD_RESULT getPaused(bool *_paused);
    FMOD_RESULT setPaused(const bool _paused);
    FMOD_RESULT getVolume(float *_volume);
    FMOD_RESULT setVolume(const float _volume);
    bool attachChannel(OpenALChannel *channel);
    void detachChannel(OpenALChannel *channel);

private:
    const char *name;
    bool paused;
    int channel_count;
    OpenALChannel **channels;
    float volume;
};


// FMOD::Channel implementation...

OpenALChannel::OpenALChannel()
    : sid(0)
    , groupvolume(1.0f)
    , volume(1.0f)
    , paused(false)
    , priority(0)
    , frequency(1.0f)
    , group(NULL)
    , sound(NULL)
    , decoder(NULL)
    , inuse(false)
    , initial(true)
{
}

void OpenALChannel::reacquire()
{
    assert(!inuse);
    inuse = true;
    volume = 1.0f;
    paused = true;
    priority = 0;
    frequency = 1.0f;
    sound = NULL;
    initial = true;
}

void OpenALChannel::setGroupVolume(const float _volume)
{
    groupvolume = _volume;
    alSourcef(sid, AL_GAIN, volume * groupvolume);
    SANITY_CHECK_OPENAL_CALL();
}

bool OpenALChannel::start(OpenALSound *sound)
{
    if (decoder)
	delete decoder;
    if (sound->getFile())
	decoder = new OggDecoder(sound->getFile());
    else
	decoder = new OggDecoder(sound->getData(), sound->getSize());
    if (!decoder->start(sid, sound->isLooping()))
    {
	delete decoder;
	decoder = NULL;
	return false;
    }
    return true;
}

void OpenALChannel::update()
{
    if (inuse)
    {
	if (decoder)
	    decoder->update();
        ALint state = 0;
        alGetSourceiv(sid, AL_SOURCE_STATE, &state);
        SANITY_CHECK_OPENAL_CALL();
        if (state == AL_STOPPED)
            stop();
    }
}

ALBRIDGE(Channel,setVolume,(float volume),(volume))
FMOD_RESULT OpenALChannel::setVolume(const float _volume, const bool setstate)
{
    if (setstate)
        volume = _volume;
    alSourcef(sid, AL_GAIN, _volume * groupvolume);
    SANITY_CHECK_OPENAL_CALL();
    return FMOD_OK;
}

ALBRIDGE(Channel,getPosition,(unsigned int *position, FMOD_TIMEUNIT postype),(position,postype))
FMOD_RESULT OpenALChannel::getPosition(unsigned int *position, FMOD_TIMEUNIT postype)
{
    assert(postype == FMOD_TIMEUNIT_MS);
    if (decoder)
    {
        *position = (unsigned int) (decoder->position() * 1000.0);
    }
    else
    {
        ALfloat secs = 0.0f;
        alGetSourcefv(sid, AL_SEC_OFFSET, &secs);
        SANITY_CHECK_OPENAL_CALL();
        *position = (unsigned int) (secs * 1000.0f);
    }
    return FMOD_OK;
}

ALBRIDGE(Channel,getVolume,(float *volume),(volume))
FMOD_RESULT OpenALChannel::getVolume(float *_volume)
{
    *_volume = volume;
    return FMOD_OK;
}

ALBRIDGE(Channel,isPlaying,(bool *isplaying),(isplaying))
FMOD_RESULT OpenALChannel::isPlaying(bool *isplaying)
{
    // Apple's Mac OS X has a bug; alSourceRewind() doesn't make the sources
    //  AL_INITIAL again, so we have to track this ourselves.  :/
    if (initial)
    {
        *isplaying = true;
        return FMOD_OK;
    }

    ALint state = 0;
    alGetSourceiv(sid, AL_SOURCE_STATE, &state);
    SANITY_CHECK_OPENAL_CALL();
    *isplaying = ((state == AL_PLAYING) || (state == AL_INITIAL));
    if (state == AL_PAUSED)
        STUBBED("Should paused channels count as playing?");  // !!! FIXME
    return FMOD_OK;
}

ALBRIDGE(Channel,setChannelGroup,(ChannelGroup *channelgroup),(channelgroup))
FMOD_RESULT OpenALChannel::setChannelGroup(ChannelGroup *_channelgroup)
{
    OpenALChannelGroup *channelgroup = ((OpenALChannelGroup *) _channelgroup);
    assert(channelgroup);
    if (!channelgroup->attachChannel(this))
        return FMOD_ERR_INTERNAL;
    if ((group != NULL) && (group != channelgroup))
        group->detachChannel(this);
    group = channelgroup;
    return FMOD_OK;
}

ALBRIDGE(Channel,setFrequency,(float frequency),(frequency))
FMOD_RESULT OpenALChannel::setFrequency(const float _frequency)
{
    frequency = _frequency;
STUBBED("read the docs, verify this");
    alSourcef(sid, AL_PITCH, _frequency);
    SANITY_CHECK_OPENAL_CALL();
    return FMOD_OK;
}

ALBRIDGE(Channel,setPaused,(bool paused),(paused))
FMOD_RESULT OpenALChannel::setPaused(const bool _paused, const bool setstate)
{
    ALint state = 0;
    alGetSourceiv(sid, AL_SOURCE_STATE, &state);
    SANITY_CHECK_OPENAL_CALL();
    if ((_paused) && (state == AL_PLAYING))
    {
        alSourcePause(sid);
        initial = false;
        SANITY_CHECK_OPENAL_CALL();
    }
    else if ((!_paused) && (initial || ((state == AL_INITIAL) || (state == AL_PAUSED))))
    {
        alSourcePlay(sid);
        initial = false;
        SANITY_CHECK_OPENAL_CALL();
    }

    if (setstate)
        paused = _paused;

    return FMOD_OK;
}

ALBRIDGE(Channel,setPriority,(int priority),(priority))
FMOD_RESULT OpenALChannel::setPriority(int _priority)
{
    priority = _priority;
    return FMOD_OK;
}


// FMOD::ChannelGroup implementation...

OpenALChannelGroup::OpenALChannelGroup(const char *_name)
    : name(NULL)
    , paused(false)
    , channel_count(0)
    , channels(NULL)
    , volume(1.0f)
{
    if (_name)
    {
        char *buf = new char[strlen(_name) + 1];
        strcpy(buf, _name);
        name = buf;
    }
}

OpenALChannelGroup::~OpenALChannelGroup()
{
    delete[] name;
}

bool OpenALChannelGroup::attachChannel(OpenALChannel *channel)
{
    channel->setGroupVolume(volume);

    for (int i = 0; i < channel_count; i++)
    {
        if (channels[i] == channel)
            return true;
    }

    void *ptr = realloc(channels, sizeof (OpenALChannel *) * (channel_count + 1));
    if (ptr == NULL)
        return false;

    channels = (OpenALChannel **) ptr;
    channels[channel_count++] = channel;
    return true;
}

void OpenALChannelGroup::detachChannel(OpenALChannel *channel)
{
    for (int i = 0; i < channel_count; i++)
    {
        if (channels[i] == channel)
        {
            if (i < (channel_count-1))
                memmove(&channels[i], &channels[i+1], sizeof (OpenALChannel *) * ((channel_count - i) - 1));
            channel_count--;
            return;
        }
    }

    assert(false && "Detached a channel that isn't part of the group!");
}


ALBRIDGE(ChannelGroup,addDSP,(DSP *dsp, DSPConnection **connection),(dsp,connection))
FMOD_RESULT OpenALChannelGroup::addDSP(DSP *dsp, DSPConnection **connection)
{
    STUBBED("write me");
    return FMOD_ERR_INTERNAL;
}

ALBRIDGE(ChannelGroup,getPaused,(bool *paused),(paused))
FMOD_RESULT OpenALChannelGroup::getPaused(bool *_paused)
{
    *_paused = paused;
    return FMOD_OK;
}

ALBRIDGE(ChannelGroup,getVolume,(float *volume),(volume))
FMOD_RESULT OpenALChannelGroup::getVolume(float *_volume)
{
    *_volume = volume;
    return FMOD_OK;
}

ALBRIDGE(ChannelGroup,setPaused,(bool paused),(paused))
FMOD_RESULT OpenALChannelGroup::setPaused(const bool _paused)
{
    for (int i = 0; i < channel_count; i++)
        channels[i]->setPaused(_paused, false);
    paused = _paused;
    return FMOD_OK;
}

ALBRIDGE(ChannelGroup,setVolume,(float volume),(volume))
FMOD_RESULT OpenALChannelGroup::setVolume(const float _volume)
{
    volume = _volume;
    for (int i = 0; i < channel_count; i++)
        channels[i]->setGroupVolume(_volume);
    return FMOD_OK;
}

ALBRIDGE(ChannelGroup,stop,(),())
FMOD_RESULT OpenALChannelGroup::stop()
{
    for (int i = 0; i < channel_count; i++)
        channels[i]->stop();
    return FMOD_OK;
}


// FMOD::DSP implementation...

FMOD_RESULT DSP::getActive(bool *active)
{
    STUBBED("write me");
    *active = false;
    return FMOD_ERR_INTERNAL;
}

FMOD_RESULT DSP::remove()
{
    STUBBED("write me");
    return FMOD_ERR_INTERNAL;
}

FMOD_RESULT DSP::setParameter(int index, float value)
{
    STUBBED("write me");
    return FMOD_ERR_INTERNAL;
}


void OpenALChannel::setSound(OpenALSound *_sound)
{
    if (sound)
        sound->release();

    sound = _sound;

    if (sound)
        sound->reference();
}


ALBRIDGE(Channel,stop,(),())
FMOD_RESULT OpenALChannel::stop()
{
    if (decoder)
    {
	delete decoder;
	decoder = NULL;
    }
    alSourceStop(sid);
    SANITY_CHECK_OPENAL_CALL();
    alSourcei(sid, AL_BUFFER, 0);
    SANITY_CHECK_OPENAL_CALL();
    if (sound)
    {
        sound->release();
        sound = NULL;
    }
    paused = false;
    inuse = false;
    initial = false;
    return FMOD_OK;
}



// FMOD::System implementation ...

class OpenALSystem
{
public:
    OpenALSystem();
    ~OpenALSystem();
    FMOD_RESULT init(int maxchannels, const FMOD_INITFLAGS flags, const void *extradriverdata, std::string defaultDevice);
    FMOD_RESULT update();
    FMOD_RESULT release();
    FMOD_RESULT getVersion(unsigned int *version);
    FMOD_RESULT setSpeakerMode(const FMOD_SPEAKERMODE speakermode);
    FMOD_RESULT setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, const int blockalign);
    FMOD_RESULT setDSPBufferSize(const unsigned int bufferlength, const int numbuffers);
    FMOD_RESULT createChannelGroup(const char *name, ChannelGroup **channelgroup);
    FMOD_RESULT createDSPByType(const FMOD_DSP_TYPE type, DSP **dsp);
    FMOD_RESULT createSound(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound);
    FMOD_RESULT createStream(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound);
    FMOD_RESULT getDriverCaps(const int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode);
    FMOD_RESULT getMasterChannelGroup(ChannelGroup **channelgroup);
    FMOD_RESULT playSound(FMOD_CHANNELINDEX channelid, Sound *sound, bool paused, Channel **channel);

    FMOD_RESULT getNumChannels(int *maxchannels_ret);

private:
    OpenALChannelGroup *master_channel_group;
    int num_channels;
    OpenALChannel *channels;
};


OpenALSystem::OpenALSystem()
    : master_channel_group(NULL)
    , num_channels(0)
    , channels(NULL)
{
}

OpenALSystem::~OpenALSystem()
{
    delete master_channel_group;
    delete[] channels;
}


FMOD_RESULT System_Create(FMOD_SYSTEM **system)
{
    *system = (FMOD_SYSTEM *) new OpenALSystem;
    return FMOD_OK;
}

ALBRIDGE(System,createChannelGroup,(const char *name, ChannelGroup **channelgroup),(name,channelgroup))
FMOD_RESULT OpenALSystem::createChannelGroup(const char *name, ChannelGroup **channelgroup)
{
    *channelgroup = (ChannelGroup *) new OpenALChannelGroup(name);
    return FMOD_OK;
}

ALBRIDGE(System,createDSPByType,(FMOD_DSP_TYPE type, DSP **dsp),(type,dsp))
FMOD_RESULT OpenALSystem::createDSPByType(const FMOD_DSP_TYPE type, DSP **dsp)
{
    *dsp = NULL;
    STUBBED("write me");
    return FMOD_ERR_INTERNAL;
}

ALBRIDGE(System,createSound,(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound),(name_or_data,mode,exinfo,sound))
FMOD_RESULT OpenALSystem::createSound(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound)
{
    assert(!exinfo);

    // !!! FIXME: if it's not Ogg, we don't have a decoder. I'm lazy.  :/
    char *fname = (char *) alloca(strlen(name_or_data) + 16);
    strcpy(fname, name_or_data);
    char *ptr = strrchr(fname, '.');
    if (ptr) *ptr = '\0';
    strcat(fname, ".ogg");

    ttvfs::VFSFile *vf = core->vfs.GetFile(fname);
    if(!vf)
        return FMOD_ERR_INTERNAL;

    if(mode & FMOD_CREATESTREAM)
    {
        // does it make sense to try to stream from anything else than an actual file on disk?
        // Files inside containers are always loaded into memory, unless on-the-fly partial decompression is implemented...
        // A typical ogg is < 3 MB in size, if that is preloaded and then decoded over time it should still be a big gain.
        if(!vf->isopen())
            vf->open(NULL, "rb");
        else
            vf->seek(0);

        *sound = (Sound *) new OpenALSound(vf, (((mode & FMOD_LOOP_OFF) == 0) && (mode & FMOD_LOOP_NORMAL)));
        return FMOD_OK;
    }

    // if we are here, create & preload & pre-decode full buffer
    vf->getBuf(); // force early size detection
    void *data = malloc(vf->size()); // because release() will use free() ...
    if (!(data && vf->getBuf()))
    {
        debugLog("Out of memory for " + std::string(fname));
        vf->close();
        vf->dropBuf(true);
        return FMOD_ERR_INTERNAL;
    }
    memcpy(data, vf->getBuf(), vf->size());
    core->addVFSFileForDrop(vf);
    *sound = (Sound *) new OpenALSound(data, vf->size(), (((mode & FMOD_LOOP_OFF) == 0) && (mode & FMOD_LOOP_NORMAL)));

    return FMOD_OK;
}

ALBRIDGE(System,createStream,(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound),(name_or_data,mode,exinfo,sound))
FMOD_RESULT OpenALSystem::createStream(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound)
{
    return createSound(name_or_data, mode | FMOD_CREATESTREAM, exinfo, sound);
}

ALBRIDGE(System,getDriverCaps,(int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode),(id,caps,minfrequency,maxfrequency,controlpanelspeakermode))
FMOD_RESULT OpenALSystem::getDriverCaps(const int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode)
{
    assert(!id);
    assert(!minfrequency);
    assert(!maxfrequency);
    *controlpanelspeakermode = FMOD_SPEAKERMODE_STEREO;  // not strictly true, but works for aquaria's usage.
    *caps = 0;   // aquaria only checks FMOD_CAPS_HARDWARE_EMULATED.
    return FMOD_OK;
}

ALBRIDGE(System,getMasterChannelGroup,(ChannelGroup **channelgroup),(channelgroup))
FMOD_RESULT OpenALSystem::getMasterChannelGroup(ChannelGroup **channelgroup)
{
    *channelgroup = (ChannelGroup *) master_channel_group;
    return FMOD_OK;
}

ALBRIDGE(System,getVersion,(unsigned int *version),(version))
FMOD_RESULT OpenALSystem::getVersion(unsigned int *version)
{
    *version = FMOD_VERSION;
    return FMOD_OK;
}

ALBRIDGE(System,init,(int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata, std::string defaultDevice),(maxchannels,flags,extradriverdata, defaultDevice))
FMOD_RESULT OpenALSystem::init(int maxchannels, const FMOD_INITFLAGS flags, const void *extradriverdata, std::string defaultDevice)
{
	ALCdevice *dev = NULL;

	if (!defaultDevice.empty())
	{
		dev = alcOpenDevice(defaultDevice.c_str());  // Try to use device specified in user config
	}

	if (!dev)
	{
		dev = alcOpenDevice(NULL); // Fall back to system default device
	}

    if (!dev)
	{
        return FMOD_ERR_INTERNAL;
	}

    // OpenAL doesn't provide a way to request sources that can be either
    // mono or stereo, so we need to request both separately (thus allocating
    // twice the theoretical requirement -- oh well).  --achurch
    ALCint requested_attributes[5];
    requested_attributes[0] = ALC_MONO_SOURCES;
    requested_attributes[1] = maxchannels;
    requested_attributes[2] = ALC_STEREO_SOURCES;
    requested_attributes[3] = maxchannels;
    requested_attributes[4] = 0;
    ALCcontext *ctx = alcCreateContext(dev, requested_attributes);
    if (!ctx)
    {
        alcCloseDevice(dev);
        return FMOD_ERR_INTERNAL;
    }

    ALCint num_attributes = 0;
    alcGetIntegerv(dev, ALC_ATTRIBUTES_SIZE, 1, &num_attributes);
    if (num_attributes > 0) {
        ALCint *attributes = new ALCint[num_attributes];
        alcGetIntegerv(dev, ALC_ALL_ATTRIBUTES, num_attributes, attributes);
        int i;
        for (i = 0; i < num_attributes; i += 2) {
            if (attributes[i] == ALC_MONO_SOURCES) {
                if (attributes[i+1] <= 0) {
                    debugLog("Couldn't get any mono sources, aborting");
                    alcDestroyContext(ctx);
                    alcCloseDevice(dev);
                    return FMOD_ERR_INTERNAL;
                } else if (attributes[i+1] < num_channels) {
                    std::ostringstream os;
                    os << "Only got " << attributes[i+1] << " of "
                       << maxchannels << " mono sources";
                    debugLog(os.str());
                    maxchannels = attributes[i+1];
                }
            } else if (attributes[i] == ALC_STEREO_SOURCES) {
                if (attributes[i+1] <= 0) {
                    debugLog("Couldn't get any stereo sources, aborting");
                    alcDestroyContext(ctx);
                    alcCloseDevice(dev);
                    return FMOD_ERR_INTERNAL;
                } else if (attributes[i+1] < num_channels) {
                    std::ostringstream os;
                    os << "Only got " << attributes[i+1] << " of "
                       << maxchannels << " stereo sources";
                    debugLog(os.str());
                    maxchannels = attributes[i+1];
                }
            }
        }
        delete[] attributes;
    } else {
        debugLog("WARNING: couldn't get device attributes!");
    }

    alcMakeContextCurrent(ctx);
    alcProcessContext(ctx);

    #ifdef _DEBUG
    printf("AL_VENDOR: %s\n", (char *) alGetString(AL_VENDOR));
    printf("AL_RENDERER: %s\n", (char *) alGetString(AL_RENDERER));
    printf("AL_VERSION: %s\n", (char *) alGetString(AL_VERSION));
    printf("AL_EXTENSIONS: %s\n", (char *) alGetString(AL_EXTENSIONS));
    #endif

    SANITY_CHECK_OPENAL_CALL();

    GVorbisFormat = AL_NONE;
    if (alIsExtensionPresent("AL_EXT_vorbis"))
        GVorbisFormat = alGetEnumValue("AL_FORMAT_VORBIS_EXT");

#if 0  // Disabled output: every bug report thinks this is the culprit. --ryan.
    if (GVorbisFormat == AL_NONE)
        fprintf(stderr, "WARNING: no AL_EXT_vorbis support. We'll use more RAM.\n");
#endif

    SANITY_CHECK_OPENAL_CALL();

    master_channel_group = new OpenALChannelGroup("master");

    num_channels = maxchannels;
    channels = new OpenALChannel[maxchannels];
    ALenum err = alGetError();  // clear any existing error state.
    for (int i = 0; i < num_channels; i++)
    {
        ALuint sid = 0;
        alGenSources(1, &sid);
        err = alGetError();
        if (err != AL_NONE)
        {
            char errmsg[512];
            sprintf(errmsg, "WARNING: OpenAL error %s:%d: 0x%X\n", __FILE__, __LINE__, (int) err);
            debugLog(errmsg);
            num_channels = i - 1; // last channel that worked
            break;
        }

        alSourcei(sid, AL_SOURCE_RELATIVE, AL_TRUE);
        SANITY_CHECK_OPENAL_CALL();
        alSource3f(sid, AL_POSITION, 0.0f, 0.0f, 0.0f);  // no panning or spatialization in Aquaria.
        SANITY_CHECK_OPENAL_CALL();
        channels[i].setSourceName(sid);
        channels[i].setChannelGroup((ChannelGroup *) master_channel_group);
    }
    std::stringstream ss;
    ss << "Using " << num_channels << " sound channels.";
    debugLog(ss.str());
    return FMOD_OK;
}

ALBRIDGE(System,getNumChannels,(int *maxchannels_ret),(maxchannels_ret))
FMOD_RESULT OpenALSystem::getNumChannels(int *maxchannels_ret)
{
    *maxchannels_ret = num_channels;
    return FMOD_OK;
}

ALBRIDGE(System,playSound,(FMOD_CHANNELINDEX channelid, Sound *sound, bool paused, Channel **channel),(channelid,sound,paused,channel))
FMOD_RESULT OpenALSystem::playSound(FMOD_CHANNELINDEX channelid, Sound *_sound, bool paused, Channel **channel)
{
    *channel = NULL;

    if (channelid == FMOD_CHANNEL_FREE)
    {
        for (int i = 0; i < num_channels; i++)
        {
            if (!channels[i].isInUse())
            {
                channelid = (FMOD_CHANNELINDEX) i;
                break;
            }
        }
    }

    if ((channelid < 0) || (channelid >= num_channels))
        return FMOD_ERR_INTERNAL;

    OpenALSound *sound = (OpenALSound *) _sound;
    const ALuint sid = channels[channelid].getSourceName();
    // alSourceRewind doesn't work right on some versions of Mac OS X.
    alSourceStop(sid);  // stop any playback, set to AL_INITIAL.
    alSourceRewind(sid);  // stop any playback, set to AL_INITIAL.
    SANITY_CHECK_OPENAL_CALL();
    alSourcei(sid, AL_BUFFER, 0);  // Reset state to AL_UNDETERMINED.
    SANITY_CHECK_OPENAL_CALL();

    if (!channels[channelid].start(sound))
       return FMOD_ERR_INTERNAL;

    channels[channelid].reacquire();
    channels[channelid].setPaused(paused);
    channels[channelid].setSound(sound);
    *channel = (Channel *) &channels[channelid];
    return FMOD_OK;
}


ALBRIDGE(System,release,(),())
FMOD_RESULT OpenALSystem::release()
{
    ALCcontext *ctx = alcGetCurrentContext();
    if (ctx)
    {
        for (int i = 0; i < num_channels; i++)
        {
            const ALuint sid = channels[i].getSourceName();
            channels[i].setSourceName(0);
            channels[i].setSound(NULL);
            alSourceStop(sid);
            alSourcei(sid, AL_BUFFER, 0);
            alDeleteSources(1, &sid);
        }
        ALCdevice *dev = alcGetContextsDevice(ctx);
        alcMakeContextCurrent(NULL);
        alcSuspendContext(ctx);
        alcDestroyContext(ctx);
        alcCloseDevice(dev);
    }
    delete this;
    return FMOD_OK;
}

ALBRIDGE(System,setDSPBufferSize,(unsigned int bufferlength, int numbuffers),(bufferlength,numbuffers))
FMOD_RESULT OpenALSystem::setDSPBufferSize(const unsigned int bufferlength, const int numbuffers)
{
    // aquaria only uses this for FMOD_CAPS_HARDWARE_EMULATED, so I skipped it.
    return FMOD_ERR_INTERNAL;
}

ALBRIDGE(System,setFileSystem,(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, int blockalign),(useropen,userclose,userread,userseek,blockalign))
FMOD_RESULT OpenALSystem::setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, const int blockalign)
{
    // Aquaria sets these, but they don't do anything fancy, so we ignore them for now.
    return FMOD_OK;
}

ALBRIDGE(System,setSpeakerMode,(FMOD_SPEAKERMODE speakermode),(speakermode))
FMOD_RESULT OpenALSystem::setSpeakerMode(const FMOD_SPEAKERMODE speakermode)
{
    return FMOD_OK;  // we ignore this for Aquaria.
}

ALBRIDGE(System,update,(),())
FMOD_RESULT OpenALSystem::update()
{
    alcProcessContext(alcGetCurrentContext());
    for (int i = 0; i < num_channels; i++)
        channels[i].update();
#if _DEBUG
    const ALenum err = alGetError();
    if (err != AL_NONE)
        fprintf(stderr, "WARNING: OpenAL error this frame: 0x%X\n", (int) err);
#endif
    return FMOD_OK;
}


// misc FMOD bits...

FMOD_RESULT Memory_GetStats(int *currentalloced, int *maxalloced, FMOD_BOOL blocking)
{
    // not ever used by Aquaria.
    *currentalloced = *maxalloced = 42;
    return FMOD_ERR_INTERNAL;
}

}  // namespace FMOD

// end of FmodOpenALBridge.cpp ...


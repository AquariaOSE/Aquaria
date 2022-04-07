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

#include "SDL.h"

#include "Base.h"
#include "Core.h"
#include "ttvfs_stdio.h"

#include "FmodOpenALBridge.h"

#define AL_LIBTYPE_STATIC
#include "al.h"
#include "alc.h"

#include "ogg/ogg.h"
#include "vorbis/vorbisfile.h"

#include "MT.h"

#undef min
#undef max

// HACK: Fix this == NULL checks with GCC 6 and up
#if defined(__GNUC__) || defined (__clang__)
#pragma GCC optimize("no-delete-null-pointer-checks")
#endif

// HACK: global because OpenAL has only one listener anyway
static FMOD_VECTOR s_listenerPos;

///////////////////////////////////////////////////////////////////////////

// Decoder implementation for streamed Ogg Vorbis audio.

class OggDecoder {
public:
    // Create a decoder that streams from a file.
    OggDecoder(VFILE *fp);

    // Create a decoder that streams from a memory buffer.
    OggDecoder(const void *data, long data_size);

    ~OggDecoder();

    // Prepare playing on the given channel.
    bool preStart(ALuint source);

    // Decodes the first few buffers, starts the actual playback and detaches the decoder
    // from the main thread, with optional looping.
    void start(bool loop);

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

    static void startDecoderThread();
    static void stopDecoderThread();

    int getNumChannels() const { return channels; }

    void setForceMono(bool mono) { forcemono = mono; }

private:

    void _stop();

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
    VFILE *fp;
    const char *data;
    long data_size;
    long data_pos;  // Current read position for memory buffers

    OggVorbis_File vf;
    ALenum format;
    int channels;
    int freq;
    bool forcemono;

    bool thread; // true if played by background thread

    bool playing;
    bool loop;
    bool eof;  // End of file _or_ unrecoverable error encountered
    bool stopped; // true if enqueued deletion
    unsigned int samples_done;  // Number of samples played and dequeued

    static SDL_Thread *decoderThread;
    static LockedQueue<OggDecoder*> decoderQ;
    static volatile bool stop_thread;
    static std::list<OggDecoder*> decoderList; // used by decoder thread only


    static void detachDecoder(OggDecoder *);
};

// File I/O callback set (OV_CALLBACKS_NOCLOSE from libvorbis 1.2.0).
// It might be better to just update libogg/libvorbis to the current
// versions so we don't have to worry about identifier collisions --
// we can then drop all this and use OV_CALLBACKS_NOCLOSE in the
// ov_open_callbacks() call.  Note that we rename the fseek() wrapper
// to avoid an identifier collision when building with more recent
// versions of libvorbis.
static int BBGE_ov_header_fseek_wrap(VFILE *f,ogg_int64_t off,int whence){
  if(f==NULL)return(-1);
  return vfseek(f,(long int)off,whence); // no ogg file is larger than 4 GB, int-cast should be ok
}
static int noclose(FILE *f) {return 0;}
static const ov_callbacks local_OV_CALLBACKS_NOCLOSE = {
  (size_t (*)(void *, size_t, size_t, void *))  vfread,
  (int (*)(void *, ogg_int64_t, int))           BBGE_ov_header_fseek_wrap,
  (int (*)(void *))                             noclose,  // NULL doesn't work in libvorbis-1.1.2
  (long (*)(void *))                            vftell
};

// Memory I/O callback set.
static const ov_callbacks ogg_memory_callbacks = {
    OggDecoder::mem_read,
    OggDecoder::mem_seek,
    (int (*)(void *))noclose,
    OggDecoder::mem_tell
};

SDL_Thread *OggDecoder::decoderThread = NULL;
LockedQueue<OggDecoder*> OggDecoder::decoderQ;
volatile bool OggDecoder::stop_thread;
std::list<OggDecoder*> OggDecoder::decoderList;

void OggDecoder::startDecoderThread()
{
    stop_thread = false;
#if SDL_VERSION_ATLEAST(2,0,0)
    decoderThread = SDL_CreateThread((int (*)(void *))decode_loop, "OggDecoder", NULL);
#else
    decoderThread = SDL_CreateThread((int (*)(void *))decode_loop, NULL);
#endif
    if (!decoderThread)
    {
        debugLog("Failed to create Ogg Vorbis decode thread: "
                 + std::string(SDL_GetError()));
    }
}

void OggDecoder::stopDecoderThread()
{
    if (decoderThread)
    {
        stop_thread = true;
        debugLog("Waiting for decoder thread to exit...");
        SDL_WaitThread(decoderThread, NULL);
        decoderThread = NULL;
    }
}

void OggDecoder::detachDecoder(OggDecoder *ogg)
{
    if(decoderThread)
    {
        ogg->thread = true;
        decoderQ.push(ogg);
    }
}

void OggDecoder::decode_loop(OggDecoder *this_)
{
    while (!this_->stop_thread)
    {
        SDL_Delay(10);
        // Transfer decoder to this background thread
        OggDecoder *ogg;
        while(decoderQ.pop(ogg))
            decoderList.push_back(ogg);

        for(std::list<OggDecoder*>::iterator it = decoderList.begin(); it != decoderList.end(); )
        {
            ogg = *it;
            if (ogg->playing)
            {
                int processed = 0;
                alGetSourcei(ogg->source, AL_BUFFERS_PROCESSED, &processed);
                for (int i = 0; i < processed; i++)
                {
                    ogg->samples_done += BUFFER_LENGTH;
                    ALuint buffer = 0;
                    alSourceUnqueueBuffers(ogg->source, 1, &buffer);
                    if (buffer)
                        ogg->queue(buffer);
                }
                ++it;
            }
            else
            {
                delete ogg;
                decoderList.erase(it++);
            }
        }

        core->dbg_numThreadDecoders = decoderList.size();
    }
}


OggDecoder::OggDecoder(VFILE *fp)
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
    this->thread = false;
    this->playing = false;
    this->loop = false;
    this->eof = false;
    this->samples_done = 0;
    this->stopped = false;
    this->format = 0;
    this->channels = 0;
    this->forcemono = false;
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
    this->thread = false;
    this->playing = false;
    this->loop = false;
    this->eof = false;
    this->samples_done = 0;
    this->stopped = false;
    this->format = 0;
    this->channels = 0;
    this->forcemono = false;
}

OggDecoder::~OggDecoder()
{
    _stop();

    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        if (buffers[i])
            alDeleteBuffers(1, &buffers[i]);
    }
}

bool OggDecoder::preStart(ALuint source)
{
    this->source = source;

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
    channels = info->channels;
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

    return true;
}

void OggDecoder::start(bool loop)
{
    this->loop = loop;

    playing = true;
    eof = false;
    samples_done = 0;
    for (int i = 0; i < NUM_BUFFERS; i++)
        queue(buffers[i]);

    detachDecoder(this);
}

void OggDecoder::update()
{
    if (!playing || thread)
        return;

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
    if (thread)
        playing = false; // The background thread will take care of deletion then.
    else
        delete this;
}

void OggDecoder::_stop()
{
    playing = false;

    if (stopped)
        return;

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
    stopped = true;
}

double OggDecoder::position()
{
    ALint samples_played = 0;
    alGetSourcei(source, AL_SAMPLE_OFFSET, &samples_played);
    samples_played += samples_done;
    return (double)samples_played / (double)freq;
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define BBGE_BIGENDIAN 1
#else
#define BBGE_BIGENDIAN 0
#endif

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
            /*bigendianp*/ BBGE_BIGENDIAN, /*word*/ 2, /*sgned*/ 1,
            &bitstream_unused
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
        ALuint fmt = format;
        if(channels == 2 && forcemono)
        {
            signed short *buf = (short*)&pcm_buffer[0];
            int numSamples = pcm_size / 2; // 16 bit samples
            int j = 0;
            for (int i = 0; i < numSamples ; i += 2)
            {
                // This is in theory not quite correct, but it doesn't add any artifacts or clipping.
                // FIXME: Seems that simple and stupid is the method of choice, then... -- FG
                buf[j++] = (buf[i] + buf[i+1]) / 2;
            }
            pcm_size = numSamples;
            fmt = AL_FORMAT_MONO16;
        }
        alBufferData(buffer, fmt, pcm_buffer, pcm_size, freq);
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

// HACK: works fairly well without it. Annoying to be thrown into the debugger because all channels are full.
#undef SANITY_CHECK_OPENAL_CALL
#define SANITY_CHECK_OPENAL_CALL()

// simply nasty.
#define ALBRIDGE(cls,method,params,args) \
    FMOD_RESULT cls::method params { \
        if (!this) return FMOD_ERR_INTERNAL; \
        return ((OpenAL##cls *) this)->method args; \
    }

// FMOD::Sound implementation ...

class OpenALSound
{
public:
    OpenALSound(VFILE *_fp, const bool _looping); // ctor for ogg streamed from file
    OpenALSound(void *_data, size_t _size, const bool _looping); // ctor for ogg streamed from memory
    OpenALSound(ALuint _bid, const bool _looping); // ctor for raw samples already assigned an opanAL buffer ID
    VFILE *getFile() const { return fp; }
    const void *getData() const { return data; }
    long getSize() const { return size; }
    bool isLooping() const { return looping; }
    bool isRaw() const { return raw; }
    FMOD_RESULT release();
    FMOD_RESULT getFormat(FMOD_SOUND_TYPE *type, FMOD_SOUND_FORMAT *format, int *channels, int *bits);
    void reference() { refcount++; }
    ALuint getBufferName() const { return bid; }
    int getNumChannels() const { return numChannels; }
    void setNumChannels(int c) { numChannels = c; }

private:
    VFILE * const fp;
    void * const data;  // Only used if fp==NULL
    const size_t size;    // Only used if fp==NULL
    const bool looping;
    int refcount;
    const bool raw; // true if buffer holds raw PCM data
    ALuint bid; // only used if raw == true
    int numChannels;
};

OpenALSound::OpenALSound(VFILE *_fp, const bool _looping)
    : fp(_fp)
    , data(NULL)
    , size(0)
    , looping(_looping)
    , refcount(1)
    , raw(false)
    , bid(0)
    , numChannels(0)
{
}

OpenALSound::OpenALSound(void *_data, size_t _size, const bool _looping)
    : fp(NULL)
    , data(_data)
    , size(_size)
    , looping(_looping)
    , refcount(1)
    , raw(false)
    , bid(0)
    , numChannels(0)
{
}

OpenALSound::OpenALSound(ALuint _bid, const bool _looping)
    : fp(NULL)
    , data(NULL)
    , size(0)
    , looping(_looping)
    , refcount(1)
    , raw(true)
    , bid(_bid)
    , numChannels(0)
{
}

ALBRIDGE(Sound,release,(),())
FMOD_RESULT OpenALSound::release()
{
    refcount--;
    if (refcount <= 0)
    {
        if(raw)
        {
            alDeleteBuffers(1, &bid);
        }
        else
        {
            if (fp)
                vfclose(fp);
            else
                free(data);
        }
        delete this;
    }
    return FMOD_OK;
}

ALBRIDGE(Sound,getFormat,(FMOD_SOUND_TYPE *type, FMOD_SOUND_FORMAT *format, int *channels, int *bits),(type, format, channels, bits))
FMOD_RESULT OpenALSound::getFormat(FMOD_SOUND_TYPE *type, FMOD_SOUND_FORMAT *format, int *channels, int *bits)
{
    if(channels)
        *channels = getNumChannels();
    return FMOD_OK;
}


class OpenALChannelGroup;

class OpenALChannel
{
public:
    OpenALChannel();
    FMOD_RESULT setVolume(const float _volume);
    FMOD_RESULT setPaused(const bool _paused, const bool setstate=true);
    FMOD_RESULT setFrequency(const float _frequency);
    FMOD_RESULT setPriority(int _priority);
    FMOD_RESULT getPosition(unsigned int *position, FMOD_TIMEUNIT postype);
    FMOD_RESULT getVolume(float *_volume);
    FMOD_RESULT isPlaying(bool *isplaying);
    FMOD_RESULT setChannelGroup(ChannelGroup *channelgroup);
    FMOD_RESULT stop();
    FMOD_RESULT setCallback(FMOD_CHANNEL_CALLBACK callback);
    FMOD_RESULT getUserData(void **userdata);
    FMOD_RESULT setUserData(void *userdata);
    FMOD_RESULT set3DAttributes(const FMOD_VECTOR *pos, const FMOD_VECTOR *vel);
    FMOD_RESULT set3DMinMaxDistance(float mindistance, float maxdistance);
    FMOD_RESULT setMode(FMOD_MODE mode);
    FMOD_RESULT getMode(FMOD_MODE *mode);

    void setGroupVolume(const float _volume);
    void setDistanceVolume(float _volume);
    void setSourceName(const ALuint _sid) { sid = _sid; }
    ALuint getSourceName() const { return sid; }
    bool start(OpenALSound *sound);
    void update();
    void reacquire();
    bool isInUse() const { return inuse; }
    void setSound(OpenALSound *sound);

private:
    void applyVolume();

    ALuint sid;  // source id.
    float groupvolume;
    float volume;
    float distvolume;
    bool paused;
    int priority;
    float frequency;
    OpenALChannelGroup *group;
    OpenALSound *sound;
    OggDecoder *decoder;
    bool inuse;
    bool initial;
    FMOD_CHANNEL_CALLBACK callback;
    void *userdata;
    FMOD_MODE _mode;
    float mindist;
    float maxdist;
    bool relative;
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
    , distvolume(1.0f)
    , paused(false)
    , priority(0)
    , frequency(1.0f)
    , group(NULL)
    , sound(NULL)
    , decoder(NULL)
    , inuse(false)
    , initial(true)
    , _mode(FMOD_DEFAULT)
    , mindist(0.0f)
    , maxdist(0.0f)
    , relative(false)
{
}

void OpenALChannel::reacquire()
{
    assert(!inuse);
    inuse = true;
    volume = 1.0f;
    distvolume = 1.0f;
    paused = true;
    priority = 0;
    frequency = 1.0f;
    sound = NULL;
    initial = true;
    mindist = 0.0f;
    maxdist = 0.0f;
    relative = false;
}

void OpenALChannel::setGroupVolume(const float _volume)
{
    groupvolume = _volume;
    applyVolume();
}

bool OpenALChannel::start(OpenALSound *sound)
{
    if (decoder)
        delete decoder;
    if (sound->isRaw())
    {
        alSourcei(sid, AL_BUFFER, sound->getBufferName());
        alSourcei(sid, AL_LOOPING, sound->isLooping() ? AL_TRUE : AL_FALSE);
    }
    else
    {
        if (sound->getFile())
            decoder = new OggDecoder(sound->getFile());
        else
            decoder = new OggDecoder(sound->getData(), sound->getSize());
        if (!decoder->preStart(sid))
        {
            delete decoder;
            decoder = NULL;
            return false;
        }
        sound->setNumChannels(decoder->getNumChannels());
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
FMOD_RESULT OpenALChannel::setVolume(const float _volume)
{
    volume = _volume;
    applyVolume();
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
        if (initial && decoder)
        {
            decoder->setForceMono(mindist || maxdist); // HACK: this is set for positional sounds.
            decoder->start(sound->isLooping());
        }
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

ALBRIDGE(Channel,stop,(),())
FMOD_RESULT OpenALChannel::stop()
{
    if (decoder)
    {
        decoder->stop();
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
    if (inuse && callback)
        callback(this, FMOD_CHANNEL_CALLBACKTYPE_END, NULL, NULL); // HACK: commanddata missing (but they are not used by the callback)
    paused = false;
    inuse = false;
    initial = false;
    return FMOD_OK;
}


ALBRIDGE(Channel,setCallback,(FMOD_CHANNEL_CALLBACK callback),(callback))
FMOD_RESULT OpenALChannel::setCallback(FMOD_CHANNEL_CALLBACK callback)
{
    this->callback = callback;
    return FMOD_OK;
}

ALBRIDGE(Channel,getUserData,(void **userdata),(userdata))
FMOD_RESULT OpenALChannel::getUserData(void **userdata)
{
    *userdata = this->userdata;
    return FMOD_OK;
}

ALBRIDGE(Channel,setUserData,(void *userdata),(userdata))
FMOD_RESULT OpenALChannel::setUserData(void *userdata)
{
    this->userdata = userdata;
    return FMOD_OK;
}

ALBRIDGE(Channel,set3DAttributes,(const FMOD_VECTOR *pos, const FMOD_VECTOR *vel),(pos, vel))
FMOD_RESULT OpenALChannel::set3DAttributes(const FMOD_VECTOR *pos, const FMOD_VECTOR *vel)
{
    if (pos)
    {
        alSource3f(sid, AL_POSITION, pos->x, pos->y, pos->z);

        if(maxdist == mindist)
            setDistanceVolume(1.0f);
        else
        {
            // This is where custom distance attenuation starts.
            float dx, dy, dz;
            if(relative)
            {
                dx = pos->x;
                dy = pos->y;
                dz = pos->z;
            }
            else
            {
                dx = s_listenerPos.x - pos->x;
                dy = s_listenerPos.y - pos->y;
                dz = s_listenerPos.z - pos->z;
            }

            float d2 = dx*dx + dy*dy + dz*dz;

            if(d2 < mindist*mindist)
                setDistanceVolume(1.0f);
            else if(d2 > maxdist*maxdist)
                setDistanceVolume(0.0f);
            else
            {
                // Replacement method for AL_INVERSE_DISTANCE_CLAMPED.
                // The problem with this distance model is that the volume never goes down
                // to 0, no matter how far sound sources are away from the listener.
                // This could be fixed by using AL_LINEAR_DISTANCE_CLAMPED, but this model does not sound
                // natural (as the gain/volume/decibels is a logarithmic measure).
                // As a remedy, use a simplified quadratic 1D-bezier curve to model
                // a decay similar to AL_INVERSE_DISTANCE_CLAMPED, but that actually reaches 0.
                // (The formula is simplified, as the control points (1, 0, 0) cause some math to vanish.) -- FG
                const float t = ((sqrtf(d2) - mindist) / (maxdist - mindist)); // [0 .. 1]
                const float t1 = 1.0f - t;
                const float a = t1 * t1;
                const float w = 2.0f; // weight; the higher this is, the steeper is the initial falloff, and the slower the final decay before reaching 0
                const float gain = a / (a + (2.0f * w * t * t1) + (t * t));
                setDistanceVolume(gain);
            }
        }
    }

    if(vel)
        alSource3f(sid, AL_VELOCITY, vel->x, vel->y, vel->z);

    SANITY_CHECK_OPENAL_CALL();

    return FMOD_OK;
}

ALBRIDGE(Channel,set3DMinMaxDistance,(float mindistance, float maxdistance),(mindistance, maxdistance))
FMOD_RESULT OpenALChannel::set3DMinMaxDistance(float mindistance, float maxdistance)
{
    alSourcef(sid, AL_REFERENCE_DISTANCE, mindistance);
    alSourcef(sid, AL_MAX_DISTANCE, maxdistance);
    SANITY_CHECK_OPENAL_CALL();
    mindist = mindistance;
    maxdist = maxdistance;

    return FMOD_OK;
}

ALBRIDGE(Channel,setMode,(FMOD_MODE mode),(mode))
FMOD_RESULT OpenALChannel::setMode(FMOD_MODE mode)
{
    _mode = mode;

    if(mode & FMOD_3D_HEADRELATIVE)
        alSourcei(sid, AL_SOURCE_RELATIVE, AL_TRUE);
    else // FMOD_3D_WORLDRELATIVE is the default according to FMOD docs
        alSourcei(sid, AL_SOURCE_RELATIVE, AL_FALSE);

    SANITY_CHECK_OPENAL_CALL();

    return FMOD_OK;
}

ALBRIDGE(Channel,getMode,(FMOD_MODE *mode),(mode))
FMOD_RESULT OpenALChannel::getMode(FMOD_MODE *mode)
{
    *mode = _mode;
    return FMOD_OK;
}

void OpenALChannel::setDistanceVolume(float _volume)
{
    distvolume = _volume;
    applyVolume();
}

void OpenALChannel::applyVolume()
{
    alSourcef(sid, AL_GAIN, volume * groupvolume * distvolume);
    SANITY_CHECK_OPENAL_CALL();
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
    if(sound == _sound)
        return;

    if (sound)
        sound->release();

    sound = _sound;

    if (sound)
        sound->reference();
}



// FMOD::System implementation ...

class OpenALSystem
{
public:
    OpenALSystem();
    ~OpenALSystem();
    FMOD_RESULT init(int maxchannels, const FMOD_INITFLAGS flags, const void *extradriverdata);
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
    FMOD_RESULT set3DListenerAttributes(int listener, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel, const FMOD_VECTOR *forward, const FMOD_VECTOR *up);

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

static void *decode_to_pcm(VFILE *io, ALenum &format, ALsizei &size, ALuint &freq)
{
    ALubyte *retval = NULL;

    // Uncompress and feed to the AL.
    OggVorbis_File vf;
    memset(&vf, '\0', sizeof (vf));
    if (ov_open_callbacks(io, &vf, NULL, 0, local_OV_CALLBACKS_NOCLOSE) == 0)
    {
        int bitstream = 0;
        vorbis_info *info = ov_info(&vf, -1);
        size = 0;
        format = (info->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        freq = info->rate;

        if ((info->channels != 1) && (info->channels != 2))
        {
            ov_clear(&vf);
            return NULL;
        }

        char buf[1024 * 16];
        long rc = 0;
        size_t allocated = 64 * 1024;
        retval = (ALubyte *) malloc(allocated);
        while ( (rc = ov_read(&vf, buf, sizeof (buf), BBGE_BIGENDIAN, 2, 1, &bitstream)) != 0 )
        {
            if (rc > 0)
            {
                size += rc;
				if ((size_t) size >= allocated)
                {
                    allocated *= 2;
                    ALubyte *tmp = (ALubyte *) realloc(retval, allocated);
                    if (tmp == NULL)
                    {
                        free(retval);
                        retval = NULL;
                        break;
                    }
                    retval = tmp;
                }
                memcpy(retval + (size - rc), buf, rc);
            }
        }
        ov_clear(&vf);
        return retval;
    }
    return NULL;
}


ALBRIDGE(System,createSound,(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound),(name_or_data,mode,exinfo,sound))
FMOD_RESULT OpenALSystem::createSound(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound)
{
    assert(!exinfo);

    FMOD_RESULT retval = FMOD_ERR_INTERNAL;

    // !!! FIXME: if it's not Ogg, we don't have a decoder. I'm lazy.  :/
    char *fname = (char *) alloca(strlen(name_or_data) + 16);
    strcpy(fname, name_or_data);
    char *ptr = strrchr(fname, '.');
    if (ptr) *ptr = '\0';
    strcat(fname, ".ogg");

    // just in case...
    VFILE *io = vfopen(adjustFilenameCase(fname).c_str(), "rb");
    if (io == NULL)
        return FMOD_ERR_INTERNAL;
    size_t filesize = 0;
    if(vfsize(io, &filesize))
        return FMOD_ERR_INTERNAL;
    if(!filesize)
        return FMOD_ERR_INTERNAL;

    if (mode & FMOD_CREATESTREAM)
    {
        // Create streaming file handle decoder
        *sound = (Sound *) new OpenALSound(io, (((mode & FMOD_LOOP_OFF) == 0) && (mode & FMOD_LOOP_NORMAL)));
        retval = FMOD_OK;
    }
    else if(core->settings.prebufferSounds)
    {
        // Pre-decode the sound file and store the raw PCM buffer
        ALenum format = AL_NONE;
        ALsizei size = 0;
        ALuint freq = 0;
        void *data = decode_to_pcm(io, format, size, freq);
        vfclose(io);

        ALuint bid = 0;
        alGenBuffers(1, &bid);
        if (bid != 0)
        {
            // FIXME: This needs to stored seperately and fed to the AL on demand,
            // converting stereo to mono when it's known whether to do so or not.
            alBufferData(bid, format, data, size, freq);
            *sound = (Sound *) new OpenALSound(bid, (((mode & FMOD_LOOP_OFF) == 0) && (mode & FMOD_LOOP_NORMAL)));
            ((OpenALSound*)*sound)->setNumChannels(format == AL_FORMAT_STEREO16 ? 2 : 1);
            retval = FMOD_OK;
        }
        free(data);
    }
    else
    {
        // Create streaming memory decoder
        void *data = malloc(filesize);
        if (data == NULL)
        {
            debugLog("Out of memory for " + std::string(fname));
            vfclose(io);
            return FMOD_ERR_INTERNAL;
        }

        size_t nread = vfread(data, 1, filesize, io);
        vfclose(io);
        if (nread != filesize)
        {
            debugLog("Failed to read data from " + std::string(fname));
            free(data);
            return FMOD_ERR_INTERNAL;
        }

        *sound = (Sound *) new OpenALSound(data, filesize, (((mode & FMOD_LOOP_OFF) == 0) && (mode & FMOD_LOOP_NORMAL)));
        retval = FMOD_OK;
    }

    return retval;
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

ALBRIDGE(System,init,(int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata),(maxchannels,flags,extradriverdata))
FMOD_RESULT OpenALSystem::init(int maxchannels, const FMOD_INITFLAGS flags, const void *extradriverdata)
{
    ALCdevice *dev = alcOpenDevice(NULL);
    if (!dev)
        return FMOD_ERR_INTERNAL;

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

        alSourcei(sid, AL_SOURCE_RELATIVE, AL_FALSE);
        SANITY_CHECK_OPENAL_CALL();
        alSource3f(sid, AL_POSITION, 0.0f, 0.0f, 0.0f);  // no panning or spatialization in Aquaria.
        SANITY_CHECK_OPENAL_CALL();
        channels[i].setSourceName(sid);
        channels[i].setChannelGroup((ChannelGroup *) master_channel_group);
    }
    std::stringstream ss;
    ss << "Using " << num_channels << " sound channels.";
    debugLog(ss.str());

    // HACK: FMOD doesn't do this.
    // For completeness, we pass FMOD_3D_LINEARROLLOFF to createSound().
    // We do our own non-standard distance attenuation model, as the modes offered by OpenAL
    // are not sufficient. See OpenALChannel::set3DMinMaxDistance() for the gain control code. -- FG
    alDistanceModel(AL_NONE);
    SANITY_CHECK_OPENAL_CALL();
    s_listenerPos.x = s_listenerPos.y = s_listenerPos.z = 0.0f;


    OggDecoder::startDecoderThread();

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
    OggDecoder::stopDecoderThread();

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

ALBRIDGE(System, set3DListenerAttributes, (int listener, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel, const FMOD_VECTOR *forward, const FMOD_VECTOR *up),
	(listener, pos, vel, forward, up))
FMOD_RESULT OpenALSystem::set3DListenerAttributes(int listener, const FMOD_VECTOR *pos, const FMOD_VECTOR *vel, const FMOD_VECTOR *forward, const FMOD_VECTOR *up)
{
    // ignore listener parameter; there is only one listener in OpenAL.

    if(up || forward)
    {
        ALfloat orientation[6];
        alGetListenerfv(AL_ORIENTATION, &orientation[0]);

        if(forward)
        {
            orientation[0] = forward->x;
            orientation[1] = forward->y;
            orientation[2] = forward->z;
        }

        if(up)
        {
            orientation[3] = up->x;
            orientation[4] = up->y;
            orientation[5] = up->z;
        }

        alListenerfv(AL_ORIENTATION, &orientation[0]);
    }

    if(pos)
    {
        s_listenerPos = *pos;
        alListener3f(AL_POSITION, pos->x, pos->y, pos->z);
    }

    if(vel)
        alListener3f(AL_VELOCITY, vel->x, vel->y, vel->z);

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


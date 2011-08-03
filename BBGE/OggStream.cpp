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
#include "OggStream.h"
#include "SoundManager.h"
#include "Base.h"

#define BUFFER_SIZE (4096 * 4)

OggStream::OggStream()
{
	loop=false;
	source = 0;
	buffers[0] = 0;
	buffers[1] = 0;
}

void OggStream::open(std::string path)
{
    int result;
    
    oggFile = fopen(core->adjustFilenameCase(path.c_str()), "rb");

	if (!oggFile)
	{
		sound->error("Could not open Ogg file.");
		return;
	}

    result = ov_open(oggFile, &oggStream, NULL, 0);
	
	if (result < 0)
    {
        fclose(oggFile);
        
		sound->error(std::string("Could not open Ogg stream. ") + errorString(result));
		return;
    }

    vorbisInfo = ov_info(&oggStream, -1);
    vorbisComment = ov_comment(&oggStream, -1);

    if(vorbisInfo->channels == 1)
        format = AL_FORMAT_MONO16;
    else
        format = AL_FORMAT_STEREO16;
        
        
    alGenBuffers(2, buffers);
    check();
    alGenSources(1, &source);
    check();
    
    alSource3f(source, AL_POSITION,        0.0, 0.0, 0.0);
    alSource3f(source, AL_VELOCITY,        0.0, 0.0, 0.0);
    alSource3f(source, AL_DIRECTION,       0.0, 0.0, 0.0);
    alSourcef (source, AL_ROLLOFF_FACTOR,  0.0          );
    alSourcei (source, AL_SOURCE_RELATIVE, AL_TRUE      );
}

void OggStream::release()
{
	BBGE_PROF(OggStream::release);
	if (source)
	{
		alSourceStop(source);
		empty();
		alDeleteSources(1, &source);
		check();
	}
	if (buffers && (buffers[0] || buffers[1]))
	{
		alDeleteBuffers(1, buffers);
		check();
	}

    ov_clear(&oggStream);

	source = 0;
	buffers[0] = 0;
	buffers[1] = 0;
}

void OggStream::display()
{
	std::cout
        << "version         " << vorbisInfo->version         << "\n"
        << "channels        " << vorbisInfo->channels        << "\n"
        << "rate (hz)       " << vorbisInfo->rate            << "\n"
        << "bitrate upper   " << vorbisInfo->bitrate_upper   << "\n"
        << "bitrate nominal " << vorbisInfo->bitrate_nominal << "\n"
        << "bitrate lower   " << vorbisInfo->bitrate_lower   << "\n"
        << "bitrate window  " << vorbisInfo->bitrate_window  << "\n"
        << "\n"
        << "vendor " << vorbisComment->vendor << "\n";
        
    for(int i = 0; i < vorbisComment->comments; i++)
		std::cout << "   " << vorbisComment->user_comments[i] << "\n";
        
	std::cout << std::endl;
}

bool OggStream::play(bool l)
{	
	BBGE_PROF(OggStream::play);
    if(isPlaying())
        return true;
        
    if(!stream(buffers[0]))
        return false;
        
    if(!stream(buffers[1]))
        return false;

	loop=l;
    
    alSourceQueueBuffers(source, 2, buffers);
    alSourcePlay(source);
    
    return true;
}

bool OggStream::isPlaying()
{
    ALenum state;
    
	if (source)
	{
		alGetSourcei(source, AL_SOURCE_STATE, &state);
    
		return (state == AL_PLAYING);
	}

	return false;
}

void OggStream::setGain(float gain)
{
	if (!source) return;

	alSourcef(source, AL_GAIN, gain);
	ALenum error=alGetError();

	if(error!=AL_FALSE)
	{
		switch(error)
		{
			case(AL_INVALID_VALUE):
				sound->error("Invalid value for gain");
			break;
			default:			
				sound->error("Error trying to set gain!");
			break;
		}
	}
}

bool OggStream::update()
{
	if (!source) return false;

    int processed;
    bool active = true;

	if (!isPlaying())
	{
		alSourcePlay(source);
	}

    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while(processed--)
    {
        ALuint buffer;
        
        alSourceUnqueueBuffers(source, 1, &buffer);
        check();

        active = stream(buffer);

        alSourceQueueBuffers(source, 1, &buffer);
        check();
    }

	if (!active)
	{
		release();
	}

    return active;
}

bool OggStream::stream(ALuint buffer)
{
    char pcm[BUFFER_SIZE];
    int  size = 0;
    int  section;
    int  result;

    while(size < BUFFER_SIZE)
    {
        result = ov_read(&oggStream, pcm + size, BUFFER_SIZE - size, 0, 2, 1, &section);
    
        if(result > 0)
            size += result;
        else
		{	if(result < 0) {} else break;	}
    }
    
    if(size == 0)
	{
		if (loop)
		{
			std::cout << "looping\n";
			int r = ov_pcm_seek(&oggStream, 0);
			if (r != 0)
			{
				std::cout << "error\n";
			}

			while(size < BUFFER_SIZE)
			{
				result = ov_read(&oggStream, pcm + size, BUFFER_SIZE - size, 0, 2, 1, &section);
			
				if(result > 0)
					size += result;
				else
				{	if(result < 0) {} else break;	}
			}
		}
		else
		{
			return false;
		}
	}
        
    alBufferData(buffer, format, pcm, size, vorbisInfo->rate);
    //check();
    
    return true;
}

void OggStream::empty()
{
	if (!source) return;
    int queued;
    
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    
    while(queued--)
    {
        ALuint buffer;
    
        alSourceUnqueueBuffers(source, 1, &buffer);
        check();
    }
}

void OggStream::check()
{
	int error = alGetError();

	if(error != AL_NO_ERROR)
		sound->error(std::string("OpenAL error was raised."));
}

std::string OggStream::errorString(int code)
{
    switch(code)
    {
        case OV_EREAD:
            return std::string("Read from media.");
        case OV_ENOTVORBIS:
            return std::string("Not Vorbis data.");
        case OV_EVERSION:
            return std::string("Vorbis version mismatch.");
        case OV_EBADHEADER:
            return std::string("Invalid Vorbis header.");
        case OV_EFAULT:
            return std::string("Internal logic fault (bug or heap/stack corruption.");
        default:
            return std::string("Unknown Ogg error.");
    }
}


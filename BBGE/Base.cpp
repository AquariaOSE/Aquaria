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
#include "Base.h"
#include "Core.h"
#include <assert.h>
#include "OSFunctions.h"

#if SDL_VERSION_ATLEAST(2,0,0)
#include "SDL_filesystem.h"
#endif

#ifdef BBGE_BUILD_VFS
#  include "ttvfs.h"
#  ifndef VFS_IGNORE_CASE
#	error Must define VFS_IGNORE_CASE, see VFSDefines.h
#  endif
   ttvfs::Root vfs; // extern
#endif

#include "ttvfs_stdio.h"



int randAngle360()
{
	return rand()%360;
}

int randRange(int n1, int n2)
{
	int r = rand()%(n2-1);
	r += n1;
	return r;
}

std::string removeSpaces(const std::string &input)
{
	std::string result;
	for (size_t i = 0; i < input.size(); i++)
	{
		if (input[i] != ' ')
		{
			result.push_back(input[i]);
		}
	}
	return result;
}

unsigned hash(const std::string &string)
{
	unsigned hash = 5381;

	for (size_t i = 0; i < string.size(); i++)
		hash = ((hash << 5) + hash) + (unsigned char)string[i];

	return hash;
}



static unsigned char lowerToUpperTable[256];
static unsigned char upperToLowerTable[256];

void initCharTranslationTables(const CharTranslationTable *ptab)
{
	for (unsigned int i = 0; i < 256; ++i)
	{
		lowerToUpperTable[i] = i;
		upperToLowerTable[i] = i;
	}
	for (unsigned char i = 'a'; i <= 'z'; ++i)
	{
		lowerToUpperTable[i] = i - 'a' + 'A';
		upperToLowerTable[i - 'a' + 'A'] = i;
	}
	if(ptab)
		for (unsigned int i = 0; i < 256; ++i)
		{
			int to = (*ptab)[i];
			if(to > 0)
			{
				lowerToUpperTable[i] = to;
				upperToLowerTable[to] = i;
			}
		}
}

struct TransatableStaticInit
{
	TransatableStaticInit()
	{
		initCharTranslationTables(NULL);
	}
};
static TransatableStaticInit _transtable_static_init;

static unsigned char charIsUpper(unsigned char c)
{
	return c == lowerToUpperTable[c];
}

static unsigned char charToLower(unsigned char c)
{
	return upperToLowerTable[c];
}

static unsigned char charToUpper(unsigned char c)
{
	return lowerToUpperTable[c];
}

std::string splitCamelCase(const std::string &input)
{
	std::string result;
	int last = 0;
	for (size_t i = 0; i < input.size(); i++)
	{
		if (last == 1)
		{
			if (charIsUpper(input[i]))
			{
				result += ' ';
			}
		}

		result += input[i];

		if (charIsUpper(input[i]))
		{
			last = 2;
		}
		else
			last = 1;
	}
	return result;
}

std::string numToZeroString(int num, size_t zeroes)
{
	std::ostringstream num_os;
	num_os << num;

	std::ostringstream os;

	if (num_os.str().size() <= zeroes)
	{
		for (size_t j = 0; j < zeroes - num_os.str().size(); j++)
		{
			os << "0";
		}
		os << num;
	}
	return os.str();
}

void stringToUpper(std::string &s)
{
	for (size_t i = 0; i < s.size(); i++)
		s[i] = charToUpper(s[i]);
}

void stringToLower(std::string &s)
{
	for (size_t i = 0; i < s.size(); i++)
		s[i] = charToLower(s[i]);
}

void stringToLowerUserData(std::string &s)
{
	// don't lowercase the userdata path.
	const std::string userdata = core->getUserDataFolder();
	const size_t len = userdata.length();
	const bool match = (s.length() > len) &&
					   ((s[len] == '/') || (s[len] == '\\')) &&
					   !strncmp(userdata.c_str(), s.c_str(), len);
	if (!match)
		stringToLower(s);
	else
	{
		s = s.substr(len);
		stringToLower(s);
		s = userdata + s;
	}
}

#ifndef HAVE_STRCASECMP
int nocasecmp(const std::string &s1, const std::string &s2)
{
	const char *it1 = s1.c_str();
	const char *it2 = s2.c_str();

  //stop when either string's end has been reached
  while ( *it1 && *it2 )
  {
	if(charToUpper(*it1) != charToUpper(*it2)) //letters differ?
	 // return -1 to indicate smaller than, 1 otherwise
	  return (charToUpper(*it1)  < charToUpper(*it2)) ? -1 : 1;
	//proceed to the next character in each string
	++it1;
	++it2;
  }
  size_t size1=s1.size(), size2=s2.size();// cache lengths
   //return -1,0 or 1 according to strings' lengths
	if (size1==size2)
	  return 0;
	return (size1<size2) ? -1 : 1;
}
#endif  // #if !HAVE_STRCASECMP

bool exists(const std::string &f, bool makeFatal, bool skipVFS)
{
	bool e = false;

#ifdef BBGE_BUILD_VFS
	if (!skipVFS)
	{
		e = !!vfgetfile(f.c_str());
	}
	else
	{
		std::string tmp = adjustFilenameCase(f);
		e = ttvfs::FileExists(tmp.c_str());
	}
#else
	if (!e)
	{
		std::string tmp = adjustFilenameCase(f);
		FILE *file = fopen(tmp.c_str(), "rb");
		if (file)
		{
			e = true;
			fclose(file);
		}
	}
#endif

	if (makeFatal && !e)
	{
		exit_error("Could not open [" + f + "]");
	}

	return e;
}



std::string getPathInfoStr()
{
	std::ostringstream os;
	os << "Working dir (expecting game data there):\n" << getWorkingDir() << "\n";
	if(core)
	{
		os << "Preferences folder:\n" << core->getPreferencesFolder() << "\n";
		os << "User data folder:\n" << core->getUserDataFolder() << "\n";
		os << "Debug log path:\n" << core->getDebugLogPath() << "\n";
	}
#if SDL_VERSION_ATLEAST(2,0,1)
	char *base = SDL_GetBasePath();
	os << "SDL_GetBasePath():\n" << base << "\n";
	SDL_free(base);
#endif
	return os.str();
}

void exit_error(const std::string &message)
{
	std::string out = message + "\n\n++ Path info for debugging aid: ++\n" + getPathInfoStr();
	fprintf(stderr, "FATAL: %s\n", out.c_str());
	errorLog(out);
	exit(1);
}

bool chance(int perc)
{
	if (perc == 100) return true;
	if (perc == 0) return false;
	return ((rand()%100) <= perc);
}

void errorLog(const std::string &s)
{
	if (core)
	{
		core->_errorLog(s);
	}
	else
	{
		messageBox("Error!", s);
	}
}

void debugLog(const std::string &s)
{
	if (core)
		core->_debugLog(s);
	else
	{
		//MessageBox(0, s.c_str(), "DebugLog (Core Not Initalized)", MB_OK);
	}
}


// Read the given file into memory and return a pointer to the allocated
// buffer.  The buffer will be null-terminated, like a C string; you can
// also obtain the data length by passing a pointer to an unsigned long
// as the (optional) second parameter.  The buffer should be freed with
// delete[] when no longer needed.
char *readFile(const char *path, size_t *size_ret)
{
	VFILE *f = vfopen(path, "rb");
	if (!f)
		return NULL;

	size_t fileSize = 0;
	if(vfsize(f, &fileSize) < 0)
	{
		debugLog(std::string(path) + ": Failed to get file size");
		vfclose(f);
		return NULL;
	}

	char *buffer = new char[fileSize + 1];
	if (!buffer)
	{
		std::ostringstream os;
		os << path << ": Not enough memory for file ("
		   << (fileSize+1) << " bytes)";
		debugLog(os.str());
		vfclose(f);
		return NULL;
	}

	size_t bytesRead = vfread(buffer, 1, fileSize, f);
	if (bytesRead != fileSize)
	{
		std::ostringstream os;
		os << path << ": Failed to read file (only got "
		   << bytesRead << " of " << fileSize << " bytes)";
		debugLog(os.str());
		vfclose(f);
		return NULL;
	}
	vfclose(f);
	buffer[fileSize] = 0;

	if (size_ret)
		*size_ret = fileSize;
	return buffer;
}

std::string stripEndlineForUnix(const std::string &in)
{
	std::string out;
	for (size_t i = 0; i < in.size(); i++)
	{
		if (int(in[i]) != 13)
		{
			out+= in[i];
		}
	}
	return out;
}

bool isTouchingLine(Vector lineStart, Vector lineEnd, Vector point, int radius, Vector *closestP)
{
	Vector dir = lineEnd - lineStart;
	Vector diff = point - lineStart;
	Vector closest;
	if (!dir.isZero()) {
	float t = diff.dot2D(dir) / dir.dot2D(dir);
	if (t < 0.0f)
		t = 0.0f;
	if (t > 1.0f)
		t = 1.0f;
	closest = lineStart + t * dir;
	} else {
	closest = lineStart;
	}
	Vector d = point - closest;
	float distsqr = d.dot2D(d);
	if (closestP)
		(*closestP) = closest;
	return distsqr <= radius*radius;
}

Vector randVector(float mag)
{
	float angle = (rand() / (float)RAND_MAX) * 2.0f * PI;
	float x = sinf(angle), y = cosf(angle);
	return Vector(x*mag, y*mag);
}

float lerp(float v1, float v2, float dt, int lerpType)
{
	switch(lerpType)
	{
		case LERP_EASE:
		{
			// ease in and out
			return v1*(2*(dt*dt*dt)-3*sqr(dt)+1) + v2*(3*sqr(dt) - 2*(dt*dt*dt));
		}
		case LERP_EASEIN:
		{
			float t = 1 - dt;
			return (v2-v1)*(sinf(-t*PI_HALF)+1)+v1;
		}
		case LERP_EASEOUT:
		{
			return (v2-v1)*-sinf(-dt*PI_HALF)+v1;
		}
	}

	return (v2-v1)*dt+v1;
}

std::string underscoresToSpaces(const std::string &str)
{
	std::string s = str;
	for (size_t i = 0; i < s.size(); i++)
		if (s[i] == '_') s[i] = ' ';
	return s;
}

std::string spacesToUnderscores(const std::string &str)
{
	std::string s = str;
	for (size_t i = 0; i < s.size(); i++)
		if (s[i] == ' ') s[i] = '_';
	return s;
}




#include "DeflateCompressor.h"

char *readCompressedFile(const char *path, size_t *size_ret)
{
	size_t size = 0;
	char *buf = readFile(path, &size);
	if(!buf)
		return NULL;
	ZlibCompressor z; // allocates with new[] by default
	z.init(buf, size, ByteBuffer::TAKE_OVER);
	z.Compressed(true);
	z.Decompress();
	if(!z.Compressed())
	{
		if (size_ret)
			*size_ret = z.size();
		z.wpos(z.size());
		z << '\0'; // be sure the buffer is null-terminated
		buf = (char*)z.ptr();
		z._setPtr(NULL);
		return buf;
	}
	return NULL;
}

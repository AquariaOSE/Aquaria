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
#include "VFSDir.h"

#ifdef BBGE_BUILD_WINDOWS
	#include <shellapi.h>
#endif

#if defined(BBGE_BUILD_UNIX)
	#include <sys/types.h>
	#include <dirent.h>
#endif

#if defined(BBGE_BUILD_MACOSX)
	#include <Carbon/Carbon.h>
#endif

#include <assert.h>

Vector getDirVector(Direction dir)
{
	switch(dir)
	{
	case DIR_DOWN:
		return Vector(0, 1);
	break;
	case DIR_UP:
		return Vector(0, -1);
	break;
	case DIR_LEFT:
		return Vector(-1, 0);
	break;
	case DIR_RIGHT:
		return Vector(1, 0);
	break;
	}
	return Vector(0,0);
}

Direction getOppositeDir(Direction dir)
{
	switch(dir)
	{
	case DIR_DOWN:
		return DIR_UP;
	break;
	case DIR_UP:
		return DIR_DOWN;
	break;
	case DIR_LEFT:
		return DIR_RIGHT;
	break;
	case DIR_RIGHT:
		return DIR_LEFT;
	break;
	}

	return DIR_NONE;
}

Direction getNextDirClockwise(Direction dir)
{
	switch(dir)
	{
	case DIR_DOWN:
		return DIR_LEFT;
		break;
	case DIR_UP:
		return DIR_RIGHT;
		break;
	case DIR_LEFT:
		return DIR_UP;
		break;
	case DIR_RIGHT:
		return DIR_DOWN;
		break;
	}
	return DIR_NONE;
}

void sizePowerOf2Texture(int &v)
{
	int p = 8, use=0;
	do 
	{
		use = 1 << p;
		p++;
	}
	while(v > use);

	v = use;
}

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
	for (int i = 0; i < input.size(); i++)
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

    for (int i = 0; i < string.size(); i++)
        hash = ((hash << 5) + hash) + string[i];

    return hash;
}

/* hash * 33 + c */

std::string splitCamelCase(const std::string &input)
{
	std::string result;
	int last = 0;
	for (int i = 0; i < input.size(); i++)
	{
		if (last == 1)
		{
			if (input[i] >= 'A' && input[i] <= 'Z')
			{
				result += ' ';
			}
		}

		result += input[i];

		if (input[i] >= 'A' && input[i] <= 'Z')
		{
			last = 2;
		}
		else
			last = 1;
	}
	return result;
}

std::string numToZeroString(int num, int zeroes)
{
	std::ostringstream num_os;
	num_os << num;

	std::ostringstream os;

	if (num_os.str().size() <= zeroes)
	{
		for (int j = 0; j < zeroes - num_os.str().size(); j++)
		{
			os << "0";
		}
		os << num;
	}
	return os.str();
}

bool isVectorInRect(const Vector &vec, const Vector &coord1, const Vector &coord2)
{
	return (vec.x > coord1.x && vec.x < coord2.x && vec.y > coord1.y && vec.y < coord2.y);
}

void stringToUpper(std::string &s)
{
	for (int i = 0; i < s.size(); i++)
	{
		if (s[i] >= 'a' && s[i] <= 'z')
		{
			s[i] = s[i]-'a' + 'A';
		}
	}
}

void stringToLower(std::string &s)
{
	for (int i = 0; i < s.size(); i++)
	{
		if (s[i] >= 'A' && s[i] <= 'Z')
		{
			s[i] = s[i]-'A' + 'a';
		}
	}
}

void stringToLowerUserData(std::string &s)
{
	// don't lowercase the userdata path.
	const std::string userdata = core->getUserDataFolder();
	const size_t len = userdata.length();
	const bool match = (s.length() > len) &&
	                   ((s[len] == '/') || (s[len] == '\\')) &&
	                   (userdata.compare(0, len, s, 0, len) == 0);
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
	std::string::const_iterator it1=s1.begin();
	std::string::const_iterator it2=s2.begin();

  //stop when either string's end has been reached
  while ( (it1!=s1.end()) && (it2!=s2.end()) )
  {
    if(::toupper(*it1) != ::toupper(*it2)) //letters differ?
     // return -1 to indicate smaller than, 1 otherwise
      return (::toupper(*it1)  < ::toupper(*it2)) ? -1 : 1;
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

std::string upperCase(const std::string &s1)
{
	std::string ret;
	std::string::const_iterator it1=s1.begin();
	while (it1 != s1.end())
	{
		ret += ::toupper(*it1);
		++it1;
	}
	return ret;
}

bool exists(const std::string &f, bool makeFatal /* = false */, bool skipVFS /* = false */)
{
    if (f.empty())
        return false;

    if(!skipVFS)
    {
        if(core->vfs.GetFile(core->adjustFilenameCase(f).c_str()))
            return true;
    }

	FILE *file = fopen(core->adjustFilenameCase(f).c_str(), "rb");
	if (!file)
	{
		if (makeFatal)
		{
			errorLog(std::string("Could not open [" + f + "]"));
			exit(0);
		}
		return false;
	}
	fclose(file);
	return true;
}

void drawCircle(float radius, int stepSize)
{
#ifdef BBGE_BUILD_OPENGL
	glDisable(GL_CULL_FACE);

	glBegin(GL_POLYGON);
	{
		for(int i=0;i < 360; i+=stepSize) {
			const float degInRad = i*PI/180.0f;
			glVertex3f(cosf(degInRad)*radius, sinf(degInRad)*radius,0.0);
		}
	}
	glEnd();

	glEnable(GL_CULL_FACE);
#endif
}

void fatalError(const std::string &message)
{
	msg(message);
	exit(0);
}

std::string parseCommand(const std::string &line, const std::string &command)
{
	int stringPos = line.find(command);
	if (stringPos != std::string::npos)
	{
		return line.substr((command.length()), line.length());
	}
	return "";
}

void glColor3_256(int r, int g, int b)
{
#ifdef BBGE_BUILD_OPENGL
	glColor4f(float(r)/256.0f, float(g)/256.0f, float(b)/256.0f, 1.0f);
#endif
}

bool chance(int perc)
{
	if (perc == 100) return true;
	if (perc == 0) return false;
	return ((rand()%100) <= perc);
}

bool chancef(float p)
{
	if (p >= 1) return true;
	if (p <= 0) return false;
	return ((rand()%100) <= p*100);
}

/*
PHYSFS_file *openRead(const std::string &f)
{

	PHYSFS_file *file = PHYSFS_openRead(f.c_str());
	if (!file)
	{
		errorLog ("Could not open [" + f + "]");
		exit(0);
	}

	return file;
}


void pfread(void *buffer, PHYSFS_uint32 size, PHYSFS_uint32 objs, PHYSFS_file *handle)
{
	PHYSFS_read(handle, buffer, size, objs);
}

void pfseek(PHYSFS_file *handle,PHYSFS_uint64 byte,int origin)
{
	if (origin == SEEK_CUR)
	{
		byte += PHYSFS_tell(handle);
	}
	PHYSFS_seek(handle,byte);
}

void pfclose(PHYSFS_file *handle)
{
	PHYSFS_close(handle);
}

std::string pLoadStream(const std::string &filename)
{
	PHYSFS_file *f = openRead(filename.c_str());
	int len = PHYSFS_fileLength(f);
	std::string s;
	for (int i = 0; i < len; i++)
	{
		char p;
		PHYSFS_read(f, &p, sizeof(char), 1);
		s += p;
	}
	//std::istringstream is(s);

	PHYSFS_close(f);
	return s;

}


void pSaveStream(const std::string &filename, std::ostringstream &os)
{
	PHYSFS_file *f = PHYSFS_openWrite(filename.c_str());
	//int size = os.str().size();
	//PHYSFS_write(f, (void*)size, sizeof(int), 1);
	PHYSFS_write(f, (void*)os.str().c_str(), sizeof(char)*os.str().size(), 1);
	PHYSFS_close(f);
}
*/

void errorLog(const std::string &s)
{
	if (core)
	{
		core->errorLog(s);
	}
	else
	{
		//msg("Core Not Initialized");
		//MessageBox(0, s.c_str(), "ErrorLog (Core Not Initalized)", MB_OK);
	}
}

void debugLog(const std::string &s)
{
	if (core)
		core->debugLog(s);
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
char *readFile(std::string path, unsigned long *size_ret)
{
    ttvfs::VFSFile *vf = core->vfs.GetFile(path.c_str());
    if(!vf)
        return NULL;
    vf->getBuf(); // force size calc early
    // we can never know how the memory was allocated;
    // because the buffer is expected to be deleted with delete[],
    // it has to be explicitly copied to memory allocated with new[].
    unsigned long s = vf->size();
    char *buf = new char[s + 1];
    memcpy(buf, vf->getBuf(), s + 1);
    core->addVFSFileForDrop(vf);
    if(size_ret)
        *size_ret = s;
    return buf;
}

void forEachFile(std::string path, std::string type, void callback(const std::string &filename, intptr_t param), intptr_t param)
{
	if (path.empty()) return;

    stringToLowerUserData(path);
    stringToLower(type);
	//HACK: MAC:
	debugLog("forEachFile - path: " + path + " type: " + type);

    ttvfs::VFSDir *vd = core->vfs.GetDir(path.c_str(), false);
    if(!vd)
    {
        debugLog("Path '" + path + "' does not exist");
        return;
    }

    for(ttvfs::ConstFileIter it = vd->fileIter(); it != vd->fileIterEnd(); ++it)
    {
        const ttvfs::VFSFile *f = it->second;
        const char *e = strrchr(f->name(), '.');
        if (e)
        {
            std::string exs(e);
            stringToLower(exs);
            if(exs != type)
                continue;
        }
        else if(type.size())
            continue;

        callback(path + f->name(), param);
    }
}

std::string msg(const std::string &message)
{
	core->msg(message);
	return message;
}

void msgVector(const std::string &name, const Vector &vec)
{
	std::ostringstream os;
	os << name << ": (" << vec.x <<", " << vec.y << ", " << vec.z << ")";
	msg (os.str());
}

Vector getNearestPointOnLine(Vector a, Vector b, Vector c)
{
	Vector nearest;
	/// Local Variables ///////////////////////////////////////////////////////////
	long dot_ta,dot_tb;
	///////////////////////////////////////////////////////////////////////////////
	// SEE IF a IS THE NEAREST POINT - ANGLE IS OBTUSE
	dot_ta = (c.x - a.x)*(b.x - a.x) + (c.y - a.y)*(b.y - a.y);
	if (dot_ta <= 0) // IT IS OFF THE a VERTEX
	{
		nearest.x = a.x;
		nearest.y = a.y;
		return nearest;
	}
	dot_tb = (c.x - b.x)*(a.x - b.x) + (c.y - b.y)*(a.y - b.y);
	// SEE IF b IS THE NEAREST POINT - ANGLE IS OBTUSE
	if (dot_tb <= 0)
	{
		nearest.x = b.x;
		nearest.y = b.y;
		return nearest;
	}
	// FIND THE REAL NEAREST POINT ON THE LINE SEGMENT - BASED ON RATIO
	nearest.x = a.x + ((b.x - a.x) * dot_ta)/(dot_ta + dot_tb);
	nearest.y = a.y + ((b.y - a.y) * dot_ta)/(dot_ta + dot_tb);
	return nearest;
}

/*
bool isTouchingLine(Vector lineStart, Vector lineEnd, Vector point, int radius)
{
	Vector p = getNearestPointOnLine(lineStart, lineEnd, point);
	Vector diff = p - point;
	std::ostringstream os;
	os << "s(" << lineStart.x << ", " << lineStart.y << ") e(";
	os << lineEnd.x << ", " << lineEnd.y << ") - p(" << point.x << ", " << point.y << ")";
	debugLog(os.str());
	return (diff.getSquaredLength2D() < sqr(radius));
}
*/

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


GLuint generateEmptyTexture(int quality)											// Create An Empty Texture
{
	GLuint txtnumber=0;											// Texture ID
	unsigned int* data;											// Stored Data

	// Create Storage Space For Texture Data (128x128x4)
	int size = (quality * quality)* 4;
	data = (unsigned int*)new GLuint[(size * sizeof(unsigned int))];

	// HACK: changed code here

	for (int i = 0; i < size; i++)
	{
		data[i] = 0;
	}


	/*
#ifdef BBGE_BUILD_WINDOWS
	ZeroMemory(data,((quality * quality)* 4 * sizeof(unsigned int)));	// Clear Storage Memory
#else
	for (int i = 0; i < size; i++)
	{
		data[i] = 0;
	}
#endif
	*/

#ifdef BBGE_BUILD_OPENGL
	glGenTextures(1, &txtnumber);								// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, txtnumber);					// Bind The Texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, quality, quality, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);						// Build Texture Using Information In data
	
	delete [] data;												// Release data
#endif

	return txtnumber;											// Return The Texture ID
}

Vector randVector(int mag)
{
	// FIXME: Is this really what you wanted?  I'd suggest:
	//     float angle = (rand() / (float)RAND_MAX) * PI;
        // --achurch
	float angle = (rand()&314);
	float x = sinf(angle), y = cosf(angle);
	return Vector(x*mag, y*mag);
}

float lerp(const float &v1, const float &v2, float dt, int lerpType)
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


#include <zlib.h>
#include <assert.h>

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int packFile(const std::string &sourcef, const std::string &destf, int level)
{
	FILE *source	= fopen(core->adjustFilenameCase(sourcef).c_str(), "rb");
	FILE *dest		= fopen(core->adjustFilenameCase(destf).c_str(), "wb");

	if (!source || !dest)
		return 0;

    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);

	fclose(source);
	fclose(dest);

    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int unpackFile(const std::string &sourcef, const std::string &destf)
{
	FILE *source	= fopen(core->adjustFilenameCase(sourcef).c_str(), "rb");
	FILE *dest		= fopen(core->adjustFilenameCase(destf).c_str(), "wb");

	if (!source || !dest)
		return 0;

    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

	fclose(source);
	fclose(dest);

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int encode[8] = {16, 32, 8, 4, 2, 1, 3, 5 };

void crunchFile(const std::string &file, const std::string &out, bool deleteOriginal)
{
	FILE *f = fopen(core->adjustFilenameCase(file).c_str(), "rb"), *o = fopen(core->adjustFilenameCase(out).c_str(), "wb");

	if (f && o)
	{
		char buf=0;
		int rot = 0, add = 0;

		while (true)
		{
			if (fread(&buf, sizeof(char), 1, f) != 1)
				break;

			buf += encode[rot] + add;

			if (fwrite(&buf, sizeof(char), 1, o) != 1)
			{
				errorLog("Failed to write to " + out);
				break;
			}

			rot++;
			if (rot>=8)
			{ rot=0; add++; }
		}

		fclose(f);
		f=0;

		fclose(o);
		o=0;
	}

	if (f) fclose(f);
	if (o) fclose(o);

	if (deleteOriginal)
		remove(file.c_str());
}

void uncrunchFile(const std::string &file, const std::string &out)
{
	FILE *f = fopen(core->adjustFilenameCase(file).c_str(), "rb"), *o = fopen(core->adjustFilenameCase(out).c_str(), "wb");

	if (f && o)
	{
		char buf=0;
		int rot=0, add=0;
		while (true)
		{
			if (fread(&buf, sizeof(char), 1, f) != 1)
				break;

			buf -= encode[rot] + add;

			if (fwrite(&buf, sizeof(char), 1, o) != 1)
			{
				errorLog("Failed to write to " + out);
				break;
			}

			rot++;
			if (rot>=8)
			{ rot=0; add++; }
		}

		fclose(f);
		f=0;

		fclose(o);
		o=0;
	}

	if (f) fclose(f);
	if (o) fclose(o);
}

void openURL(const std::string &url)
{
#ifdef BBGE_BUILD_WINDOWS
	ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
#if defined(BBGE_BUILD_MACOSX)
	CFStringRef str = CFStringCreateWithCString (0, url.c_str(), 0);
	CFURLRef ref = CFURLCreateWithString(kCFAllocatorDefault, str, NULL);
	LSOpenCFURLRef(ref, 0);
	CFRelease(ref);
	CFRelease(str);
#elif BBGE_BUILD_UNIX
	std::string cmd("PATH=$PATH:. xdg-open '");
	cmd += url;
	cmd += "'";
	if (system(cmd.c_str()) != 0)
		debugLog("system(xdg_open '" + url + "') failed");
#endif
}

void clamp256Col(int *r)
{
	if (*r > 255)	*r = 255;
	if (*r < 0)		*r = 0;
}

Vector colorRGB(int r, int g, int b)
{
	Vector c;

	clamp256Col(&r);
	clamp256Col(&g);
	clamp256Col(&b);

	c.x = float(r)/255.0f;
	c.y = float(g)/255.0f;
	c.z = float(b)/255.0f;

	return c;
}


std::string underscoresToSpaces(const std::string &str)
{
	std::string s = str;
	for (int i = 0; i < s.size(); i++)
		if (s[i] == '_') s[i] = ' ';
	return s;
}

std::string spacesToUnderscores(const std::string &str)
{
	std::string s = str;
	for (int i = 0; i < s.size(); i++)
		if (s[i] == ' ') s[i] = '_';
	return s;
}

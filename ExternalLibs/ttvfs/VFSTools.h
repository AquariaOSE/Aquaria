// VFSTools.h - useful functions and misc stuff
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFS_TOOLS_H
#define VFS_TOOLS_H

#include <deque>
#include <string>

#include "VFSDefines.h"

VFS_NAMESPACE_START

typedef std::deque<std::string> StringList;

std::string stringToUpper(const std::string& s);
std::string stringToLower(const std::string& s);
void makeUppercase(std::string& s);
void makeLowercase(std::string& s);
void GetFileList(const char *, StringList& files);
void GetDirList(const char *, StringList& dirs, bool recursive = false);
bool FileExists(const char *);
bool IsDirectory(const char *);
bool CreateDir(const char*);
bool CreateDirRec(const char*);
vfspos GetFileSize(const char*);
std::string FixSlashes(const std::string& s);
std::string FixPath(const std::string& s);
const char *PathToFileName(const char *str);
void MakeSlashTerminated(std::string& s);
std::string StripFileExtension(const std::string& s);
std::string StripLastPath(const std::string& s);
void GetFileListRecursive(std::string dir, StringList& files, bool withQueriedDir = false);
bool WildcardMatch(const char *str, const char *pattern);
size_t strnNLcpy(char *dst, const char *src, unsigned int n = -1);
char *fastcat(char *s, const char *add);

template <class T> void StrSplit(const std::string &src, const std::string &sep, T& container, bool keepEmpty = false)
{
    std::string s;
    for (std::string::const_iterator i = src.begin(); i != src.end(); i++)
    {
        if (sep.find(*i) != std::string::npos)
        {
            if (keepEmpty || s.length())
                container.push_back(s);
            s = "";
        }
        else
        {
            s += *i;
        }
    }
    if (keepEmpty || s.length())
        container.push_back(s);
}

inline static size_t stringhash(const char *s)
{
    size_t h = 0;
    for( ; *s; ++s)
    {
        h += *s;
        h += ( h << 10 );
        h ^= ( h >> 6 );
    }

    h += ( h << 3 );
    h ^= ( h >> 11 );
    h += ( h << 15 );

    return h;
}

inline static size_t stringhash_nocase(const char *s)
{
    size_t h = 0;
    for( ; *s; ++s)
    {
        h += tolower(*s);
        h += ( h << 10 );
        h ^= ( h >> 6 );
    }

    h += ( h << 3 );
    h ^= ( h >> 11 );
    h += ( h << 15 );

    return h;
}

#ifdef VFS_IGNORE_CASE
#  define STRINGHASH(s) stringhash_nocase(s)
#else
#  define STRINGHASH(s) stringhash(s)
#endif

VFS_NAMESPACE_END

#endif

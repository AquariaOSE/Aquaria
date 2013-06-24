// VFSTools.cpp - useful functions and misc stuff
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSFileFuncs.h"

#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <stack>
#include <cstdio>

#include "VFSTools.h"


#if _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#  ifdef __HAIKU__
#    include <dirent.h>
#  else
#    include <sys/dir.h>
#  endif
#  include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

VFS_NAMESPACE_START

void stringToLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), tolower);
}

void stringToUpper(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), toupper);
}

#if !_WIN32
#ifdef DT_DIR
static bool _IsFile(const char *path, dirent *dp)
{
    switch(dp->d_type)
    {
        case DT_DIR:
            return false;
        case DT_LNK:
        {
            std::string fullname = path;
            fullname += '/';
            fullname += dp->d_name;
            struct stat statbuf;
            if(stat(fullname.c_str(), &statbuf))
                return false; // error
            return !S_ISDIR(statbuf.st_mode);
        }
        // TODO: for now, we consider other file types as regular files
        default:
            ;
    }
    return true;
}

static bool _IsDir(const char *path, dirent *dp)
{
    switch(dp->d_type)
    {
        case DT_DIR:
            return true;
        case DT_LNK:
        {
            std::string fullname = path;
            fullname += '/';
            fullname += dp->d_name;
            struct stat statbuf;
            if(stat(fullname.c_str(), &statbuf))
                return false; // error
            return S_ISDIR(statbuf.st_mode);
        }
        default:
            ;
    }
    return false;
}

#else // No DT_DIR, assume plain POSIX

static bool _IsDir(const char *path, dirent *dp)
{
    const int len1 = strlen(path);
    const int len2 = strlen(dp->d_name);

    char *pathname = (char*)alloca(len1 + 1 + len2 + 1 + 13);
    strcpy (pathname, path);

	/* Avoid UNC-path "//name" on Cygwin.  */
    if (len1 > 0 && pathname[len1 - 1] != '/')
        strcat (pathname, "/");

    strcat (pathname, dp->d_name);

    struct stat st;
    if (stat (pathname, &st))
        return false;
    return S_ISDIR (st.st_mode);
}

static bool _IsFile(const char *path, dirent *dp)
{
	return !_IsDir(path, dp);
}
#endif // DT_DIR

#endif // !_WIN32

// returns list of *plain* file names in given directory,
// without paths, and without anything else
void GetFileList(const char *path, StringList& files)
{
#if !_WIN32
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(path);
    if(dirp)
    {
        while((dp=readdir(dirp)) != NULL)
        {
            if (_IsFile(path, dp)) // only add if it is not a directory
            {
                std::string s(dp->d_name);
                files.push_back(s);
            }
        }
        closedir(dirp);
    }

# else

    WIN32_FIND_DATA fil;
    std::string search(path);
    MakeSlashTerminated(search);
    search += "*";
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil != INVALID_HANDLE_VALUE)
    {
        do
        {
            if(!(fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string s(fil.cFileName);
                files.push_back(s);
            }
        }
        while(FindNextFile(hFil, &fil));

        FindClose(hFil);
    }

# endif
}

// returns a list of directory names in the given directory, *without* the source dir.
// if getting the dir list recursively, all paths are added, except *again* the top source dir beeing queried.
void GetDirList(const char *path, StringList &dirs, int depth /* = 0 */)
{
#if !_WIN32
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(path);
    if(dirp)
    {
        std::string pathstr(path);
        MakeSlashTerminated(pathstr);
        while((dp = readdir(dirp))) // assignment is intentional
        {
            if (_IsDir(path, dp)) // only add if it is a directory
            {
                if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
                {
                    dirs.push_back(dp->d_name);
                    if (depth) // needing a better way to do that
                    {
                        std::string d = dp->d_name;
                        std::string subdir = pathstr + d;
                        MakeSlashTerminated(d);
                        StringList newdirs;
                        GetDirList(subdir.c_str(), newdirs, depth - 1);
                        for(std::deque<std::string>::iterator it = newdirs.begin(); it != newdirs.end(); ++it)
                            dirs.push_back(d + *it);
                    }
                }
            }
        }
        closedir(dirp);
    }

#else
    std::string pathstr(path);
    MakeSlashTerminated(pathstr);
    WIN32_FIND_DATA fil;
    HANDLE hFil = FindFirstFile((pathstr + '*').c_str(),&fil);
    if(hFil != INVALID_HANDLE_VALUE)
    {
        do
        {
            if( fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                if (!strcmp(fil.cFileName, ".") || !strcmp(fil.cFileName, ".."))
                    continue;

                dirs.push_back(fil.cFileName);

                if (depth) // need a better way to do that
                {
                    std::string d = fil.cFileName;
                    std::string subdir = pathstr + d;
                    MakeSlashTerminated(d);
                    StringList newdirs;
                    GetDirList(subdir.c_str(), newdirs, depth - 1);
                    for(std::deque<std::string>::iterator it = newdirs.begin(); it != newdirs.end(); ++it)
                        dirs.push_back(d + *it);
                }
            }
        }
        while(FindNextFile(hFil, &fil));

        FindClose(hFil);
    }

#endif
}

bool FileExists(const char *fn)
{
#ifdef _WIN32
    void *fp = real_fopen(fn, "rb");
    if(fp)
    {
        real_fclose(fp);
        return true;
    }
    return false;
#else
    return access(fn, F_OK) == 0;
#endif
}

// must return true if creating the directory was successful, or already exists
bool CreateDir(const char *dir)
{
    if(IsDirectory(dir)) // do not try to create if it already exists
        return true;
    bool result;
# if _WIN32
    result = !!::CreateDirectory(dir, NULL);
# else
    result = !mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    return result;
}

bool CreateDirRec(const char *dir)
{
    if(IsDirectory(dir))
        return true;
    bool result = true;
    StringList li;
    StrSplit(dir, "/\\", li, false);
    std::string d;
    d.reserve(strlen(dir) + 1);
    if(*dir == '/')
        d += '/';
    bool last = false;
    for(StringList::iterator it = li.begin(); it != li.end(); ++it)
    {
        d += *it;
        last = CreateDir(d.c_str());
        result = last && result;
        d += '/';
    }
    return result || last;
}

vfspos GetFileSize(const char* fn)
{
#ifdef VFS_LARGEFILE_SUPPORT
# ifdef _MSC_VER
    struct _stat64 st;
    if(_stat64(fn, &st))
        return 0;
    return st.st_size;
# else // _MSC_VER
    struct stat64 st;
    if(stat64(fn, &st))
        return 0;
    return st.st_size;
# endif
#else // VFS_LARGEFILE_SUPPORT
    struct stat st;
    if(stat(fn, &st))
        return 0;
    return st.st_size;
#endif
}

std::string FixSlashes(const std::string& s)
{
    std::string r;
    r.reserve(s.length() + 1);
    char last = 0, cur;
    for(size_t i = 0; i < s.length(); ++i)
    {
        cur = s[i];
        if(cur == '\\')
            cur = '/';
        if(last == '/' && cur == '/')
            continue;
        r += cur;
        last = cur;
    }
    return r;
}

std::string FixPath(const std::string& s)
{
    if(s.empty())
        return s;
    const char *p = s.c_str();
    while(p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
        p += 2;
    if(!*p)
        return "";
    char end = s[s.length() - 1];
    if(end == '/' || end == '\\')
    {
        std::string r(p);
        r.erase(r.length() - 1); // strip trailing '/'
        return FixSlashes(r);
    }
    return FixSlashes(p);
}

bool IsDirectory(const char *s)
{
#if _WIN32
    DWORD dwFileAttr = GetFileAttributes(s);
    if(dwFileAttr == INVALID_FILE_ATTRIBUTES)
        return false;
    return !!(dwFileAttr & FILE_ATTRIBUTE_DIRECTORY);
#else
    if ( access( s, 0 ) == 0 )
    {
        struct stat status;
        stat( s, &status );
        return status.st_mode & S_IFDIR; // FIXME: what about symlinks here?
    }
    return false;
#endif
}

void MakeSlashTerminated(std::string& s)
{
    if(s.length() && s[s.length() - 1] != '/')
        s += '/';
}

// extracts the file name from a given path
const char *PathToFileName(const char *str)
{
    const char *p = strrchr(str, '/');
    return p ? p+1 : str;
}

std::string StripFileExtension(const std::string& s)
{
    size_t pos = s.find_last_of('.');
    size_t pos2 = s.find_last_of('/');
    if(pos != std::string::npos && (pos2 < pos || pos2 == std::string::npos))
        return s.substr(0, pos);

    return s;
}

std::string StripLastPath(const std::string& s)
{
    if(s.empty())
        return "";

    if(s[s.length() - 1] == '/')
        return StripLastPath(s.substr(0, s.length() - 1));

    size_t pos = s.find_last_of('/');
    if(pos == std::string::npos)
        return ""; // nothing remains

    return s.substr(0, pos);
}

// from http://board.byuu.org/viewtopic.php?f=10&t=1089&start=15
bool WildcardMatch(const char *str, const char *pattern)
{
    const char *cp = 0, *mp = 0;
    while(*str && *pattern != '*')
    {
        if(*pattern != *str && *pattern != '?')
            return false;
        ++pattern;
        ++str;
    }

    while(*str)
    {
        if(*pattern == '*')
        {
            if(!*++pattern)
                return true;
            mp = pattern;
            cp = str + 1;
        }
        else if(*pattern == *str || *pattern == '?')
        {
            ++pattern;
            ++str;
        }
        else
        {
            pattern = mp;
            str = cp++;
        }
    }

    while(*pattern == '*')
        ++pattern;

    return !*pattern;
}

// copy strings, mangling newlines to system standard
// windows has 13+10
// *nix has 10
// exotic systems may have 10+13
size_t strnNLcpy(char *dst, const char *src, unsigned int n /* = -1 */)
{
    char *olddst = dst;
    bool had10 = false, had13 = false;

    --n; // reserve 1 for \0 at end

    while(*src && n)
    {
        if((had13 && *src == 10) || (had10 && *src == 13))
        {
            ++src; // last was already mangled
            had13 = had10 = false; // processed one CRLF pair
            continue;
        }
        had10 = *src == 10;
        had13 = *src == 13;

        if(had10 || had13)
        {
            *dst++ = '\n';
            ++src;
        }
        else
            *dst++ = *src++;

        --n;
    }

    *dst++ = 0;

    return dst - olddst;
}

// Directly appends 'add' to 's', ensuring that 's' is null-terminated.
// Returns the next write position for fastcat (the null byte at the end).
char *fastcat(char *s, const char *add)
{
    size_t len = strlen(add);
    memcpy(s, add, len);
    s += len;
    *(s + 1) = 0;
    return s;
}



VFS_NAMESPACE_END

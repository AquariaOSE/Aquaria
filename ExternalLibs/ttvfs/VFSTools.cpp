// VFSTools.cpp - useful functions and misc stuff
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"
#include "VFSTools.h"

#include <algorithm>
#include <ctype.h>

#if _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <io.h>
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
bool GetFileList(const char *path, StringList& files)
{
#if !_WIN32
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(path);
    if(!dirp)
        return false;

    while((dp=readdir(dirp)) != NULL)
    {
        if (_IsFile(path, dp)) // only add if it is not a directory
        {
            std::string s(dp->d_name);
            files.push_back(s);
        }
    }
    closedir(dirp);
    return true;

#else

    WIN32_FIND_DATA fil;
    std::string search(path);
    MakeSlashTerminated(search);
    search += "*";
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil == INVALID_HANDLE_VALUE)
        return false;

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
    return true;

#endif
}

// returns a list of directory names in the given directory, *without* the source dir.
// if getting the dir list recursively, all paths are added, except *again* the top source dir beeing queried.
bool GetDirList(const char *path, StringList &dirs, int depth /* = 0 */)
{
#if !_WIN32
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(path);
    if(!dirp)
        return false;

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
    return true;

#else

    std::string pathstr(path);
    MakeSlashTerminated(pathstr);
    WIN32_FIND_DATA fil;
    HANDLE hFil = FindFirstFile((pathstr + '*').c_str(),&fil);
    if(hFil == INVALID_HANDLE_VALUE)
        return false;

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
    return true;

#endif
}

bool FileExists(const char *fn)
{
#if _WIN32
    return _access(fn, 0) == 0;
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

bool GetFileSize(const char* fn, vfspos& size)
{
    vfspos sz = 0;
#if defined(VFS_LARGEFILE_SUPPORT) && defined(_MSC_VER)
    struct _stat64 st;
    if(_stat64(fn, &st))
        return false;
    sz = st.st_size;
#else
    struct stat st;
    if(stat(fn, &st))
        return false;
    sz = st.st_size;
#endif
    size = sz;
    return true;
}

void FixSlashes(std::string& s)
{
    char last = 0, cur;
    size_t wpos = 0;
    for(size_t i = 0; i < s.length(); ++i)
    {
        cur = s[i];
        if(cur == '\\')
            cur = '/';
        if(last == '/' && cur == '/')
            continue;
        s[wpos++] = cur;
        last = cur;
    }
    s.resize(wpos);
}

void FixPath(std::string& s)
{
    if(s.empty())
        return;
    const char *p = s.c_str();
    while(p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
        p += 2;
    if(!*p)
    {
        s.clear();
        return;
    }
    if(s.c_str() != p)
        s = p;
    size_t len = s.length();
    while(len > 1) // remove all trailing slashes unless the first char is a slash -- leave it there for absolute unix paths
    {
        char end = s[len - 1];
        if(end == '/' || end == '\\') // strip trailing '/'
            --len;
        else
            break;
    }
    s.resize(len);
    FixSlashes(s);
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

// extracts the file name (part after the last /) from a given path
// returns the string "/" as-is.
const char *GetBaseNameFromPath(const char *str)
{
    if(str[0] == '/' && !str[1])
        return str;
    const char *p = strrchr(str, '/');
    return p ? p+1 : str;
}

void StripFileExtension(std::string& s)
{
    size_t pos = s.find_last_of('.');
    size_t pos2 = s.find_last_of('/');
    if(pos != std::string::npos && (pos2 == std::string::npos || pos2 < pos))
        s.resize(pos+1);
}

void StripLastPath(std::string& s)
{
    size_t len = s.length();
    while(len)
    {
        char end = s[len - 1];
        if(end == '/' || end == '\\') // strip trailing '/'
            --len;
        else
            break;
    }
    s.resize(len);

    if(s.empty())
        return;

    size_t pos = s.find_last_of('/');
    if(pos == std::string::npos)
    {
        s.clear();
        return; // nothing remains
    }

    s.resize(pos+1);
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



VFS_NAMESPACE_END

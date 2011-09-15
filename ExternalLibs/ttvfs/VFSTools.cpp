// VFSTools.cpp - useful functions and misc stuff
// For conditions of distribution and use, see copyright notice in VFS.h

#include "VFSInternal.h"

#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <stack>
#include "VFSTools.h"


#if _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <sys/dir.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <unistd.h>
#endif

VFS_NAMESPACE_START

std::string stringToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), tolower);
    return s;
}

std::string stringToUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), toupper);
    return s;
}

void makeLowercase(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), tolower);
}

void makeUppercase(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), toupper);
}

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
            if (dp->d_type != DT_DIR) // only add if it is not a directory
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
void GetDirList(const char *path, StringList &dirs, bool recursive /* = false */)
{
#if !_WIN32
    DIR * dirp;
    struct dirent * dp;
    dirp = opendir(path);
    if(dirp)
    {
        while((dp = readdir(dirp))) // assignment is intentional
        {
            if (dp->d_type == DT_DIR) // only add if it is a directory
            {
                if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
                {
                    dirs.push_back(dp->d_name);
                    if (recursive) // needing a better way to do that
                    {
                        std::deque<std::string> newdirs;
                        GetDirList(dp->d_name, newdirs, true);
                        std::string d(dp->d_name);
                        for(std::deque<std::string>::iterator it = newdirs.begin(); it != newdirs.end(); ++it)
                            dirs.push_back(d + *it);
                    }
                }
            }
        }
        closedir(dirp);
    }

#else

    std::string search(path);
    MakeSlashTerminated(search);
    search += "*";
    WIN32_FIND_DATA fil;
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil != INVALID_HANDLE_VALUE)
    {
        do
        {
            if( fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                if (!strcmp(fil.cFileName, ".") || !strcmp(fil.cFileName, ".."))
                    continue;

                std::string d(fil.cFileName);
                dirs.push_back(d);

                if (recursive) // need a better way to do that
                {
                    StringList newdirs;
                    GetDirList(d.c_str(), newdirs, true);
                    
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
    FILE *fp = fopen(fn, "rb");
    if(fp)
    {
        fclose(fp);
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
    d.reserve(strlen(dir));
    bool last;
    for(StringList::iterator it = li.begin(); it != li.end(); it++)
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
    if(!fn || !*fn)
        return 0;
    FILE *fp = fopen(fn, "rb");
    if(!fp)
        return 0;
    fseek(fp, 0, SEEK_END);
    vfspos s = (vfspos)ftell(fp);
    fclose(fp);

    return s == npos ? 0 : s;
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
    const char *p = s.c_str();
    while(p[0] == '.' && p[1] == '/')
        p += 2;
    return FixSlashes(p == s.c_str() ? s : p); // avoid hidden re-allocation when pointer was not moved
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
        return status.st_mode & S_IFDIR;
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
    if(s[s.length() - 1] == '/')
        return StripLastPath(s.substr(0, s.length() - 1));

    size_t pos = s.find_last_of('/');
    if(pos == std::string::npos)
        return ""; // nothing remains

    return s.substr(0, pos);
}

void GetFileListRecursive(std::string dir, StringList& files, bool withQueriedDir /* = false */)
{
    std::stack<std::string> stk;

    if(withQueriedDir)
    {
        stk.push(dir);
        while(stk.size())
        {
            dir = stk.top();
            stk.pop();
            MakeSlashTerminated(dir);

            StringList li;
            GetFileList(dir.c_str(), li);
            for(std::deque<std::string>::iterator it = li.begin(); it != li.end(); ++it)
                files.push_back(dir + *it);

            li.clear();
            GetDirList(dir.c_str(), li, true);
            for(std::deque<std::string>::iterator it = li.begin(); it != li.end(); ++it)
                stk.push(dir + *it);
        }
    }
    else
    {
        std::string topdir = dir;
        MakeSlashTerminated(topdir);
        stk.push("");
        while(stk.size())
        {
            dir = stk.top();
            stk.pop();
            MakeSlashTerminated(dir);

            StringList li;
            dir = topdir + dir;
            GetFileList(dir.c_str(), li);
            for(std::deque<std::string>::iterator it = li.begin(); it != li.end(); ++it)
                files.push_back(dir + *it);

            li.clear();
            GetDirList(dir.c_str(), li, true);
            for(std::deque<std::string>::iterator it = li.begin(); it != li.end(); ++it)
                stk.push(dir + *it);
        }
    }
}

// from http://board.byuu.org/viewtopic.php?f=10&t=1089&start=15
bool WildcardMatch(const char *str, const char *pattern)
{
    const char *cp = 0, *mp = 0;
    while(*str && *pattern != '*')
    {
        if(*pattern != *str && *pattern != '?')
            return false;
        pattern++, str++;
    }

    while(*str)
    {
        if(*pattern == '*')
        {
            if(!*++pattern)
                return 1;
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

    while(*pattern++ == '*');

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

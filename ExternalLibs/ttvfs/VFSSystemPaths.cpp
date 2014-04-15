#include "VFSInternal.h"

#ifdef _WIN32
#  include <shlobj.h>
#endif

#include "VFSSystemPaths.h"
#include "VFSTools.h"


VFS_NAMESPACE_START


std::string GetUserDir()
{
    const char *user;
#ifdef _WIN32
    TCHAR szPath[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szPath)))
    {
        return szPath;
    }

    // Fallback
    user = getenv("USERPROFILE");
    if(user)
        return user;

#endif

    // Assume POSIX compliance
    user = getenv("HOME");
    if(user)
        return user;

    return "";
}

std::string GetAppDir(const char *appname)
{
    std::string ret;

#ifdef _WIN32

    TCHAR szPath[MAX_PATH];

    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath))) 
    {
        ret = szPath;
    }
    else
    {
        // Fallback
        const char *user = getenv("APPDATA");
        if(user)
            ret = user;
        else
            ret = "."; // Seems we have no other choice
    }

    ret += '/';

#else // Assume POSIX compliance

    const char *user = getenv("HOME");
    if(user)
        ret = user;
    else
        ret = ".";

    ret += "/.";
    

#endif

    ret += appname;
    FixPath(ret);
    return ret;
}




VFS_NAMESPACE_END

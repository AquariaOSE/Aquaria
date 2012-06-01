#ifdef _WIN32
#  include <shlobj.h>
#endif

#include "VFSSystemPaths.h"
#include "VFSTools.h"
#include "VFSInternal.h"

VFS_NAMESPACE_START


std::string GetUserDir()
{
#ifdef _WIN32
    TCHAR szPath[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szPath))) 
    {
        return szPath;
    }

    // Fallback
    const char *user = getenv("USERPROFILE");
    if(user)
        return user;

    // Sorry, windoze :(
    return "";

#else // Assume POSIX compliance
    const char *user = getenv("HOME");
    if(user)
        return user;
#endif
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

    return FixPath(ret + '/' + appname);

#else // Assume POSIX compliance

    const char *user = getenv("HOME");
    if(user)
        ret = user;
    else
        ret = ".";

    return FixPath(ret + "/." + appname); // just in case

#endif
}




VFS_NAMESPACE_END

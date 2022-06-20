#include "OSFunctions.h"
#include "Base.h"
#include <sstream>

#include "ttvfs_stdio.h"

#ifdef BBGE_BUILD_WINDOWS
//#  define WIN32_LEAN_AND_MEAN
#  define WIN32_NOMINMAX
#  include <windows.h>
#  undef min
#  undef max
#include <shellapi.h>
#elif BBGE_BUILD_LINUX
#  include <sys/types.h>
#  include <stdint.h>
#endif

#ifdef _MSC_VER
#	include <intrin.h>
#endif

#if defined(BBGE_BUILD_UNIX)
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if defined(BBGE_BUILD_MACOSX)
#include <Carbon/Carbon.h>
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFString.h>

// veeery clunky.
static std::string _CFToStdString(CFStringRef cs)
{
	char buf[1024];
	CFStringGetCString(cs, &buf[0], 1024, kCFStringEncodingUTF8);
	return &buf[0];
}
#endif

#ifdef BBGE_BUILD_VFS
#include "ttvfs.h"
#endif

#include "SDL.h"
#include "SDL_syswm.h"

#ifdef BBGE_BUILD_WINDOWS
static HICON icon_windows = 0;
#endif

void initIcon(void *screen)
{
#ifdef BBGE_BUILD_WINDOWS
	HINSTANCE handle = ::GetModuleHandle(NULL);
	if(!icon_windows)
		icon_windows = ::LoadIcon(handle, "icon");
	if(icon_windows)
	{
		SDL_SysWMinfo wminfo;
		SDL_VERSION(&wminfo.version)

#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_GetWindowWMInfo((SDL_Window*)screen, &wminfo);
		HWND hwnd = wminfo.info.win.window;
#else
		SDL_GetWMInfo(&wminfo);
		HWND hwnd = wminfo.window;
#endif

		::SetClassLongPtr(hwnd, -14, (LONG)(uintptr_t)icon_windows); // -14 is GCL_HICON (32bit) or GCLP_HICON (64bit)
	}
#endif
}

void destroyIcon()
{
#ifdef BBGE_BUILD_WINDOWS
	if (icon_windows)
	{
		::DestroyIcon(icon_windows);
		icon_windows = 0;
	}
#endif
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


void triggerBreakpoint()
{
#ifdef _MSC_VER
	__debugbreak();
#elif defined(__GNUC__) && ((__i386__) || (__x86_64__))
	__asm__ __volatile__ ( "int $3\n\t" );
#else
	raise(SIGTRAP);
#endif
}



bool createDir(const std::string& d)
{
	bool success = false;
	int err = 0;
#if defined(BBGE_BUILD_UNIX)
	if (!mkdir(d.c_str(), S_IRWXU))
		success = true;
	else
	{
		err = errno;
		if (err == EEXIST)
			success = true;
	}
#elif defined(BBGE_BUILD_WINDOWS)
	if (CreateDirectoryA(d.c_str(), NULL))
		success = true;
	else
	{
		err = GetLastError();
		if(err == ERROR_ALREADY_EXISTS)
			success = true;
	}
#endif
	if (!success)
	{
		std::ostringstream os;
		os <<  "Failed to create directory: [" << d << "], error code: " << err;
		debugLog(os.str());
	}
	return success;
}

#if defined(BBGE_BUILD_MACOSX) && !SDL_VERSION_ATLEAST(2,0,0)
void cocoaMessageBox(const std::string &title, const std::string &msg);
#endif

void messageBox(const std::string& title, const std::string &msg)
{
#ifdef BBGE_BUILD_WINDOWS
	MessageBox (0,msg.c_str(),title.c_str(),MB_OK);
#elif SDL_VERSION_ATLEAST(2,0,0)
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, title.c_str(),
		msg.c_str(), NULL);
#elif defined(BBGE_BUILD_MACOSX)
	cocoaMessageBox(title, msg);
#elif defined(BBGE_BUILD_UNIX)
	// !!! FIXME: probably don't want the whole GTK+ dependency in here...
	fprintf(stderr, "%s: %s\n", title.c_str(), msg.c_str());
#else
#error Please define your platform.
#endif
}


#ifdef BBGE_BUILD_VFS

struct vfscallback_s
{
	const std::string *path;
	const char *ext;
	void *param;
	void (*callback)(const std::string &filename, void *param);
};

void forEachFile_vfscallback(VFILE *vf, void *user)
{
	vfscallback_s *d = (vfscallback_s*)user;
	if(d->ext)
	{
		const char *e = strrchr(vf->name(), '.');
		if(e && nocasecmp(d->ext, e))
			return;
	}
	d->callback(*(d->path) + vf->name(), d->param);
}

#endif

void forEachFile(const std::string& inpath, std::string type, void callback(const std::string &filename, void *param), void *param)
{
	if (inpath.empty()) return;

#ifdef BBGE_BUILD_VFS
	ttvfs::DirView view;
	if(!vfs.FillDirView(inpath.c_str(), view))
	{
		debugLog("Path '" + inpath + "' does not exist");
		return;
	}
	vfscallback_s dat;
	dat.path = &inpath;
	dat.ext = type.length() ? type.c_str() : NULL;
	dat.param = param;
	dat.callback = callback;
	view.forEachFile(forEachFile_vfscallback, &dat, true);

	return;
	// -------------------------------------
#endif

	stringToLower(type);
	std::string path = adjustFilenameCase(inpath.c_str());
	debugLog("forEachFile - path: " + path + " type: " + type);

#if defined(BBGE_BUILD_UNIX)
	DIR *dir=0;
	dir = opendir(path.c_str());
	if (dir)
	{
		dirent *file=0;
		while ( (file=readdir(dir)) != NULL )
		{
			if (strlen(file->d_name) > 4)
			{
				debugLog(file->d_name);
				char *extension=strrchr(file->d_name,'.');
				if (extension)
				{
					debugLog(extension);
					if (extension!=NULL)
					{
						if (strcasecmp(extension,type.c_str())==0)
						{
							callback(path + std::string(file->d_name), param);
						}
					}
				}
			}
		}
		closedir(dir);
	}
	else
	{
		debugLog("FAILED TO OPEN DIR");
	}
#endif

#ifdef BBGE_BUILD_WINDOWS
	BOOL            fFinished;
	HANDLE          hList;
	TCHAR           szDir[MAX_PATH+1];
	WIN32_FIND_DATA FileData;

	int end = path.size()-1;
	if (path[end] != '/')
		path[end] += '/';

	// Get the proper directory path
	// \\ %s\\*



	if (type.find('.')==std::string::npos)
	{
		type = "." + type;
	}



	sprintf(szDir, "%s\\*", path.c_str());

	stringToUpper(type);

	// Get the first file
	hList = FindFirstFile(szDir, &FileData);
	if (hList == INVALID_HANDLE_VALUE)
	{

		debugLog("No files of type " + type + " found in path " + path);
	}
	else
	{
		// Traverse through the directory structure
		fFinished = FALSE;
		while (!fFinished)
		{
			// Check the object is a directory or not
			//printf("%*s%s\n", indent, "", FileData.cFileName);
			std::string filename = FileData.cFileName;

			if (filename.size()>4)
			{

				std::string filetype = filename.substr(filename.size()-4, filename.size());
				stringToUpper(filetype);

				if (filetype==type)
				{
					callback(path+filename, param);
				}
			}


			if (!FindNextFile(hList, &FileData))
			{

				fFinished = TRUE;
			}
		}
	}

	FindClose(hList);
#endif
}



#if BBGE_BUILD_UNIX
// based on code I wrote for PhysicsFS: http://icculus.org/physfs/
//  the zlib license on physfs allows this cut-and-pasting.
static int locateOneElement(char *buf)
{
	char *ptr;
	DIR *dirp;

	if (access(buf, F_OK) == 0)
		return(1);  // quick rejection: exists in current case.

	ptr = strrchr(buf, '/');  // find entry at end of path.
	if (ptr == NULL)
	{
		dirp = opendir(".");
		ptr = buf;
	}
	else
	{
		*ptr = '\0';
		dirp = opendir(buf);
		*ptr = '/';
		ptr++;  // point past dirsep to entry itself.
	}

	struct dirent *dent;
	while ((dent = readdir(dirp)) != NULL)
	{
		if (strcasecmp(dent->d_name, ptr) == 0)
		{
			strcpy(ptr, dent->d_name); // found a match. Overwrite with this case.
			closedir(dirp);
			return(1);
		}
	}

	// no match at all...
	closedir(dirp);
	return(0);
}
#endif

std::string adjustFilenameCase(const std::string& s)
{
	return adjustFilenameCase(s.c_str());
}

std::string adjustFilenameCase(const char *_buf)
{
#ifdef BBGE_BUILD_UNIX  // any case is fine if not Linux.
	int rc = 1;
	char *buf = (char *) alloca(strlen(_buf) + 1);
	strcpy(buf, _buf);

	char *ptr = buf;
	while ((ptr = strchr(ptr + 1, '/')) != 0)
	{
		*ptr = '\0';  // block this path section off
		rc = locateOneElement(buf);
		*ptr = '/'; // restore path separator
		if (!rc)
			break;  // missing element in path.
	}

	// check final element...
	if (rc)
		rc = locateOneElement(buf);

#if 0
	if (strcmp(_buf, buf) != 0)
	{
		fprintf(stderr, "Corrected filename case: '%s' => '%s (%s)'\n",
			_buf, buf, rc ? "found" : "not found");
	}
#endif

	return std::string(buf);
#else
	return std::string(_buf);
#endif
}


std::string getSystemLocale()
{
	std::string localeStr;

#ifdef BBGE_BUILD_WINDOWS
	LCID lcid = GetThreadLocale();

	char buf[100];
	char ctry[100];

	if (GetLocaleInfo(lcid, LOCALE_SISO639LANGNAME, buf, sizeof buf) != 0)
	{
		localeStr = buf;

		if (GetLocaleInfo(lcid, LOCALE_SISO3166CTRYNAME, ctry, sizeof ctry) != 0)
		{
			localeStr += "_";
			localeStr += ctry;
		}
	}
#elif BBGE_BUILD_MACOSX
	CFLocaleRef locale = CFLocaleCopyCurrent();
	CFStringRef buf;

	if ((buf = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleLanguageCode)) != NULL)
	{
		localeStr = _CFToStdString(buf);

		if ((buf = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleCountryCode)) != NULL)
		{
			localeStr += "_";
			localeStr += _CFToStdString(buf);
		}
	}

	CFRelease(locale);

#else
	const char *lang = (const char *)getenv("LANG");

	if (lang && *lang)
	{
		localeStr = lang;

		size_t found = localeStr.find('.');

		if (found != std::string::npos)
			localeStr.resize(found);
	}
#endif

	return localeStr;
}

std::string getWorkingDir()
{
	char buf[1024*2] = {0};
#ifdef _WIN32
	GetCurrentDirectoryA(sizeof(buf), buf);
#else
	getcwd(buf, sizeof(buf));
#endif
	return buf;
}

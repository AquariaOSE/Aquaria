#include "Localization.h"

#ifdef BBGE_BUILD_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef BBGE_BUILD_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef BBGE_BUILD_MACOSX
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

static std::string s_locale;
static std::string s_modpath;

void setUsedLocale(const std::string& s)
{
	s_locale = s;
}

const char *getUsedLocale()
{
	return s_locale.c_str();
}

void setLocalisationModPath(const std::string& s)
{
	s_modpath = s;
	stringToLower(s_modpath);
}

// hackish
// intended to be used only for paths which are known to start with the mod path,
// but can deal with it if this is not the case
std::string localisePathInternalModpath(const std::string &path)
{
	std::string tmp = path;
	stringToLower(tmp);

	if(!strncmp(tmp.c_str(), s_modpath.c_str(), s_modpath.length()))
		return localisePath(path, s_modpath);

	return localisePath(path);
}

std::string localisePath(const std::string &path, const std::string& modpath /* = "" */)
{
	if (s_locale.empty() || s_locale == "-")
		return path;

	const std::string fname = path.substr(modpath.length());

	/* we first try with complete locale name, i.e "locales/en_US/" */
	std::string localisedPath = modpath + "locales/" + s_locale + "/" + fname;

	if (exists(localisedPath.c_str()))
		return localisedPath;

	/* ok didn't work, let's retry with only language part of locale name, i.e "locales/en/" */
	const size_t found = s_locale.find('_');

	/* hmm, seems like we didn't have a full locale name anyway, use original path */
	if (found == std::string::npos)
		return path;

	localisedPath = modpath + "locales/" + s_locale.substr(0,found) + "/" + fname;

	/* hooray we found a file! */
	if (exists(localisedPath.c_str()))
		return localisedPath;

	/* seems like we don't have a localized version of the file available, use original path */
	return path;
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
		CFRelease(buf);

		if ((buf = (CFStringRef)CFLocaleGetValue(locale, kCFLocaleCountryCode)) != NULL)
		{
			localeStr += "_";
			localeStr += _CFToStdString(buf);
			CFRelease(buf);
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


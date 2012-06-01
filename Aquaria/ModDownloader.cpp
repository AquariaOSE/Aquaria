#include "DSQ.h"
#include "minihttp.h"

#ifdef BBGE_BUILD_VFS

#include "ModDownloader.h"
#include "ModSelector.h"
#include "Network.h"
#include "tinyxml.h"

#ifdef BBGE_BUILD_UNIX
#include <sys/stat.h>
#endif

using Network::NetEvent;
using Network::NE_ABORT;
using Network::NE_FINISH;
using Network::NE_UPDATE;


// external, global
ModDL moddl;


// TODO: move this to Base.cpp and replace other similar occurrances
static void createDir(const char *d)
{
#if defined(BBGE_BUILD_UNIX)
	mkdir(d, S_IRWXU);
#elif defined(BBGE_BUILD_WINDOWS)
	CreateDirectoryA(d, NULL);
#endif
}

// .../_mods/<MODNAME>
// .../_mods/<MODNAME>.zip
static std::string _PathToModName(const std::string& path)
{
	size_t pos = path.find_last_of('/')+1;
	size_t pos2 = path.find_last_of('.');
	return path.substr(pos, pos2-pos);
}

// fuuugly
static bool _CompareByPackageURL(ModIconOnline *ico, const std::string& n)
{
	return ico->packageUrl == n;
}
static bool _CompareByIcon(ModIconOnline *ico, const std::string& n)
{
	return ico->iconfile == n;
}
// this function is required because it is never guaranteed that the original
// ModIconOnline which triggered the download still exists.
// This means the pointer to the icon can't be stored anywhere without risking crashing.
// Instead, use this way to find the correct icon, even if it was deleted and recreated in the meantime.
static ModIconOnline *_FindModIconOnline(const std::string& n, bool (*func)(ModIconOnline*,const std::string&))
{
	ModSelectorScreen* scr = dsq->modSelectorScr;
	IconGridPanel *grid = scr? scr->panels[2] : NULL;
	if(!grid)
		return NULL;

	for(RenderObject::Children::iterator it = grid->children.begin(); it != grid->children.end(); ++it)
	{
		ModIconOnline *ico = dynamic_cast<ModIconOnline*>(*it);
		if(ico && func(ico, n))
			return ico;
	}
	return NULL;
}

class ModlistRequest : public Network::RequestData
{
public:
	ModlistRequest(bool chain) : allowChaining(chain), first(false) {}
	virtual ~ModlistRequest() {}
	virtual void notify(NetEvent ev, size_t recvd, size_t total)
	{
		moddl.NotifyModlist(this, ev, recvd, total);
		if(ev == NE_ABORT || ev == NE_FINISH)
			delete this;
	}
	bool allowChaining;
	bool first;
};

class IconRequest : public Network::RequestData
{
public:
	virtual ~IconRequest() {}
	virtual void notify(NetEvent ev, size_t recvd, size_t total)
	{
		moddl.NotifyIcon(this, ev, recvd, total);
		if(ev == NE_ABORT || ev == NE_FINISH)
			delete this;
	}
};

class ModRequest : public Network::RequestData
{
public:
	virtual ~ModRequest() {}
	virtual void notify(NetEvent ev, size_t recvd, size_t total)
	{
		moddl.NotifyMod(this, ev, recvd, total);
		if(ev == NE_ABORT || ev == NE_FINISH)
			delete this;
	}
	std::string modname;
};

ModDL::ModDL()
{
}

ModDL::~ModDL()
{
}

void ModDL::init()
{
	tempDir = dsq->getUserDataFolder() + "/webcache";
	createDir(tempDir.c_str());

	ttvfs::VFSDir *vd = vfs.GetDir(tempDir.c_str());
	if(vd)
		vd->load(false);
}

bool ModDL::hasUrlFileCached(const std::string& url)
{
	return exists(remoteToLocalName(url));
}

std::string ModDL::remoteToLocalName(const std::string& url)
{
	if(!url.length())
		return "";

	std::string here;
	here.reserve(url.length() + tempDir.length() + 2);
	here += tempDir;
	here += '/';
	for(size_t i = 0; i < url.length(); ++i)
	{
		if(!(isalnum(url[i]) || url[i] == '_' || url[i] == '-' || url[i] == '.'))
			here += '_';
		else
			here += url[i];
	}
	return here;
}

void ModDL::GetModlist(const std::string& url, bool allowChaining, bool first)
{
	if(first)
		knownServers.clear();
	
	// Prevent recursion, self-linling, or cycle linking.
	// In theory, this allows setting up a server network
	// where each server links to any servers it knows,
	// without screwing up, but this isn't going to happen anyways.
	// It's still useful for safety. -- FG
	if(knownServers.size() > 30)
	{
		debugLog("GetModlist: Too many servers. Whaat?!");
		return;
	}
	else
	{
		std::string host, dummy_file;
		int dummy_port;
		minihttp::SplitURI(url, host, dummy_file, dummy_port);
		stringToLower(host);
		if(knownServers.find(host) != knownServers.end())
		{
			debugLog("GetModlist: Already seen host: " + host + " - ignoring");
			return;
		}
		knownServers.insert(host);
	}

	std::ostringstream os;
	os << "Fetching mods list [" << url << "], chain: " << allowChaining;
	debugLog(os.str());
	
	std::string localName = remoteToLocalName(url);

	debugLog("... to: " + localName);

	ModlistRequest *rq = new ModlistRequest(allowChaining);
	rq->tempFilename = localName;
	rq->finalFilename = localName;
	rq->allowChaining = allowChaining;
	rq->url = url;
	rq->first = first;

	Network::download(rq);

	ModSelectorScreen* scr = dsq->modSelectorScr;
	if(scr)
	{
		scr->globeIcon->quad->color.interpolateTo(Vector(1,1,1), 0.3f);
		scr->globeIcon->alpha.interpolateTo(0.5f, 0.2f, -1, true, true);
		scr->dlText.setText("Retrieving online mod list...");
		scr->dlText.alpha.stopPath();
		scr->dlText.alpha.interpolateTo(1, 0.1f);
	}
}

void ModDL::NotifyModlist(ModlistRequest *rq, NetEvent ev, size_t recvd, size_t total)
{
	if(ev == NE_UPDATE)
		return;

	ModSelectorScreen* scr = dsq->modSelectorScr;

	if(ev == NE_ABORT)
	{
		dsq->sound->playSfx("denied");
		if(scr)
		{
			scr->globeIcon->alpha.stop();
			scr->globeIcon->alpha.interpolateTo(1, 0.5f, 0, false, true);
			scr->globeIcon->quad->color.interpolateTo(Vector(0.5f, 0.5f, 0.5f), 0.3f);
			scr->dlText.setText("Unable to retrieve online mod list.\nCheck your connection and try again."); // TODO: put into stringbank
			scr->dlText.alpha = 0;
			scr->dlText.alpha.ensureData();
			scr->dlText.alpha.data->path.addPathNode(0, 0);
			scr->dlText.alpha.data->path.addPathNode(1, 0.1);
			scr->dlText.alpha.data->path.addPathNode(1, 0.7);
			scr->dlText.alpha.data->path.addPathNode(0, 1);
			scr->dlText.alpha.startPath(5);

			// Allow requesting another server list if the initial fetch failed.
			// Do not care for child servers.
			if(rq->first)
				scr->gotServerList = false;
		}
		return;
	}
	
	if(scr)
	{
		scr->globeIcon->alpha.stop();
		scr->globeIcon->alpha.interpolateTo(1, 0.2f);
		scr->dlText.alpha.stopPath();
		scr->dlText.alpha.interpolateTo(0, 0.3f);
		if(rq->first)
			dsq->clickRingEffect(scr->globeIcon->getWorldPosition(), 1);
	}

	if(!ParseModXML(rq->finalFilename, rq->allowChaining))
	{
		if(scr)
		{
			scr->dlText.alpha.stopPath();
			scr->dlText.alpha.interpolateTo(1, 0.5f);
			scr->dlText.setText("Server error!\nBad XML, please contact server admin.\nURL: " + rq->url); // TODO: -> stringbank
		}
	}
}

bool ModDL::ParseModXML(const std::string& fn, bool allowChaining)
{
	TiXmlDocument xml;
	if(!xml.LoadFile(fn))
	{
		debugLog("Failed to parse downloaded XML: " + fn);
		return false;
	}

	ModSelectorScreen* scr = dsq->modSelectorScr;
	IconGridPanel *grid = scr? scr->panels[2] : NULL;

	// XML Format:
	/*
	<ModList>
		<Server url="example.com/mods.xml" chain="1" /> //-- Server network - link to other servers
		...
		<AquariaMod>
			<Fullname text="Jukebox"/>
			<Description text="Listen to all the songs in the game!" />
			<Icon url="localhost/aq/jukebox.png" size="1234" /> // -- size is optional, used to detect file change on server
			<Package url="localhost/aq/jukebox.aqmod" saveAs="jukebox" size="1234" /> // -- saveAs is optional, and ".aqmod" appended to it
			<Author name="Dolphin's Cry" />  //-- optional tag
			<Confirm text="" />  //-- optional tag, pops up confirm dialog
			<Properties type="patch" /> //-- optional tag, if not given, "mod" is assumed.
		</AquariaMod>
		
		<AquariaMod>
		...
		</AquariaMod>
	<ModList>
	*/

	TiXmlElement *modlist = xml.FirstChildElement("ModList");
	if(!modlist)
	{
		debugLog("ModList root tag not found");
		return false;
	}

	if(allowChaining)
	{
		TiXmlElement *servx = modlist->FirstChildElement("Server");
		while(servx)
		{
			int chain = 0;
			servx->Attribute("chain", &chain);
			if(const char *url = servx->Attribute("url"))
				GetModlist(url, chain, false);

			servx = servx->NextSiblingElement("Server");
		}
	}

	TiXmlElement *modx = modlist->FirstChildElement("AquariaMod");
	while(modx)
	{
		std::string namestr, descstr, iconurl, pkgurl, confirmStr, localname;
		std::string sizestr;
		bool isPatch = false;
		int serverSize = 0;
		int serverIconSize = 0;
		TiXmlElement *fullname, *desc, *icon, *pkg, *confirm, *props;
		fullname = modx->FirstChildElement("Fullname");
		desc = modx->FirstChildElement("Description");
		icon = modx->FirstChildElement("Icon");
		pkg = modx->FirstChildElement("Package");
		confirm = modx->FirstChildElement("Confirm");
		props = modx->FirstChildElement("Properties");

		if(fullname && fullname->Attribute("text"))
			namestr = fullname->Attribute("text");

		if(desc && desc->Attribute("text"))
			descstr = desc->Attribute("text");

		if(icon)
		{
			if(icon->Attribute("url"))
				iconurl = icon->Attribute("url");
			if(icon->Attribute("size"))
				icon->Attribute("size", &serverIconSize);
		}

		if(props && props->Attribute("type"))
			isPatch = !strcmp(props->Attribute("type"), "patch");

		if(pkg)
		{
			if(pkg->Attribute("url"))
			{
				pkgurl = pkg->Attribute("url");
				localname = _PathToModName(pkgurl);
			}
			if(pkg->Attribute("saveAs"))
				localname = _PathToModName(pkg->Attribute("saveAs"));

			if(pkg->Attribute("size"))
				pkg->Attribute("size", &serverSize);
		}

		if(confirm && confirm->Attribute("text"))
			confirmStr = confirm->Attribute("text");

		modx = modx->NextSiblingElement("AquariaMod");

		// -------------------

		if (descstr.size() > 255)
			descstr.resize(255);

		std::string localIcon = remoteToLocalName(iconurl);

		size_t localIconSize = 0;
		if(ttvfs::VFSFile *vf = vfs.GetFile(localIcon.c_str()))
		{
			localIconSize = vf->size();
		}

		debugLog("NetMods: " + namestr);

		ModIconOnline *ico = NULL;
		if(grid)
		{
			ico = new ModIconOnline;
			ico->iconfile = localIcon;
			ico->packageUrl = pkgurl;
			ico->namestr = namestr;
			ico->desc = descstr;
			ico->confirmStr = confirmStr;
			ico->localname = localname;
			ico->label = "--[ " + namestr + " ]--\n" + descstr;
			ico->isPatch = isPatch;

			if(serverSize && dsq->modIsKnown(localname))
			{
				std::string modpkg = dsq->mod.getBaseModPath() + localname;
				modpkg += ".aqmod";
				ttvfs::VFSFile *vf = vfs.GetFile(modpkg.c_str());
				if(vf)
				{
					size_t sz = vf->size();
					ico->hasUpdate = (serverSize && ((size_t)serverSize != sz));
				}
				// if vf==NULL, then the mod was not installed with the mod downloader.
				// There is a warning on download that's supposed to prevent this.
				// However, if we end up with vf==NULL, there's not much to do about it.
			}

			// try to set texture, if its not there, download it.
			// download a new icon if file size changed.
			if(!ico->fixIcon() || !localIconSize || (serverIconSize && (size_t)serverIconSize != localIconSize))
			{
				ico->setDownloadProgress(0, 10);
				GetIcon(iconurl, localIcon);
				// we do not pass the ico ptr to the call above; otherwise it will crash if the mod menu is closed
				// while a download is in progress
			}

			grid->add(ico);
		}
	}

	return true;
}

void ModDL::GetMod(const std::string& url, const std::string& localname)
{
	ModRequest *rq = new ModRequest;

	if(localname.empty())
		rq->modname = _PathToModName(url);
	else
		rq->modname = localname;

	rq->tempFilename = remoteToLocalName(url);
	rq->finalFilename = rq->tempFilename; // we will fix this later on
	rq->url = url;

	debugLog("ModDL::GetMod: " + rq->finalFilename);

	Network::download(rq);
}

void ModDL::GetIcon(const std::string& url, const std::string& localname)
{
	if(url.empty())
		return;
	IconRequest *rq = new IconRequest;
	rq->url = url;
	rq->finalFilename = localname;
	rq->tempFilename = localname;
	debugLog("ModDL::GetIcon: " + localname);
	Network::download(rq);
}

void ModDL::NotifyIcon(IconRequest *rq, NetEvent ev, size_t recvd, size_t total)
{
	ModIconOnline *ico = _FindModIconOnline(rq->finalFilename, _CompareByIcon);
	if(ico)
	{
		float perc = -1; // no progress bar
		if(ev == NE_FINISH)
			ico->fixIcon();
		else if(ev == NE_UPDATE)
			perc = total ? ((float)recvd / (float)total) : 0.0f;

		ico->setDownloadProgress(perc, 10); // must be done after setting the new texture for proper visuals
	}
}

void ModDL::NotifyMod(ModRequest *rq, NetEvent ev, size_t recvd, size_t total)
{
	if(ev == NE_ABORT)
		dsq->sound->playSfx("denied");
	else if(ev == NE_FINISH)
		dsq->sound->playSfx("gem-collect");

	ModIconOnline *ico = _FindModIconOnline(rq->url, _CompareByPackageURL);
	if(!ico)
	{
		if(ev == NE_FINISH)
			dsq->centerMessage("Finished downloading mod " + rq->modname, 420); // TODO: -> stringbank
		return;
	}

	float perc = -1;
	if(ev == NE_UPDATE)
		perc = total ? ((float)recvd / (float)total) : 0.0f;

	ico->setDownloadProgress(perc);
	ico->clickable = (ev == NE_ABORT || ev == NE_FINISH);

	if(ev == NE_FINISH)
	{
		const std::string& localname = ico->localname;
		std::string moddir = dsq->mod.getBaseModPath() + localname;
		// the mod file can already exist, and if it does, it will most likely be mounted.
		// zip archives are locked and cannot be deleted/replaced, so we need to unload it first.
		// this effectively closes the file handle only, nothing else.
		ttvfs::VFSDir *vd = vfs.GetDir(moddir.c_str());
		if(vd)
		{
			ttvfs::VFSBase *origin = vd->getOrigin();
			if(origin)
				origin->close();
		}

		std::string archiveFile = moddir + ".aqmod";

		// At least on win32 rename() fails when the destination file already exists
		remove(archiveFile.c_str());
		if(rename(rq->tempFilename.c_str(), archiveFile.c_str()))
		{
			debugLog("Could not rename mod " + rq->tempFilename + " to " + archiveFile);
			return;
		}
		else
			debugLog("ModDownloader: Renamed mod " + rq->tempFilename + " to " + archiveFile);

		if(vd)
		{
			// Dir already exists, just remount everything
			vfs.Reload();
		}
		else if(!dsq->mountModPackage(archiveFile)) 
		{
			// make package readable (so that the icon can be shown)
			// But only if it wasn't mounted before!
			dsq->screenMessage("Failed to mount archive: " + archiveFile);
			return;
		}

		// if it is already known, the file was re-downloaded
		if(!dsq->modIsKnown(localname))
		{
			// yay, got something new!
			DSQ::loadModsCallback(archiveFile, 0); // does not end in ".xml" but thats no problem here
			if(dsq->modSelectorScr)
				dsq->modSelectorScr->initModAndPatchPanel(); // HACK
		}

		ico->hasUpdate = false;
		ico->fixIcon();
	}
}

#endif // BBGE_BUILD_VFS

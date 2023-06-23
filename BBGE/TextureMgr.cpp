#include "TextureMgr.h"
#include <sstream>
#include <SDL.h>
#include "MT.h"
#include "Image.h"
#include "Base.h"
#include "Localization.h"

struct TexLoadTmp
{
    TexLoadTmp() : loadmode(TextureMgr::KEEP), curTex(NULL), success(false), arrayidx(0) { img.pixels = NULL; }
    std::string name, filename;
    ImageData img;
    TextureMgr::LoadMode loadmode;
    Texture *curTex; // immutable
    bool success; // if this is true and img.pixels is NULL, don't change anything
    //bool mipmap;
    size_t arrayidx;
};

typedef ImageData (*ImageLoadFunc)(const char *fn);

struct TexLoader
{
    TexLoader(ImageLoadFunc f, const std::string& s, const std::string& name = std::string())
        : loader(f), fn(s), name(name) {}
    ImageLoadFunc loader;
    std::string fn;
    std::string name; // usually empty, but may specify tex name in case cleanups had to be done. always lowercase.
    ImageData load() const { return loader(fn.c_str()); }
};

static ImageData loadGeneric(const char *fn)
{
    return imageLoadGeneric(fn, false);
}

static ImageData loadQOI(const char *fn)
{
    return imageLoadQOI(fn, false);
}

// This handles unix/win32 relative paths: ./rel/path
// Unix abs paths: /home/user/...
// Win32 abs paths: C:/Stuff/.. and also C:\Stuff\...
// -- We want to keep those paths untouched and just load as-is.
// This is mainly used for special things like savegame thumbnails and mod preview images,
// that exist outside of the game's known texture directories and are also not part of a running mod.
static bool isSpecialPath(const std::string& s)
{
    return s[0] == '.' || s[0] == '/' || (s.length() > 1 && s[1] == ':');
}

static const std::string fixup(const std::string& fn)
{
    std::string file = localisePathInternalModpath(fn);
	file = adjustFilenameCase(file);
    return file;
}


struct ExtAndLoader
{
    const char *ext;
    ImageLoadFunc func;
};

static const ExtAndLoader s_extAndLoader[] =
{
    { "png", loadGeneric },
    { "qoi", loadQOI },
    { "jpg", loadGeneric },
    { "zga", imageLoadZGA },
    { "tga", loadGeneric },
    { NULL, NULL }
};

static TexLoader getFullnameAndLoader(const std::string& name, const std::string& basedir)
{
    // TODO: use localisePath()
    // TODO: assert that we don't start with . or /

    if(exists(name))
    {
        // check first if name exists and has a known extension, if so, use correct loader
        size_t lastdot = name.rfind('.');
        size_t lastslash = name.rfind('/');
        if(lastdot != std::string::npos && (lastslash == std::string::npos || lastslash < lastdot))
        {
            std::string ext = name.substr(lastdot + 1);
            for(const ExtAndLoader *exl = &s_extAndLoader[0]; exl->func; ++exl)
            {
                if(ext == exl->ext)
                {
                    // remove basedir and extension
                    std::string texname = name.substr(basedir.length(), name.length() - (basedir.length() + ext.length() + 1)); // strip extension
                    return TexLoader(exl->func, fixup(name), texname );
                }
            }
        }
    }

    std::string fn = name;
    for(const ExtAndLoader *exl = &s_extAndLoader[0]; exl->func; ++exl)
    {
        fn.resize(name.length());
        fn += '.';
        fn += exl->ext;
        if(exists(fn))
            return TexLoader(exl->func, fixup(fn));
    }

    return TexLoader(NULL, std::string());
}

static bool locateAndLoad(TexLoadTmp& tt, const std::string& basedir)
{
    std::string filename = basedir + tt.name;
    TexLoader ldr = getFullnameAndLoader(filename, basedir);
    if(!ldr.name.empty()) // name was cleaned up, use the updated one
        tt.name = ldr.name;
    if(ldr.loader)
    {
        if(tt.loadmode < TextureMgr::OVERWRITE && tt.curTex && tt.curTex->success && tt.curTex->filename == ldr.fn)
        {
            // still referring to the same file that was already loaded, so we wouldn't gain anything from loading it again
            tt.success = true;
            return true;
        }

        tt.filename = ldr.fn;
        tt.img = ldr.load();
        tt.success = !!tt.img.pixels;
        return true;
    }
    return false;
}

void TextureMgr::th_loadFromFile(TexLoadTmp& tt) const
{
    if(isSpecialPath(tt.name))
        if(locateAndLoad(tt, ""))
            return;

    for(size_t i = 0; i < loadFromPaths.size(); ++i)
        if(locateAndLoad(tt, loadFromPaths[i]))
            return;
}


TextureMgr::TextureMgr()
    : sem(SDL_CreateSemaphore(0))
{
}

TextureMgr::~TextureMgr()
{
    for(size_t i = 0; i < threads.size(); ++i)
        worktodo.push(NULL); // signal all threads to exit
    for(size_t i = 0; i < threads.size(); ++i)
        SDL_WaitThread((SDL_Thread*)threads[i], NULL);

    SDL_DestroySemaphore((SDL_sem*)sem);
}

size_t TextureMgr::spawnThreads(size_t n)
{
    for(size_t i = 0; i < n; ++i)
    {
        SDL_Thread *worker;
#if SDL_VERSION_ATLEAST(2,0,0)
		worker = SDL_CreateThread(Th_Main, "texldr", this);
#else
		worker = SDL_CreateThread(Th_Main, this);
#endif
        if(!worker)
            return i;

        threads.push_back(worker);
    }
    return n;
}

size_t TextureMgr::getNumLoaded() const
{
    return cache.size();
}

Texture* TextureMgr::getOrLoad(const std::string& name)
{
    return load(name, KEEP);
}

void TextureMgr::clearUnused()
{
    size_t done = 0;
    for(TexCache::iterator it = cache.begin(); it != cache.end(); )
    {
        if(it->second->refcount() <= 1)
        {
            it = cache.erase(it);
            ++done;
        }
        else
            ++it;
    }
    std::ostringstream os;
    os << "TextureMgr: Dropped " << done << " unused textures, now " << cache.size() << " left";
    debugLog(os.str());
}

void TextureMgr::unloadAll()
{
    for(TexCache::iterator it = cache.begin(); it != cache.end(); ++it)
        it->second->unload();
    cache.clear();
}

int TextureMgr::Th_Main(void* ud)
{
    TextureMgr *self = (TextureMgr*)ud;
    self->thMain();
    return 0;
}

void TextureMgr::thMain()
{
    for(;;)
    {
        void *p;
        worktodo.pop(p);
        if(!p) // a pushed NULL is the signal to exit
            break;

        TexLoadTmp *tt = (TexLoadTmp*)p;
        th_loadFromFile(*tt);

        workdone.push(p);
    }
}

Texture *TextureMgr::finalize(TexLoadTmp& tt)
{
    Texture *tex = tt.curTex;
    if(!tex)
    {
        tex = new Texture; // always make sure a valid Texture object comes out
        tex->name = tt.name; // this doesn't ever change
        cache[tt.name] = tex; // didn't exist, cache now
    }

    tex->filename = tt.filename;
    tex->success = tt.success;

    if(!tt.success)
    {
        debugLog("FAILED TO LOAD TEXTURE: [" + tt.name + "]");
        tex->unload();
        tex->width = 64;
        tex->height = 64;
    }
    if(tt.img.pixels)
    {
        debugLog("LOADED TEXTURE FROM DISK: [" + tt.filename + "]");
        tex->upload(tt.img, /*tt.mipmap*/ true);
        free(tt.img.pixels);
        tt.img.pixels = NULL;
    }
    return tex;
}

void TextureMgr::loadBatch(Texture * pdst[], const std::string texnames[], size_t n, LoadMode mode, ProgressCallback cb, void *cbUD)
{
    if(threads.empty())
    {
        for(size_t i = 0; i < n; ++i)
        {
            Texture *tex = load(texnames[i], mode);
            if(pdst)
                pdst[i] = tex;
        }
        return;
    }

    // Important that this is pre-allocated. We store pointers to elements and
    // send them to threads, so this must never reallocate.
    std::vector<TexLoadTmp> tmp(n);

    size_t inprogress = 0, doneCB = 0;
    for(size_t i = 0; i < n; ++i)
    {
        TexLoadTmp& tt = tmp[i];
        tt.arrayidx = i;
        tt.name = texnames[i];
        stringToLower(tt.name);
        TexCache::iterator it = cache.find(tt.name);
        tt.curTex = it != cache.end() ? it->second.content() : NULL;
        if(mode == KEEP && tt.curTex && tt.curTex->success)
        {
            if(pdst)
                pdst[i] = tt.curTex;
            if(cb)
                cb(++doneCB, cbUD);
            continue;
        }

        tt.loadmode = mode;

        worktodo.push(&tt);
        ++inprogress;
    }

    for(size_t i = 0; i < inprogress; ++i)
    {
        void *p;
        workdone.pop(p);
        TexLoadTmp& tt = *(TexLoadTmp*)p;
        Texture *tex = finalize(tt);
        if(pdst)
            pdst[tt.arrayidx] = tex;
        if(cb)
            cb(++doneCB, cbUD);
    }
}

Texture* TextureMgr::load(const std::string& texname, LoadMode mode)
{
    TexLoadTmp tt;
    tt.name = texname;
    stringToLower(tt.name);
    TexCache::iterator it = cache.find(tt.name);
    tt.curTex = it != cache.end() ? it->second.content() : NULL;

    // texname "" will never load, so don't even try once we have a valid object
    if(tt.curTex && (texname.empty() || (mode == KEEP && tt.curTex->success)))
        return tt.curTex;

    tt.loadmode = mode;
    th_loadFromFile(tt);
    return finalize(tt);
}

void TextureMgr::reloadAll(LoadMode mode)
{
    std::vector<std::string> todo;
    for(TexCache::iterator it = cache.begin(); it != cache.end(); ++it)
    {
        Texture *tex = it->second.content();
        if(mode == KEEP && tex->success)
            continue;

        todo.push_back(tex->name);
    }

    std::ostringstream os;
    os << "TextureMgr: Potentially reloading up to " << todo.size() << " textures...";
    debugLog(os.str());

    if(!todo.empty())
        loadBatch(NULL, &todo[0],todo.size(), mode);
}



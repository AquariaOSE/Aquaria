#ifndef BBGE_TEXTUREMGR_H
#define BBGE_TEXTUREMGR_H

#include <map>
#include "Texture.h"
#include "MT.h"


struct TexLoadTmp;

class TextureMgr
{
public:
    TextureMgr();
    ~TextureMgr();

    typedef void (*ProgressCallback)(size_t done, void*);

    std::vector<std::string> loadFromPaths;
    size_t spawnThreads(size_t n);
    size_t getNumLoaded() const;
    Texture *getOrLoad(const std::string& name);
    void clearUnused(); // clear everything whose refcount is 1
    void unloadAll();

    enum LoadMode
    {
        KEEP,          // if already exists, keep unchanged
        KEEP_IF_SAME,  // load if we resolve to a different file than the texture that's already there, if any.
        OVERWRITE,     // always overwrite
    };

    void loadBatch(Texture *pdst[], const std::string texnames[], size_t n, LoadMode mode = KEEP, ProgressCallback cb = 0, void *cbUD = 0);
    Texture *load(const std::string& texname, LoadMode mode);
    void reloadAll(LoadMode mode);

private:
    typedef std::map<std::string, CountedPtr<Texture> > TexCache;
    TexCache cache;

    BlockingQueue<void*> worktodo;
    BlockingQueue<void*> workdone;
    std::vector<void*> threads;
    void *sem; // SDL_sem*

    static int Th_Main(void *self);
    void thMain(); // for int

    void th_loadFromFile(TexLoadTmp& tt) const;
    Texture *finalize(TexLoadTmp& tt);
};

#endif

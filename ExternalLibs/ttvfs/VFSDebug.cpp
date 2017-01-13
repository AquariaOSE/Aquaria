#include "VFSDebug.h"
#include "VFSInternal.h"
#include "VFSRoot.h"
#include "VFSDir.h"
#include "VFSFile.h"
#include "VFSTools.h"

#include <iostream>
#include <set>


VFS_NAMESPACE_START
namespace debug {


struct _DbgParams
{
    _DbgParams(std::ostream& os_, const std::string& path, const std::string& sp_)
        : os(os_), mypath(path), sp(sp_) {}

    std::ostream& os;
    std::string mypath;
    const std::string& sp;
    std::set<std::string> dirnames;
};

static void _DumpFile(File *vf, void *user)
{
    _DbgParams& p = *((_DbgParams*)user);
    p.os << p.sp << " F:" << vf->fullname() << " [" << vf->getType() << ", ref " << vf->getRefCount() << ", 0x" << vf << "]" << std::endl;
}


static void _DumpDir(DirBase *vd, void *user)
{
    _DbgParams& p = *((_DbgParams*)user);
    if(!(vd->fullname()[0] == '/' && vd->fullnameLen() == 1)) // don't recurse down the root dir.
        p.dirnames.insert(vd->name());
    p.os << p.sp << "D : " << vd->fullname() << " [" << vd->getType() << ", ref " << vd->getRefCount() << ", 0x" << vd << "]" << std::endl;
}

static void _DumpTree(_DbgParams& p, Root& vfs, int level)
{
    p.os << ">> [" << p.mypath << "]" << std::endl;

    vfs.ForEach(p.mypath.c_str(), _DumpFile, _DumpDir);

    if(!level)
        return;

    std::string sub = p.sp + "  ";
    for(std::set<std::string>::iterator it = p.dirnames.begin(); it != p.dirnames.end(); ++it)
    {
        _DbgParams recP(p.os, joinPath(p.mypath, it->c_str()), sub);
        _DumpTree(recP, vfs, level - 1);
    }
}

void dumpTree(Root& root, std::ostream& os, const char *path /* = NULL */, int level /* = -1 */)
{
    if(!path)
        path = "";
    os << ">>> FILE TREE DUMP <<<" << std::endl;
    _DbgParams recP(os, path, "");
    _DumpTree(recP, root, level);
}


} // end namespace debug

VFS_NAMESPACE_END


#ifndef LVPA_COMMON_H
#define LVPA_COMMON_H

#include "LVPACompileConfig.h"

#include <stdlib.h>
#include <cstring>

LVPA_NAMESPACE_START

#ifdef _MSC_VER
    typedef __int64            int64;
    typedef long               int32;
    typedef short              int16;
    typedef char               int8;
    typedef unsigned __int64   uint64;
    typedef unsigned long      uint32;
    typedef unsigned short     uint16;
    typedef unsigned char      uint8;
#else
    typedef long long int64;
    typedef int int32;
    typedef short int16;
    typedef char int8;
    typedef unsigned long long uint64;
    typedef unsigned int uint32;
    typedef unsigned short uint16;
    typedef unsigned char uint8;
#endif

struct memblock
{
    memblock() : ptr(NULL), size(0) {}
    memblock(uint8 *p, uint32 s) : size(s), ptr(p) {}
    uint8 *ptr;
    uint32 size;
};


LVPA_NAMESPACE_END


#endif


#ifndef BYTECONVERTER_H
#define BYTECONVERTER_H

#include <algorithm>
#include "LVPAInternal.h" // this is important to fix up any possible ***_ENDIAN misconfigurations

LVPA_NAMESPACE_START

namespace ByteConverter
{
    template<size_t T>
    inline void convert(char *val)
    {
        std::swap(*val, *(val + T - 1));
        convert<T - 2>(val + 1);
    }

    template<> inline void convert<0>(char *) {}
    template<> inline void convert<1>(char *) {}

    template<typename T>
    inline void apply(T *val)
    {
        convert<sizeof(T)>((char *)(val));
    }
}

#if IS_BIG_ENDIAN
template<typename T> inline void ToLittleEndian(T& val) { ByteConverter::apply<T>(&val); }
template<typename T> inline void ToBigEndian(T&) { }
#else
template<typename T> inline void ToLittleEndian(T&) { }
template<typename T> inline void ToBigEndian(T& val) { ByteConverter::apply<T>(&val); }
#endif

template<typename T> void ToLittleEndian(T*);   // will generate link error
template<typename T> void ToBigEndian(T*);      // will generate link error

inline void ToLittleEndian(uint8&) { }
inline void ToLittleEndian(int8&)  { }
inline void ToBigEndian(uint8&) { }
inline void ToBigEndian( int8&) { }

LVPA_NAMESPACE_END

#endif

#ifndef LVPA_COMPILE_CONFIG
#define LVPA_COMPILE_CONFIG

// TODO ADD TEXT
#define LVPA_NAMESPACE lvpa

//#define LVPA_SUPPORT_ZLIB
#define LVPA_SUPPORT_LZMA
//#define LVPA_SUPPORT_LZO
#define LVPA_SUPPORT_LZF



// ------ End of config ------

#ifdef LVPA_NAMESPACE
#  define LVPA_NAMESPACE_START namespace LVPA_NAMESPACE {
#  define LVPA_NAMESPACE_END }
#  define LVPA_NAMESPACE_IMPL LVPA_NAMESPACE::
   namespace LVPA_NAMESPACE {} // predeclare namespace to make compilers happy
#else
#  define LVPA_NAMESPACE_START
#  define LVPA_NAMESPACE_END
#  define LVPA_NAMESPACE_IMPL
#endif


#endif

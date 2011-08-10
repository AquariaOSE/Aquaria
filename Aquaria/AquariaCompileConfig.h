#ifndef __AQUARIA_COMPILE_CONFIG_H__
#define __AQUARIA_COMPILE_CONFIG_H__

// The settings below are also configurable with CMake.
// Define BBGE_SKIP_CONFIG_HEADERS to use CMake-only configuration.
#ifndef BBGE_SKIP_CONFIG_HEADERS

    #define AQUARIA_FULL 1
    //#define AQUARIA_DEMO 1
    #define AQUARIA_BUILD_CONSOLE 1
    #define AQUARIA_BUILD_SCENEEDITOR 1

    #define AQUARIA_CUSTOM_BUILD_ID ""

    // no console window in release mode (note for MSVC: use together with linker SubSystem setting)
    #ifdef NDEBUG
    #  define AQUARIA_WIN32_NOCONSOLE
    #endif

#endif



// Not CMake-configurable defines, change at your own risk

// Should stay always defined; this tracks visited map areas
#define AQUARIA_BUILD_MAPVIS

// Define this to save map visited data in a base64-encoded raw format.
// This can take much less space than the standard text format (as little
// as 10%), but WILL BE INCOMPATIBLE with previous builds of Aquaria --
// the visited data will be lost if the file is loaded into such a build.
// (Current builds will load either format regardless of whether or not
// this is defined.)
//#define AQUARIA_SAVE_MAPVIS_RAW

// Interesting, old test stuff
//#define AQ_TEST_QUADTRAIL


#endif //__AQUARIA_COMPILE_CONFIG_H__

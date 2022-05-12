#ifndef AQUARIA_COMPILE_CONFIG_H
#define AQUARIA_COMPILE_CONFIG_H


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

#endif //__AQUARIA_COMPILE_CONFIG_H__

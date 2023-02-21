# Building Aquaria on Windows

Note that this tries to be a HOWTO for dummies. If you know your way with C++ and CMake then this guide is not for you.

## With CMake

### Prerequisites

- [CMake](https://cmake.org/download/) installed
- Any IDE or compiler toolchain of your choice. [Visual Studio Community Edition](https://visualstudio.microsoft.com/) is recommended.
- [Git for Windows](https://gitforwindows.org/) or a compatible alternative installed. 

- For the sake of this tutorial, assume everything happens in **C:\code**. If this isn't the case on your system, adapt appropriately.
- Hint: Git for windows integrates with Explorer. To get a *git bash* quickly, RClick on a directory and choose "git bash here".

### Build dependencies

#### Build SDL2

- Open a git bash in **C:\code**

- Run these:

```bash
git clone https://github.com/libsdl-org/SDL.git
cd SDL
git checkout release-2.26.3
mkdir build
cd build
cmake-gui ..
```

- In CMake GUI, at the bottom, click Configure
  - As generator, your compiler/IDE should be already selected. If not, select the correct one.
    - When using Visual Studio (full version, not VSCode), select the correct version.
    - If in doubt, try "Unix Makefiles".
  - Use default native compilers
  - Click "Finish". This will take a while.
  - When it's done, everything is red.
  - If there's a CMAKE_BUILD_TYPE entry in the list, enter *Release*
  - Click "Configure" again, then all red should be gone.
  - Click generate.
  
- At this point, project files are generated and the next goal is to start a build. How exactly this works depends on your IDE/compiler.
  - For commandline compilers (MinGW, MSYS, Clang), you can usually just enter `make -j8`  and it'll go for a while and build everything.
  - If you use Visual Studio, change the build type (dropdown near the top of the screen) to *Release*, then **Build > Build Solution**.
  - Any other IDE? No idea. Go figure it out.
- Now SDL should be built. Check that `build/Release/SDL2.dll` exists.
- If all is good, anything SDL related can be closed now.

### Build Aquaria

- Open a git bash in **C:\code**
- Run these:
- 
```bash
git clone https://github.com/AquariaOSE/Aquaria.git
cd Aquaria
git checkout experimental
mkdir build
cd build
cmake-gui ..
```

- Now, the same as previously with SDL:
- In CMake GUI, at the bottom, click Configure
  - As generator, your compiler/IDE should be already selected. If not, select the correct one.
    - When using Visual Studio (full version, not VSCode), select the correct version.
    - If in doubt, try "Unix Makefiles".
  - Use default native compilers
  - Click "Finish". This will take a while and eventually pop an error. In the log there should be "Could NOT find SDL2".
  - This is normal. CMake doesn't track things system-wide so it has no idea we just built SDL.
  - Find the entry SDL2MAIN_LIBRARY (that should be NOTFOUND), click that, use the [...] to navigate and select **C:/code/SDL/build/Release/SDL2main.lib**
  - For SDL2_INCLUDE_DIR, select **C:/code/SDL/include**
  - For SDL2_LIBRARY_TEMP, select **C:/code/SDL/build/Release/SDL2.lib**
  - Check that CMAKE_BUILD_TYPE is set to *Release*
  - Click Configure twice
  - Now all red lines should be gone.  Click Generate.
- Build Aquaria. Same procedure as with SDL.
  - If using Visual Studio, don't forget to set the build type to release first (it doesn't care about the CMake setting).
- If all goes well, the finished executable can be found as **C:/code/Aquaria/build/Aquaria/Release/aquaria.exe**

## Updating and running the game

- Copy the built aquaria.exe and also the SDL2.dll from earlier into your Aquaria game directory.
- Copy the contents of **C:\code\Aquaria\files** (that is, *data*, *gfx*, ..., *scripts*, ...) over the existing files and directories in your Aquaira game directory. Overwrite everything.
- Now the updated build is ready to run!


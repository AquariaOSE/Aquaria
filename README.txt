This folder contains all Aquaria sources and necessary build scripts.
However, it does *not* contain any graphical file nor sound. If you
want to play the game, you first need to buy the original
full-featured version (http://www.bit-blot.com/aquaria/) and install
it. Once you have done that, you need to build the files in this
folder (see below for how to do that) and copy the resulting files to
the place where you installed the original full-featured version.

In case of problems, you can get support via IRC:
#bitblot @ irc.esper.net.


TL;DR for contributions:
-------------------------
- Branches:
  * master - stable branch
  * experimental - merged into master when a new release is due
  Other branches are feature/testing branches.

- Pull requests / new developments should go into experimental.
- Major bug fixes, security relevant things, or easy platform compile fixes can be made against master.



[1] BUILDING
------------

Follow these steps to build Aquaria. 

1- Install required dependencies first. This includes a C++ compiler
  and a handful of libraries. Here is a list of the Debian names for
  some of these dependencies:

build-essential cmake liblua5.1-0-dev libogg-dev libvorbis-dev
libopenal-dev libsdl1.2-dev

For Lua, libogg, and libvorbis the included versions can be used instead,
thus a system-wide install of these libs is not required.

2- Create a sub-directory 'cmake-build' and move into it

$ mkdir cmake-build
$ cd cmake-build

3- run cmake

$ cmake ..

Alternatively, if you want to specifiy compile-time options:
(See also [2] SETUP further down)

$ ccmake ..


4- If you miss some dependencies, install them and run cmake again.
   Due to windows lacking package management, it is recommended
   to set all AQUARIA_INTERNAL_* cmake variables to TRUE for win32
   builds, or for self-contained linux builds.

5- run make

$ make

6- If everything went well, the 'aquaria' binary is now in the
   current directory (cmake-build).



[2] SETUP
---------

First, be sure you have an existing installation of Aquaria.
This can be a Windows, Linux, or Mac OSX version, doesn't matter.

Take this directory tree, showing the mandatory subdirs:

Aquaria  <--- this is the directory you want!
 |
 +--- data
 +--- gfx
 +--- mus
 +--- scripts
 +--- sfx
 +--- vox
 +--- ...

There are three ways to setup Aquaria, you may choose the one that
fits your intentions most:


****
*** If you just want to get the thing working right now ***
*** and don't care much about updating frequently       ***
****

After building, copy the executable to your aquaria root directory.
Then copy all files inside the repo's "files" directory
to your Aquaria root dir, replacing everything that exists.

You should *not* remove any file from the Aquaria installation, just
replace some of them with the versions included in the repo.

/!\ Be careful if you do this on a Mac with finder because
    it replaces whole folders instead of just merging the new files in.
    You have been warned.

Lastly, if you want to play the included default mods,
copy game_scripts/_mods over your local _mods directory.



****
*** If you want to hack on Aquaria or update frequently: ***
****

* Linux/OSX: 
Set the AQUARIA_DATA_PATH environment variable to the installation
directory as described above. E.g. add this line to your ~/.profile:

  export AQUARIA_DATA_PATH=~/games/Aquaria

Alternatively, set the AQUARIA_DEFAULT_DATA_DIR compile time option
when building with cmake.

Then, go to $AQUARIA_DATA_PATH and create a symlink
to the updated files directory:

  cd ~/games/Aquaria
  ln -s ~/code/Aquaria-repo/files override

This will make sure that whenever you update the repo, the updated datafiles
from the repo will be loaded instead of those that ship with the game.

Lastly, if you want to play the included default mods,
copy game_scripts/_mods over your local _mods directory.



* Windows:

You'll probably want to install something like this for symlink/junction support:
--> http://schinagl.priv.at/nt/hardlinkshellext/hardlinkshellext.html
(You don't have to, but it's rather tedious to update everything by hand)

Take all subdirs from the Aquaria root directory, and copy them into
path/to/Aquaria-repo/bin, or whereever your compiler will output its executable.
If you are able to, better create junctions to save space.

Note that this will become your main working directory.

Unfortunately you will have to use junctions to link the updated files.
Create a junction in path/to/Aquaria-repo/bin that points to
path/to/Aquaria-repo/files.
Otherwise, if you don't use junctions, copy the files dir by hand should they change.
You can either name it "override" and drop it into bin, or simply copy & replace
its contents into bin if you don't mind keeping your data clean.

Lastly, if you want to play the included default mods,
copy game_scripts/_mods over your local _mods directory.



****
*** If you're a Linux package maintainer ***
****

I assume there is a package for e.g. the Humble Bundle version available;
you'll want to make sure that the game path is fixed and the same
on all user's installs when you set the following cmake options.

Set the AQUARIA_DEFAULT_DATA_DIR cmake option to the game data dir
as described above.
Make sure your package includes all files from the repo's "files" directory.
Set the AQUARIA_EXTRA_DATA_DIR cmake option to the directory where these
extra files will be installed by the package.

An example can be found here:
https://aur.archlinux.org/packages/aquaria-git/

Using this package as data dependecy:
https://aur.archlinux.org/packages/aquaria-data-hib/

You also need to make sure that the mod scripts in ~/.Aquaria/_mods/*/scripts/
are updated with those from the repo's game_scripts/_mods directory.


If this doesn't apply to your setup, ask. [email, github, IRC, ...]



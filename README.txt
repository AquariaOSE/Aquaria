This folder contains all Aquaria sources and necessary build scripts.
However, it does *not* contain any graphical file nor sound. If you
want to play the game, you first need to buy the original
full-featured version (http://www.bit-blot.com/aquaria/) and install
it. Once you have done that, you need to build the files in this
folder (see below for how to do that) and copy the resulting files to
the place where you installed the original full-featured version.

BUILDING
--------

Follow these steps to build Aquaria. 

1- Install required dependencies first. This includes a C++ compiler
  and a handful of libraries. Here is a list of the Debian names for
  some of these dependencies:

build-essential cmake liblua5.1-0-dev libogg-dev libvorbis-dev
libopenal-dev libsdl1.2-dev

2- Create a sub-directory 'cmake-build' and move into it

$ mkdir cmake-build
$ cd cmake-build

3- run cmake

$ cmake ..

4- If you miss some dependencies, install them and run cmake again.

5- run make

$ make

6- Copy necessary files to where you installed the original
  full-featured version of Aquaria (e.g., ~/aquaria which is the
  default)

$ cp aquaria ~/aquaria/
$ cp -r ../games_scripts/* ~/aquaria

You should *not* remove any file from the aquaria installation, just
replace some of them with the versions included in this folder.

MODS
----

If you plan to use any of the Aquaria mods, you'll also need to update
the copies in your personal data directory:

cp -a ~/aquaria/_mods ~/.Aquaria/

LINUX RUMBLE SUPPORT
--------------------

SDL 1.2 does not support rumble features, even though Linux does. This
feature will be added in SDL 1.3, which is still a long time coming.

In the meantime there is a hackish rumble implementation for Linux that
needs environment variables to be set that map joysticks via their indices
to event devices. E.g. to map the first joystick to the event device
"/dev/input/event6" you need to run aquaria like this:

$ export AQUARIA_EVENT_JOYSTICK0=/dev/input/event6
$ aquaria

Because aquaria is a single player game you never need to map another joystick
than the first one. Also keep in mind that your joystick event device has
another path. E.g. I use this command to run aquaria:

$ export AQUARIA_EVENT_JOYSTICK0=/dev/input/by-id/usb-©Microsoft_Corporation_Controller_0709960-event-joystick
$ aquaria

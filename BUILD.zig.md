# Building Aquaria with Zig

The Zig build is an alternative to CMake. It needs nothing installed except Zig: no compiler, no IDE, no SDL.
SDL2 is built from source automatically. It also cross-compiles, so you can build the Windows, Linux and
(with some extra setup) macOS versions from any machine.

## Quick start

1. Install [Zig](https://ziglang.org/download/) **0.17.0** (or newer, if the build still works with it).
   On Windows, unpack the zip somewhere and add that folder to your `PATH`, or use a package manager (`choco install zig`, `scoop install zig`, `winget install zig.zig`).
2. Get the source (`git clone https://github.com/AquariaOSE/Aquaria.git`) and open a terminal in the `Aquaria` folder.
3. Run:

```
zig build --release=fast
```

The first build downloads the SDL2 source (and, for Linux targets, a header package) into `zig-pkg/` and takes a
few minutes. Later builds are fast.

The game executable is in `zig-out/bin/` (`aquaria.exe` on Windows, `aquaria` elsewhere).

To play, copy it into your Aquaria game directory together with the contents of `files/` (`data`, `gfx`,
`scripts`, ...), overwriting what's there. With the Zig build, SDL is linked into the executable, so no `SDL2.dll` is needed.

## Common commands

| Command | Result |
|---|---|
| `zig build` | Debug build (slow, with asserts and debug info) |
| `zig build --release=fast` | Optimized release build |
| `zig build --release=safe` | Optimized, but keeps `assert()`s |
| `zig build --release=fast -Dstrip` | Release build without debug info (much smaller) |
| `zig build -Dtarget=x86_64-windows-gnu` | Cross-compile for 64-bit Windows |
| `zig build -Dtarget=x86-windows-gnu` | 32-bit Windows |
| `zig build -Dtarget=x86_64-linux-gnu.2.17` | 64-bit Linux, runs on any distro with glibc 2.17 or newer |
| `zig build -Dtarget=aarch64-linux-gnu.2.17` | 64-bit ARM Linux (Raspberry Pi 4/5, ...) |
| `zig build dist --release=fast -Dstrip` | All of the above at once, into `zig-out/<target>/` |
| `zig build --help` | All options |

Always add the `.2.17` to Linux targets. Without it, Zig picks a newer glibc and the binary won't run on older distros.
A native build on Linux (no `-Dtarget`) already uses 2.17.

## Options

| Option | Default | Meaning |
|---|---|---|
| `-Dconsole=false` | on | No console window / console log (Debug builds always log) |
| `-Dsystem-sdl` | off | Link the system's SDL2 via pkg-config instead of building it |
| `-Dvfs=false` | on | Build without the virtual file system |
| `-Ddir-watch=false` | on | Build without the texture hot-reload directory watcher |
| `-Ddemo` | off | Demo build (no console, no scene editor) |
| `-Dsanitize` | off | Trap on undefined behaviour in game/engine code (debugging aid) |
| `-Dstrip` | off | Strip debug info |
| `-Dcustom-build-id=" (my build)"` | | Text appended to the version on the title screen |
| `-Doverride-version-string=...` | | Replaces the version text on the title screen |
| `-Ddefault-data-dir=...`, `-Dextra-data-dir=...` | | Data directories (package maintainers) |
| `-Ddebug-show-paths` | off | Print important paths at startup |
| `-Dmacos-sdk=/path/to/MacOSX.sdk` | | macOS SDK, for building macOS targets on a non-Mac |

## Linux

The default Linux build links SDL2, libc++ and all other libraries statically. Only glibc is linked dynamically.
SDL loads X11 or Wayland, OpenGL/EGL, ALSA/PulseAudio, udev and D-Bus at runtime, so the binary runs on any distro
that has them.

Both video backends are built. SDL2 tries X11 first (also XWayland), then native Wayland, so a Wayland-only system
without XWayland works too. To use native Wayland in a Wayland session that also has XWayland, set
`SDL_VIDEODRIVER=wayland` (or `wayland,x11` to fall back). Native Wayland needs `libwayland-client`,
`libwayland-egl`, `libwayland-cursor`, `libxkbcommon` and `libEGL`; `libdecor` is optional and draws window
decorations on compositors without server-side decorations (GNOME).

A fully static (musl) binary is not possible: it couldn't load the system's OpenGL driver.
`-Dtarget=x86_64-linux-musl` works, but the result only runs on musl distros such as Alpine.

### Distro packagers

Use the system libc and SDL2:

```
zig build --release=fast -Dtarget=native -Dsystem-sdl -Ddefault-data-dir=/usr/share/aquaria
```

`-Dtarget=native` overrides the glibc 2.17 default, and `-Dsystem-sdl` uses `pkg-config sdl2`.

## macOS

On a Mac with Xcode installed, `zig build --release=fast` should work.
To build for macOS on another OS, Apple's SDK is needed: `-Dmacos-sdk=/path/to/MacOSX.sdk`. Zig doesn't ship it,
and Apple's license restricts its use on non-Apple hardware, so you have to provide it yourself.
`zig build dist` includes the macOS targets only on a Mac or when `-Dmacos-sdk` is given.

## Notes

- The Zig build is kept in sync with the CMake build: same sources, same defines.
  If you add or remove a source file, update both `CMakeLists.txt` and `build.zig`.
- `build.zig` builds the game; `zig/sdl2.zig` builds SDL2. The SDL2 config headers are in `zig/`.
- Downloaded packages go into `zig-pkg/`, build output into `zig-out/` and the cache into `.zig-cache/`.
  All of them can be deleted at any time.

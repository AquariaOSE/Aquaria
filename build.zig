//! Zig build for Aquaria, kept in sync with the CMake build. See BUILD.zig.md.
//!   zig build --release=fast                         native build into zig-out/bin
//!   zig build -Dtarget=x86_64-windows-gnu                    cross-compile
//!   zig build dist --release=fast                    all release targets into zig-out/<target>/

const std = @import("std");
const builtin = @import("builtin");
const sdl2 = @import("zig/sdl2.zig");

const glibc_min: std.SemanticVersion = .{ .major = 2, .minor = 17, .patch = 0 };

const Options = struct {
    system_sdl: bool,
    console: bool,
    vfs: bool,
    dir_watch: bool,
    demo: bool,
    sanitize: bool,
    strip: ?bool,
    debug_show_paths: bool,
    custom_build_id: ?[]const u8,
    override_version_string: ?[]const u8,
    default_data_dir: ?[]const u8,
    extra_data_dir: ?[]const u8,
    macos_sdk: ?std.Build.LazyPath,
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{
        .default_target = if (builtin.os.tag == .linux) .{ .abi = .gnu, .glibc_version = glibc_min } else .{},
    });
    const optimize = b.standardOptimizeOption(.{});

    const opts: Options = .{
        .system_sdl = b.option(bool, "system-sdl", "Link the system's SDL2 (via pkg-config) instead of building it from source") orelse false,
        .console = b.option(bool, "console", "Enable console output (always on in Debug builds)") orelse false,
        .vfs = b.option(bool, "vfs", "Use the virtual file system (needed for some additional features)") orelse true,
        .dir_watch = b.option(bool, "dir-watch", "Directory watcher to hot-reload textures in mods") orelse true,
        .demo = b.option(bool, "demo", "Demo build") orelse false,
        .sanitize = b.option(bool, "sanitize", "UBSan traps in game and engine code (not in external libs)") orelse false,
        .strip = b.option(bool, "strip", "Strip debug info from the executable"),
        .debug_show_paths = b.option(bool, "debug-show-paths", "Show important paths at startup") orelse false,
        .custom_build_id = b.option([]const u8, "custom-build-id", "Text appended to the version ID on the title screen"),
        .override_version_string = b.option([]const u8, "override-version-string", "Text shown instead of the version ID on the title screen"),
        .default_data_dir = b.option([]const u8, "default-data-dir", "Default data directory (package maintainers)"),
        .extra_data_dir = b.option([]const u8, "extra-data-dir", "Extra data directory overriding the default one (package maintainers)"),
        .macos_sdk = b.option(std.Build.LazyPath, "macos-sdk", "Path to MacOSX.sdk, for building macOS targets on a non-Mac host"),
    };

    b.installArtifact(addAquaria(b, target, optimize, opts));

    const dist = b.step("dist", "Build all release targets into zig-out/<target>/");
    const can_macos = builtin.os.tag == .macos or opts.macos_sdk != null;
    for (dist_targets) |q| {
        if (q.os_tag == .macos and !can_macos) continue;
        const t = b.resolveTargetQuery(q);
        const exe = addAquaria(b, t, optimize, opts);
        const dir = q.zigTriple(b.allocator) catch @panic("OOM");
        dist.dependOn(&b.addInstallArtifact(exe, .{ .dest_dir = .{ .override = .{ .custom = dir } } }).step);
    }
}

const dist_targets = [_]std.Target.Query{
    .{ .cpu_arch = .x86_64, .os_tag = .windows, .abi = .gnu },
    .{ .cpu_arch = .x86, .os_tag = .windows, .abi = .gnu },
    .{ .cpu_arch = .x86_64, .os_tag = .linux, .abi = .gnu, .glibc_version = glibc_min },
    .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .gnu, .glibc_version = glibc_min },
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .macos },
};

fn addAquaria(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.lang.Optimize, opts: Options) *std.Build.Step.Compile {
    const t = target.result;
    const os = t.os.tag;
    if (os != .windows and os != .linux and os != .macos)
        @panic("Aquaria: unsupported target OS (supported: windows, linux, macos)");
    if (os == .macos and builtin.os.tag != .macos and opts.macos_sdk == null and !opts.system_sdl)
        @panic("Aquaria: building for macOS on a non-Mac host needs -Dmacos-sdk=<path to MacOSX.sdk>");

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
        .stack_protector = false,
        .sanitize_c = .off,
        .strip = opts.strip,
    });
    const exe = b.addExecutable(.{ .name = "aquaria", .root_module = mod });

    if (opts.system_sdl) {
        mod.linkSystemLibrary("SDL2", .{});
    } else {
        const sdl = sdl2.build(b, target, optimize, opts.macos_sdk);
        mod.addIncludePath(sdl.include);
        mod.linkLibrary(sdl.lib);
        _ = sdl2.addPlatformPaths(b, mod, opts.macos_sdk);
    }

    // Defines that CMake sets for everything
    mod.addCMacro("AQUARIA_INTERNAL_FTGL", "1");
    mod.addCMacro("AQUARIA_INTERNAL_LUA", "1");
    mod.addCMacro("FTGL_LIBRARY_STATIC", "1");
    mod.addCMacro("HAVE_STRCASECMP", "1");
    if (opts.vfs) {
        mod.addCMacro("VFS_ENABLE_C_API", "1");
        mod.addCMacro("BBGE_BUILD_VFS", "1");
    }
    if (opts.demo) {
        mod.addCMacro("AQUARIA_DEMO", "1");
    } else {
        mod.addCMacro("AQUARIA_BUILD_CONSOLE", "1");
        mod.addCMacro("AQUARIA_BUILD_SCENEEDITOR", "1");
    }
    if (opts.dir_watch) mod.addCMacro("AQUARIA_ENABLE_DIR_WATCH", "1");
    if (opts.debug_show_paths) mod.addCMacro("AQUARIA_DEBUG_SHOW_PATHS", "1");
    if (opts.console or optimize == .debug) mod.addCMacro("AQUARIA_ENABLE_CONSOLE_LOG", "1");
    if (opts.custom_build_id) |s| mod.addCMacro("AQUARIA_CUSTOM_BUILD_ID", cString(b, s));
    if (opts.override_version_string) |s| mod.addCMacro("AQUARIA_OVERRIDE_VERSION_STRING", cString(b, s));
    if (opts.default_data_dir) |s| mod.addCMacro("AQUARIA_DEFAULT_DATA_DIR", cString(b, s));
    if (opts.extra_data_dir) |s| mod.addCMacro("AQUARIA_EXTRA_DATA_DIR", cString(b, s));
    switch (optimize) {
        .debug => mod.addCMacro("_DEBUG", "1"),
        .fast, .small => mod.addCMacro("NDEBUG", "1"),
        .safe => {},
    }
    switch (os) {
        .windows => {
            mod.addCMacro("BBGE_BUILD_WINDOWS", "1");
            mod.addCMacro("_CRT_SECURE_NO_WARNINGS", "1");
            mod.addCMacro("_CRT_SECURE_NO_DEPRECATE", "1");
        },
        .macos => {
            mod.addCMacro("BBGE_BUILD_UNIX", "1");
            mod.addCMacro("BBGE_BUILD_MACOSX", "1");
        },
        else => mod.addCMacro("BBGE_BUILD_UNIX", "1"),
    }

    for ([_][]const u8{
        "BBGE",
        "ExternalLibs",
        "ExternalLibs/AL",
        "ExternalLibs/FTGL/include",
        "ExternalLibs/freetype2/include",
        "ExternalLibs/freetype2/include/freetype/config",
        "ExternalLibs/freetype2/src",
        "ExternalLibs/lua-5.1.4/src",
        "ExternalLibs/libogg/include",
        "ExternalLibs/libvorbis/include",
        "ExternalLibs/tinyxml2",
        "ExternalLibs/ttvfs",
        "ExternalLibs/ttvfs_zip",
        "ExternalLibs/ttvfs_cfileapi",
    }) |dir| mod.addIncludePath(b.path(dir));

    const common = [_][]const u8{ "-ffast-math", "-fsigned-char" };
    const ext_flags: []const []const u8 = &common;
    const game_flags: []const []const u8 = if (opts.sanitize)
        &(common ++ [_][]const u8{ "-fsanitize=undefined", "-fsanitize-trap=undefined" })
    else
        &common;

    mod.addCSourceFiles(.{ .root = b.path("Aquaria"), .files = &aquaria_files, .flags = game_flags });
    mod.addCSourceFiles(.{ .root = b.path("BBGE"), .files = &bbge_files, .flags = game_flags });

    mod.addCSourceFiles(.{ .root = b.path("ExternalLibs"), .files = &extlib_files, .flags = ext_flags });
    mod.addCSourceFiles(.{ .root = b.path("ExternalLibs"), .files = &.{"mojoal.c"}, .flags = &(common ++ [_][]const u8{"-DAL_LIBTYPE_STATIC=1"}) });
    mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/tinyxml2"), .files = &.{"tinyxml2.cpp"}, .flags = ext_flags });
    mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/FTGL/src"), .files = &ftgl_files, .flags = ext_flags });
    mod.addCSourceFiles(.{
        .root = b.path("ExternalLibs/freetype2/src"),
        .files = &freetype_files,
        .flags = &(common ++ [_][]const u8{ "-fno-strict-aliasing", "-DFT2_BUILD_LIBRARY", "-DFT_CONFIG_OPTION_SYSTEM_ZLIB" }),
    });
    mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/libogg/src"), .files = &.{ "bitwise.c", "framing.c" }, .flags = ext_flags });
    mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/libvorbis/lib"), .files = &vorbis_files, .flags = ext_flags });
    mod.addCSourceFiles(.{
        .root = b.path("ExternalLibs/lua-5.1.4/src"),
        .files = &lua_files,
        .flags = switch (os) {
            .linux => &(common ++ [_][]const u8{"-DLUA_USE_LINUX"}),
            .macos => &(common ++ [_][]const u8{"-DLUA_USE_MACOSX"}),
            else => ext_flags,
        },
        .language = .cpp,
    });
    if (opts.vfs) {
        mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/ttvfs"), .files = &ttvfs_files, .flags = ext_flags });
        mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/ttvfs_zip"), .files = &ttvfs_zip_files, .flags = ext_flags });
        mod.addCSourceFiles(.{ .root = b.path("ExternalLibs/ttvfs_cfileapi"), .files = &.{"ttvfs_stdio.cpp"}, .flags = ext_flags });
    }

    switch (os) {
        .windows => {
            exe.subsystem = if (opts.console) .console else .windows;
            mod.addWin32ResourceFile(.{ .file = b.path("Aquaria/aquaria.rc"), .include_paths = &.{b.path("Aquaria")} });
            mod.linkSystemLibrary("ws2_32", .{});
            mod.linkSystemLibrary("dbghelp", .{});
        },
        .linux => {
            mod.linkSystemLibrary("dl", .{});
            mod.linkSystemLibrary("pthread", .{});
            mod.linkSystemLibrary("m", .{});
        },
        .macos => {
            mod.linkFramework("Carbon", .{});
            mod.linkFramework("Cocoa", .{});
        },
        else => unreachable,
    }

    return exe;
}

fn cString(b: *std.Build, s: []const u8) []const u8 {
    return b.fmt("\"{s}\"", .{s});
}

const aquaria_files = [_][]const u8{
    "AnimationEditor.cpp",    "AquariaComboBox.cpp",  "AquariaMenuItem.cpp",    "AquariaSaveSlot.cpp",
    "Avatar.cpp",             "Beam.cpp",             "BitBlotLogo.cpp",        "CollideEntity.cpp",
    "Continuity.cpp",         "Credits.cpp",          "DSQ.cpp",                "Emote.cpp",
    "Entity.cpp",             "FlockEntity.cpp",      "Game.cpp",               "GameEnums.cpp",
    "GameplayVariables.cpp",  "GameStructs.cpp",      "GasCloud.cpp",           "GridRender.cpp",
    "Hair.cpp",               "InGameMenu.cpp",       "Ingredient.cpp",         "Intro.cpp",
    "Main.cpp",               "ManaBall.cpp",         "MiniMapRender.cpp",      "Mod.cpp",
    "ModDownloader.cpp",      "ModSelector.cpp",      "Network.cpp",            "ParticleEditor.cpp",
    "Path.cpp",               "PathFinding.cpp",      "PathRender.cpp",         "QuadSegmentRender.cpp",
    "RecipeMenuEntry.cpp",    "SceneEditor.cpp",      "SchoolFish.cpp",         "Scriptable.cpp",
    "ScriptedEntity.cpp",     "ScriptInterface.cpp",  "Segmented.cpp",          "SFXLoops.cpp",
    "Shot.cpp",               "Spore.cpp",            "States.cpp",             "StatsAndAchievements.cpp",
    "SubtitlePlayer.cpp",     "TileMgr.cpp",          "ToolTip.cpp",            "UserSettings.cpp",
    "WaterSurfaceRender.cpp", "Web.cpp",              "WorldMapRender.cpp",     "WorldMapTiles.cpp",
};

const bbge_files = [_][]const u8{
    "ActionInput.cpp",      "ActionMapper.cpp",   "ActionSet.cpp",        "ActionStatus.cpp",
    "AfterEffect.cpp",      "Base.cpp",           "BitmapFont.cpp",       "Core.cpp",
    "Interpolators.cpp",    "DarkLayer.cpp",      "DataStructures.cpp",   "DebugFont.cpp",
    "DirWatcher.cpp",       "Emitter.cpp",        "Event.cpp",            "FmodOpenALBridge.cpp",
    "FrameBuffer.cpp",      "GameKeyNames.cpp",   "GLLoad.cpp",           "Gradient.cpp",
    "Image.cpp",            "Joystick.cpp",       "Localization.cpp",     "MT.cpp",
    "OSFunctions.cpp",      "ParticleEffect.cpp", "Panel.cpp",            "ParticleManager.cpp",
    "Precacher.cpp",        "Quad.cpp",           "QuadGrid.cpp",         "Randomness.cpp",
    "ReadXML.cpp",          "RenderBase.cpp",     "RenderGrid.cpp",       "RenderObject.cpp",
    "RenderObjectLayer.cpp", "RenderState.cpp",   "RoundedRect.cpp",      "ScreenTransition.cpp",
    "ScriptObject.cpp",     "Shader.cpp",         "SkeletalSprite.cpp",   "Slider.cpp",
    "SoundManager.cpp",     "SpineQuad.cpp",      "SplineGrid.cpp",       "SplineQuad.cpp",
    "StateMachine.cpp",     "StateManager.cpp",   "Strand.cpp",           "StringBank.cpp",
    "Texture.cpp",          "TextureMgr.cpp",     "TTFFont.cpp",          "Tile.cpp",
    "TileRender.cpp",       "Tileset.cpp",        "VertexBuffer.cpp",     "Vector.cpp",
    "Window.cpp",           "Window_SDL1.cpp",    "Window_SDL2.cpp",
};

const extlib_files = [_][]const u8{
    "DeflateCompressor.cpp", "luaalloc.cpp", "minihttp.cpp", "miniz.cpp", "tinylibs.cpp", "glfont2/glfont2.cpp",
};

const ftgl_files = [_][]const u8{
    "FTCharmap.cpp",       "FTFace.cpp",        "FTFont.cpp", "FTGLTextureFont.cpp", "FTGlyph.cpp",
    "FTGlyphContainer.cpp", "FTLibrary.cpp",    "FTPoint.cpp", "FTSize.cpp",         "FTTextureGlyph.cpp",
};

const freetype_files = [_][]const u8{
    "base/ftsystem.c",   "base/ftdebug.c",    "base/ftinit.c",     "base/ftbase.c",    "base/ftbbox.c",
    "base/ftbdf.c",      "base/ftbitmap.c",   "base/ftcid.c",      "base/ftfstype.c",  "base/ftgasp.c",
    "base/ftglyph.c",    "base/ftgxval.c",    "base/ftlcdfil.c",   "base/ftmm.c",      "base/ftotval.c",
    "base/ftpatent.c",   "base/ftpfr.c",      "base/ftstroke.c",   "base/ftsynth.c",   "base/fttype1.c",
    "base/ftwinfnt.c",   "truetype/truetype.c", "type1/type1.c",   "cff/cff.c",        "cid/type1cid.c",
    "pfr/pfr.c",         "type42/type42.c",   "winfonts/winfnt.c", "pcf/pcf.c",        "bdf/bdf.c",
    "sfnt/sfnt.c",       "autofit/autofit.c", "pshinter/pshinter.c", "raster/raster.c", "smooth/smooth.c",
    "cache/ftcache.c",   "gzip/ftgzip.c",     "lzw/ftlzw.c",       "psaux/psaux.c",    "psnames/psmodule.c",
};

const vorbis_files = [_][]const u8{
    "analysis.c", "bitrate.c", "block.c",    "codebook.c", "envelope.c",  "floor0.c",    "floor1.c",
    "info.c",     "lpc.c",     "lsp.c",      "mapping0.c", "mdct.c",      "psy.c",       "registry.c",
    "res0.c",     "sharedbook.c", "smallft.c", "synthesis.c", "vorbisfile.c", "window.c",
};

const lua_files = [_][]const u8{
    "lapi.c",    "lauxlib.c", "lbaselib.c", "lcode.c",   "ldblib.c",  "ldebug.c",   "ldo.c",
    "ldump.c",   "lfunc.c",   "lgc.c",      "linit.c",   "liolib.c",  "llex.c",     "lmathlib.c",
    "lmem.c",    "loadlib.c", "lobject.c",  "lopcodes.c", "loslib.c", "lparser.c",  "lstate.c",
    "lstring.c", "lstrlib.c", "ltable.c",   "ltablib.c", "ltm.c",     "lundump.c",  "lvm.c",
    "lzio.c",    "print.c",
};

const ttvfs_files = [_][]const u8{
    "VFSBase.cpp",   "VFSDebug.cpp",     "VFSDir.cpp",  "VFSDirInternal.cpp", "VFSDirView.cpp",
    "VFSFile.cpp",   "VFSFileFuncs.cpp", "VFSLoader.cpp", "VFSRoot.cpp",      "VFSSystemPaths.cpp",
    "VFSTools.cpp",
};

const ttvfs_zip_files = [_][]const u8{
    "VFSDirZip.cpp", "VFSFileZip.cpp", "VFSZipArchiveLoader.cpp", "VFSZipArchiveRef.cpp",
};
